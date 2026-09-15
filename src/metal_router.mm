#include "../include/metal_router.hpp"

#import <Metal/Metal.h>

#include <iostream>
#include <stdexcept>
#include <limits>
#include <vector>
#include <cstdint>
#include <cstring>

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
    
    std::uint32_t sourceNode = 0;
    const std::size_t nodeCount = graph.nodeOffsets.size() - 1;

    id<MTLBuffer> nodeOffsetsBuffer =
        [device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    const std::uint32_t INF = std::numeric_limits<std::uint32_t>::max();

    id<MTLBuffer> edgeDestinationsBuffer =
        [device newBufferWithBytes:graph.edgeDestinations.data()
                            length:graph.edgeDestinations.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeTravelTimesBuffer =
        [device newBufferWithBytes:graph.edgeTravelTimes.data()
                            length:graph.edgeTravelTimes.size() * sizeof(float)
                          options:MTLResourceStorageModeShared];

    std::vector<std::uint32_t> frontier = {sourceNode};

    id<MTLBuffer> frontierBuffer =
        [device newBufferWithBytes:frontier.data()
                            length:frontier.size() * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];

    std::vector<std::uint32_t> initialTravelTimes(nodeCount, INF);

    initialTravelTimes[sourceNode] = 0;

    id<MTLBuffer> travelTimesBuffer =
    [device newBufferWithBytes:initialTravelTimes.data()
                        length:initialTravelTimes.size() * sizeof(std::uint32_t)
                       options:MTLResourceStorageModeShared];
    
    std::vector<std::uint32_t> improved(
        nodeCount,
        0
    );
    
    id<MTLBuffer> improvedBuffer =
        [device newBufferWithBytes:improved.data()
                            length:improved.size() * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];

    if (frontierBuffer == nil ||
        travelTimesBuffer == nil ||
        improvedBuffer == nil) {
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
    [encoder setBuffer:improvedBuffer offset:0 atIndex:5];

    [encoder dispatchThreads:
        MTLSizeMake(frontier.size(), 1, 1)
        threadsPerThreadgroup:
        MTLSizeMake(frontier.size(), 1, 1)];

    [encoder endEncoding];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    if (commandBuffer.status == MTLCommandBufferStatusError) {
        throw std::runtime_error(
            "Metal routing command buffer failed"
        );
    }

    const std::uint32_t* travelTimeResults = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);
    const std::uint32_t* improvedResults = static_cast<const std::uint32_t*>(improvedBuffer.contents);

    std::cout << "\nImproved nodes:\n";

    for (std::uint32_t node = 0;
        node < nodeCount;
        ++node) {

        if (improvedResults[node] != 0) {

            float minutes =
                static_cast<float>(
                    travelTimeResults[node]
                ) / 1000.0f;

            std::cout << "  node "
                    << node
                    << ": "
                    << minutes
                    << " minutes\n";
        }
    }
}

void MetalRouter::testGraph() {
    std::cout << "Metal graph upload successful\n";
}