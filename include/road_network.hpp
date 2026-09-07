#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>

struct RoadNode {
    int64_t id; // external node id
    double latitude;
    double longitude;
};

struct RoadEdge {
    std::uint32_t destination; // internal node index
    float distance;
    float speedLimit;
};

class RoadNetwork {

private:
    std::vector<RoadNode> nodes;
    std::vector<std::vector<RoadEdge>> adjacency;
    std::unordered_map<int64_t, uint32_t> osmToIndex;

public:
    uint32_t addNode(int64_t osmId, double latitude, double longitude);

    void addEdge(uint32_t source, const RoadEdge& edge);

    const RoadNode& getNode(uint32_t index) const;

    const std::vector<RoadNode>& getNodes() const;

    const std::vector<RoadEdge>& getNeighbours(uint32_t index) const;

    uint32_t getNodeIndex(int64_t osmId) const;

    bool containsNode(int64_t osmId) const;

    size_t nodeCount() const;

    size_t edgeCount() const;

    void printNetwork() const;
};