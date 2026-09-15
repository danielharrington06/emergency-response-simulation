#pragma once

#include "gpu_graph.hpp"

#include <cstdint>

class MetalRouter {

public:
    MetalRouter(const GPUGraph& graph);
    ~MetalRouter();

    float route(
        std::uint32_t sourceNode,
        std::uint32_t targetNode
    );

private:
    struct MetalState;
    MetalState* state;
};