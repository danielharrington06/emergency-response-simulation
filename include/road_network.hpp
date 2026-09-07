#pragma once

#include <vector>
#include <cstdint>

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