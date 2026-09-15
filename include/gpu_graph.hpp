#pragma once

#include "road_network.hpp"
#include <cstdint>
#include <vector>

struct GPUGraph {
    std::vector<std::uint32_t> nodeOffsets;
    std::vector<std::uint32_t> edgeDestinations;
    std::vector<float> edgeTravelTimes;

    std::vector<float> nodeLatitudes;
    std::vector<float> nodeLongitudes;
};

GPUGraph createGPUGraph(const RoadNetwork& network);
void testGPUGraph(const RoadNetwork& network);