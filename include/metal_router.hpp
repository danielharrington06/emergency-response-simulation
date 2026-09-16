#pragma once

#include "gpu_graph.hpp"
#include "route.hpp"

#include <cstdint>

class MetalRouter {

public:
    MetalRouter(const GPUGraph& graph);
    ~MetalRouter();

    Route findRouteGPU(std::uint32_t sourceNode, std::uint32_t targetNode);

private:
    struct MetalState;
    MetalState* state;
    double calculateStraightlineDistance(std::uint32_t sourceNode, std::uint32_t targetNode);
};