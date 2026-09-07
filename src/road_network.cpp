#include "../include/road_network.hpp"

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <optional>

class RoadNetwork {
private:
    std::vector<RoadNode> nodes;
    std::vector<std::vector<RoadEdge>> adjacency;

    std::unordered_map<int64_t, uint32_t> osmToIndex;

public:
    // construction

    uint32_t addNode(int64_t osmId, double latitude, double longitude) {
        RoadNode node {
            .id = osmId,
            .latitude = latitude,
            .longitude = longitude
        };

        uint32_t index = nodes.size();
        nodes.push_back(node);
        osmToIndex[osmId] = index;
        adjacency.emplace_back();

        return index;
    }

    void addEdge(uint32_t source, const RoadEdge& edge) {
        adjacency.at(source).push_back(edge);
    }

    // lookup

    const RoadNode& getNode(uint32_t index) const {
        return nodes.at(index);
    }

    const std::vector<RoadNode>& getNodes() const {
        return nodes;
    }

    const std::vector<RoadEdge>& getNeighbours(uint32_t index) const {
        return adjacency.at(index);
    }

    // OSM ID to internal index

    uint32_t  getNodeIndex(int64_t osmId) const { // would be nice to change this to optional
        return osmToIndex.at(osmId);
    }

    bool containsNode(int64_t osmId) const {
        return osmToIndex.find(osmId) != osmToIndex.end();
    }

    // network information

    size_t nodeCount() const {
        return nodes.size();
    }

    // returns the number of directed edges (so a two-way road is double counted)
    size_t edgeCount() const { 
        size_t count = 0;
        for (const std::vector<RoadEdge>& edges : adjacency) {
            count += edges.size();
        }

        return count;
    }

    // debugging

    void printNetwork() const {
        for (size_t i = 0; i < nodes.size(); i++) {
            std::cout << "Node " << i << ":\n";
            std::cout << "\tLatitude: " << nodes.at(i).latitude << '\n';
            std::cout << "\tLongitude: " << nodes.at(i).longitude << '\n';
            std::cout << "\tEdges:\n";
            for (const RoadEdge& edge : adjacency.at(i)) {
                std::cout << "\t\tDestination: Node " << edge.destination 
                << "\t, Distance: " << edge.distance << "units" 
                << "\t, Speed Limit: " << edge.speedLimit << "units per hour\n";
            }
            std::cout << '\n';
        }
    }
};