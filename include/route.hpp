#pragma once

#include <cstdint>
#include <vector>

struct Route {
    std::vector<uint32_t> nodes;

    double totalDistance = 0.0;
    double totalTravelTime = 0.0;

    uint32_t nodesVisited = 0;

    bool found = false;
};