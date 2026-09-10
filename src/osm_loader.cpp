#include "../include/osm_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    constexpr double METRES_PER_MILE = 1609.344;

    std::vector<std::string> parseCsvLine(const std::string& line) { // written by AI
        std::vector<std::string> fields;
        std::string field;
        bool insideQuotes = false;

        for (size_t i = 0; i < line.size(); i++) {
            char character = line[i];
            if (character == '"') {
                if (insideQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    i++;
                } 
                else {
                    insideQuotes = !insideQuotes;
                }
            } 
            else if (character == ',' && !insideQuotes) {
                fields.push_back(field);
                field.clear();
            } 
            else {
                field += character;
            }
        }

        fields.push_back(field);
        return fields;
    }

    bool parseBool(const std::string& value) {

        return value == "True" ||
               value == "true" ||
               value == "1";
    }

    void loadNodes(RoadNetwork& network, const std::string& nodesFile) {

        std::ifstream nodeInput(nodesFile);

        if (!nodeInput) throw std::runtime_error("Could not open nodes file: " + nodesFile );

        std::string line;

        // Skip header
        std::getline(nodeInput, line);

        while (std::getline(nodeInput, line)) {

            std::vector<std::string> fields = parseCsvLine(line);

            if (fields.size() != 3) throw std::runtime_error("Invalid node CSV row: expected 3 fields");

            std::int64_t osmId = std::stoll(fields[0]);
            double latitude = std::stod(fields[1]);
            double longitude = std::stod(fields[2]);

            network.addNode(osmId, latitude, longitude);
        }
    }

    void loadEdges(RoadNetwork& network, const std::string& edgesFile) {

        std::ifstream edgeInput(edgesFile);

        if (!edgeInput) {
            throw std::runtime_error("Could not open edges file: " + edgesFile);
        }

        std::string line;

        // Skip header
        std::getline(edgeInput, line);

        while (std::getline(edgeInput, line)) {
            std::vector<std::string> fields = parseCsvLine(line);

            if (fields.size() != 7) throw std::runtime_error("Invalid edge CSV row: expected 7 fields");

            // CSV format:
            // source,target,name,length_m,highway,speed_mph,oneway

            std::int64_t sourceOsmId = std::stoll(fields[0]);
            std::int64_t targetOsmId = std::stoll(fields[1]);

            std::string name = fields[2];

            double lengthMetres = std::stod(fields[3]);

            std::string highway = fields[4];

            float speedLimit = std::stof(fields[5]);

            bool oneway = parseBool(fields[6]);

            if (!network.containsNode(sourceOsmId)) throw std::runtime_error("Edge references missing source node: " + std::to_string(sourceOsmId));
            if (!network.containsNode(targetOsmId)) throw std::runtime_error("Edge references missing target node: " + std::to_string(targetOsmId));

            std::uint32_t source = network.getNodeIndex(sourceOsmId);
            std::uint32_t target = network.getNodeIndex(targetOsmId);

            float distanceMiles = static_cast<float>(lengthMetres / METRES_PER_MILE);

            network.addEdge(source, RoadEdge{
                .destination = target,
                .name = name,
                .distance = distanceMiles,
                .speedLimit = speedLimit
            });

            if (!oneway) {

                network.addEdge(target, RoadEdge{
                    .destination = source,
                    .name = name,
                    .distance = distanceMiles,
                    .speedLimit = speedLimit
                });

            }
        }
    }
}

RoadNetwork loadOsmNetwork(const std::string& nodesFile, const std::string& edgesFile) {

    RoadNetwork network;

    loadNodes(network, nodesFile);
    loadEdges(network, edgesFile);

    return network;
}