#pragma once

#include "gpu_graph.hpp"

#include <cstdint>

class MetalRouter {
public:
    MetalRouter(const GPUGraph& graph);

    void testGraph();
};