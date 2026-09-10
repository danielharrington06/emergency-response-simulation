#include "../include/osm_loader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

constexpr double METRES_PER_MILE = 1069.344;

std::string removeQuotes(const std::string& value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size()-2);
    }
    return value;
}

bool parseBool(const std::string& value) {
    return value == "True" || value == "true" || value == "1";
}

void loadNodes(RoadNetwork network, const std::string& nodesFile) {
    std::ifstream nodeInput(nodesFile);

    if (!nodeInput) {
        throw std::runtime_error("Could not open nodes file: " + nodesFile);
    }

    std::string line;

    // skip header
    std::getline(nodeInput, line);

    while (std::getline(nodeInput, line)) {
        std::stringstream ss(line);

        std::string idString;
        std::string latitudeString;
        std::string longitudeString;

        std:getline(ss, idString, ',');
        std:getline(ss, latitudeString, ',');
        std:getline(ss, longitudeString, ',');

        int64_t osmId = std::stoll(idString);
        double latitude = std::stod(latitudeString);
        double longitude = std::stod(longitudeString);

        network.addNode(osmId, latitude, longitude);
    }
}

void loadEdges(RoadNetwork network, const std::string& edgesFile) { 
    std::ifstream edgeInput(edgesFile);

    if (!edgeInput) {
        throw std::runtime_error("Could not open edges file: " + edgesFile);
    }

    std::string line;

    // skip header
    std::getline(edgeInput, line);

    while (std::getline(edgeInput, line)) {
        std::stringstream ss(line);

        std::string sourceString;
        std::string targetString;
        std::string lengthString;
        std::string highwayString;
        std::string speedString;
        std::string onewayString;
        std::string nameString;

        std::getline(ss, sourceString, ',');
        std::getline(ss, targetString, ',');
        std::getline(ss, lengthString, ',');
        std::getline(ss, highwayString, ',');
        std::getline(ss, speedString, ',');
        std::getline(ss, onewayString, ',');
        std::getline(ss, nameString, ',');

        int64_t sourceOsmId = std::stoll(sourceString);
        int64_t targetOsmId = std::stoll(targetString);

        double lengthMetres = std::stod(lengthString);
        float distanceMiles = static_cast<float>(lengthMetres / METRES_PER_MILE);

        float speedLimit = std::stof(speedString);

        bool oneway = parseBool(onewayString);

        std::string name = removeQuotes(nameString);

        std::uint32_t source = network.getNodeIndex(sourceOsmId);
        std::uint32_t target = network.getNodeIndex(targetOsmId);

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

RoadNetwork loadOsmNetwork(const std::string& nodesFile, const std::string& edgesFile) {
    RoadNetwork network;

    loadNodes(network, nodesFile);

    loadEdges(network, edgesFile);

    return network;
}