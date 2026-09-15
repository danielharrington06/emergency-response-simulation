#include "../include/metal_router.hpp"

#import <Metal/Metal.h>

#include <iostream>
#include <stdexcept>

MetalRouter::MetalRouter(const GPUGraph& graph) {

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();

    if (device == nil) {
        throw std::runtime_error("Failed to create Metal device");
    }

    id<MTLCommandQueue> commandQueue = [device newCommandQueue];

    if (commandQueue == nil) {
        throw std::runtime_error("Failed to create command queue");
    }

    id<MTLBuffer> nodeOffsetsBuffer =
        [device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeDestinationsBuffer =
        [device newBufferWithBytes:graph.edgeDestinations.data()
                            length:graph.edgeDestinations.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeTravelTimesBuffer =
        [device newBufferWithBytes:graph.edgeTravelTimes.data()
                            length:graph.edgeTravelTimes.size() * sizeof(float)
                           options:MTLResourceStorageModeShared];

    if (nodeOffsetsBuffer == nil ||
        edgeDestinationsBuffer == nil ||
        edgeTravelTimesBuffer == nil) {
        throw std::runtime_error("Failed to create Metal graph buffers");
    }

    std::cout << "Metal device: " << [device.name UTF8String] << '\n';

    std::cout << "Uploaded graph:\n";

    std::cout << "Nodes: " << graph.nodeOffsets.size() - 1 << '\n';

    std::cout << "Edges: " << graph.edgeDestinations.size() << '\n';
}

void MetalRouter::testGraph() {
    std::cout << "Metal graph upload successful\n";
}