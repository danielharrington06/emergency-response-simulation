#include "../include/metal_router.hpp"

#import <Metal/Metal.h>

#include <iostream>
#include <stdexcept>
#include <limits>
#include <vector>
#include <cstdint>

MetalRouter::MetalRouter(const GPUGraph& graph) {

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();

    if (device == nil) {
        throw std::runtime_error("Failed to create Metal device");
    }

    id<MTLCommandQueue> commandQueue = [device newCommandQueue];

    if (commandQueue == nil) {
        throw std::runtime_error("Failed to create Metal command queue");
    }

    NSURL* libraryURL = [NSURL fileURLWithPath:@"bin/routing.metallib"];

    id<MTLLibrary> library = [device newLibraryWithURL:libraryURL error:nil];

    if (library == nil) {
        throw std::runtime_error("Failed to load routing.metallib");
    }

    id<MTLFunction> function =
        [library newFunctionWithName:@"relax_frontier"];

    if (function == nil) {
        throw std::runtime_error("Failed to find relax_frontier kernel");
    }

    id<MTLComputePipelineState> pipeline =
        [device newComputePipelineStateWithFunction:function error:nil];

    if (pipeline == nil) {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    id<MTLBuffer> nodeOffsetsBuffer =
        [device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    // id<MTLBuffer> degreesBuffer =
    //     [device newBufferWithLength:
    //                 graph.nodeOffsets.size() * sizeof(std::uint32_t)
    //                        options:MTLResourceStorageModeShared];
    
    std::uint32_t sourceNode = 0;

    std::uint32_t frontierData[] = { sourceNode };

    id<MTLBuffer> frontierBuffer =
        [device newBufferWithBytes:frontierData
                            length:sizeof(frontierData)
                        options:MTLResourceStorageModeShared];
    
    const std::size_t nodeCount = graph.nodeOffsets.size() - 1;

    const std::uint32_t INF = std::numeric_limits<std::uint32_t>::max();

    std::vector<uint32_t> initialTravelTimes(nodeCount, INF);

    initialTravelTimes[sourceNode] = 0.0f;

    id<MTLBuffer> travelTimesBuffer =
    [device newBufferWithBytes:initialTravelTimes.data()
                        length:initialTravelTimes.size() * sizeof(uint32_t)
                       options:MTLResourceStorageModeShared];

    std::vector<std::uint32_t> nextTravelTimes(nodeCount, INF);

    id<MTLBuffer> nextTravelTimesBuffer =
        [device newBufferWithBytes:nextTravelTimes.data()
                            length:nextTravelTimes.size() * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeDestinationsBuffer =
        [device newBufferWithBytes:graph.edgeDestinations.data()
                            length:graph.edgeDestinations.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeTravelTimesBuffer =
        [device newBufferWithBytes:graph.edgeTravelTimes.data()
                            length:graph.edgeTravelTimes.size() * sizeof(float)
                          options:MTLResourceStorageModeShared];

    if (frontierBuffer == nil ||
        travelTimesBuffer == nil ||
        nextTravelTimesBuffer == nil) {
        throw std::runtime_error(
            "Failed to create routing buffers"
        );
    }

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

    [encoder setComputePipelineState:pipeline];

    [encoder setBuffer:nodeOffsetsBuffer offset:0 atIndex:0];

    [encoder setBuffer:edgeDestinationsBuffer offset:0 atIndex:1];

    [encoder setBuffer:edgeTravelTimesBuffer offset:0 atIndex:2];

    [encoder setBuffer:frontierBuffer offset:0 atIndex:3];

    [encoder setBuffer:travelTimesBuffer offset:0 atIndex:4];

    [encoder setBuffer:nextTravelTimesBuffer offset:0 atIndex:5];

    [encoder dispatchThreads:
        MTLSizeMake(1, 1, 1)
        threadsPerThreadgroup:
        MTLSizeMake(1, 1, 1)];

    [encoder endEncoding];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    if (commandBuffer.status == MTLCommandBufferStatusError) {
        throw std::runtime_error("Metal command buffer failed");
    }

    const std::uint32_t* results = static_cast<const std::uint32_t*>(nextTravelTimesBuffer.contents);

    const std::uint32_t start = graph.nodeOffsets[sourceNode];
    const std::uint32_t end = graph.nodeOffsets[sourceNode + 1];

    std::cout << "\nGPU edge relaxation from node "
            << sourceNode
            << ":\n";

    for (std::uint32_t edge = start; edge < end; ++edge) {

        const std::uint32_t destination = graph.edgeDestinations[edge];
        float seconds = static_cast<float>(results[destination]) / 1000000.0f; // 1 000 000 was the multiplier

        std::cout << "  -> node "
                << destination
                << ": "
                << seconds
                << " seconds\n";
    }
}

void MetalRouter::testGraph() {
    std::cout << "Metal graph upload successful\n";
}