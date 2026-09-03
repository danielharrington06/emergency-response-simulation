#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>

#include "../include/road_network.hpp"

class RoadNetwork {
private:
    std::vector<RoadNode> nodes;
    std::vector<std::vector<RoadEdge>> adjacency;

    std::unordered_map<int64_t, uint32_t> osmToIndex;

public:
    // construction

    void addNode(int64_t osmId, double latitude, double longitude) {
        RoadNode node {
            .id = osmId,
            .latitude = latitude,
            .longitude = longitude
        };
        nodes.push_back(node);
    }

    void addEdge(uint32_t source, const RoadEdge& edge) {
        adjacency.at(source).push_back(edge);
    }

    // lookup

    const RoadNode& getNode(uint32_t index) const {
        return nodes.at(index);
    }

    const std::vector<RoadNode> getNeighbours(uint32_t index) {
        std::vector<RoadNode> neighbours;

        for (RoadEdge edge : adjacency.at(index)) {
            neighbours.push_back(getNode(edge.destination));
        }

        return neighbours;
    }

    // OSM ID <-> internal index

    uint32_t getNodeIndex(int64_t osmId) const {
        try {
            return osmToIndex.at(osmId);
        }
        catch (int error) {
            return NULL;
        }
    }

    bool containsNode(int64_t osmId) const {
        return osmToIndex.find(osmId) == osmToIndex.end();
    }

    // network information

    size_t nodeCount() const {
        return nodes.size();
    }

    size_t edgeCount() const {
        size_t count = 0;
        for (std::vector<RoadEdge> edges : adjacency) {
            count += edges.size();
        }

        return count;
    }

    // debugging

    void printNetwork() const {
        for (int i = 0; i < nodes.size(); i++) {
            std::cout << "Node " << i << ":\n";
            std::cout << "\tLatitude: " << nodes.at(i).latitude << '\n';
            std::cout << "\tLongitude: " << nodes.at(i).longitude << '\n';
            std::cout << "\tEdges:";
            for (RoadEdge edge : adjacency.at(i)) {
                std::cout << "\t\tDestination: Node " << edge.destination << "\t, Distance: " << edge.distance << "units\n";
            }
            std::cout << '\n';
        }
    }
};