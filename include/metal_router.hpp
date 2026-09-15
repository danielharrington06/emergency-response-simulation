#pragma once

#include "gpu_graph.hpp"

class MetalRouter {
public:
    MetalRouter(const GPUGraph& graph);

    void testGraph();
};