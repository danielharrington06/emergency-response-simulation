#include "../include/metal_router.hpp"

#import <Metal/Metal.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <iomanip>

struct MetalRouter::MetalState {

    GPUGraph graph;

    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLComputePipelineState> pipeline;

    MetalState(const GPUGraph& graph) : graph(graph) {

        device = MTLCreateSystemDefaultDevice();

        if (device == nil) {
            throw std::runtime_error(
                "Failed to create Metal device"
            );
        }

        commandQueue = [device newCommandQueue];

        if (commandQueue == nil) {
            throw std::runtime_error(
                "Failed to create Metal command queue"
            );
        }

        NSURL* libraryURL =
            [NSURL fileURLWithPath:@"bin/routing.metallib"];

        id<MTLLibrary> library = [device newLibraryWithURL:libraryURL error:nil];

        if (library == nil) {
            throw std::runtime_error(
                "Failed to load routing.metallib"
            );
        }

        id<MTLFunction> function = [library newFunctionWithName:@"relax_frontier"];

        if (function == nil) {
            throw std::runtime_error(
                "Failed to find relax_frontier kernel"
            );
        }

        pipeline = [device newComputePipelineStateWithFunction:function
                                                   error:nil];

        if (pipeline == nil) {
            throw std::runtime_error(
                "Failed to create compute pipeline"
            );
        }
    }
};

MetalRouter::MetalRouter(const GPUGraph& graph)
    : state(new MetalState(graph)) {
}

MetalRouter::~MetalRouter() {
    delete state;
}

float MetalRouter::route(std::uint32_t sourceNode, std::uint32_t targetNode) {
    const GPUGraph& graph = state->graph;

    const std::size_t nodeCount =
        graph.nodeOffsets.size() - 1;

    if (sourceNode >= nodeCount ||
        targetNode >= nodeCount) {

        throw std::out_of_range(
            "Source or target node is out of range"
        );
    }

    id<MTLBuffer> nodeOffsetsBuffer =
        [state->device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    const std::uint32_t INF = std::numeric_limits<std::uint32_t>::max();

    id<MTLBuffer> edgeDestinationsBuffer =
        [state->device newBufferWithBytes:graph.edgeDestinations.data()
                            length:graph.edgeDestinations.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> edgeTravelTimesBuffer =
        [state->device newBufferWithBytes:graph.edgeTravelTimes.data()
                            length:graph.edgeTravelTimes.size() * sizeof(float)
                          options:MTLResourceStorageModeShared];

    std::vector<std::uint32_t> initialFrontier = {sourceNode};
    id<MTLBuffer> frontierBuffer =
        [state->device
            newBufferWithBytes:initialFrontier.data()
                        length:nodeCount * sizeof(std::uint32_t)
                    options:MTLResourceStorageModeShared];

    id<MTLBuffer> nextFrontierBuffer =
        [state->device
            newBufferWithLength:nodeCount * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];

    std::vector<std::uint32_t> initialTravelTimes(nodeCount, INF);

    initialTravelTimes[sourceNode] = 0;

    id<MTLBuffer> travelTimesBuffer =
    [state->device newBufferWithBytes:initialTravelTimes.data()
                        length:initialTravelTimes.size() * sizeof(std::uint32_t)
                       options:MTLResourceStorageModeShared];
    
    std::vector<std::uint32_t> improved(
        nodeCount,
        0
    );
    
    id<MTLBuffer> improvedBuffer =
        [state->device newBufferWithBytes:improved.data()
                            length:improved.size() * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];
    
    std::uint32_t initialFrontierCount = 0;

    id<MTLBuffer> nextFrontierCountBuffer =
        [state->device
            newBufferWithBytes:&initialFrontierCount
                        length:sizeof(std::uint32_t)
                    options:MTLResourceStorageModeShared];

    if (frontierBuffer == nil ||
        travelTimesBuffer == nil ||
        improvedBuffer == nil) {
        throw std::runtime_error(
            "Failed to create routing buffers"
        );
    }
    
    std::size_t frontierCount = 1;

    while (frontierCount > 0) {

        std::memset(
            improvedBuffer.contents,
            0,
            improved.size() * sizeof(std::uint32_t)
        );

        std::uint32_t zero = 0;

        std::memcpy(
            nextFrontierCountBuffer.contents,
            &zero,
            sizeof(std::uint32_t)
        );

        id<MTLCommandBuffer> commandBuffer = [state->commandQueue commandBuffer];

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:state->pipeline];

        [encoder setBuffer:nodeOffsetsBuffer offset:0 atIndex:0];
        [encoder setBuffer:edgeDestinationsBuffer offset:0 atIndex:1];
        [encoder setBuffer:edgeTravelTimesBuffer offset:0 atIndex:2];
        [encoder setBuffer:frontierBuffer offset:0 atIndex:3];
        [encoder setBuffer:travelTimesBuffer offset:0 atIndex:4];
        [encoder setBuffer:improvedBuffer offset:0 atIndex:5];
        [encoder setBuffer:nextFrontierBuffer offset:0 atIndex:6];
        [encoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:7];

        const NSUInteger threadgroupSize =
            std::min(
                static_cast<NSUInteger>(frontierCount),
                state->pipeline.maxTotalThreadsPerThreadgroup
            );

        [encoder dispatchThreads:
            MTLSizeMake(frontierCount, 1, 1)
            threadsPerThreadgroup:
            MTLSizeMake(threadgroupSize, 1, 1)];

        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        if (commandBuffer.status == MTLCommandBufferStatusError) {
            throw std::runtime_error(
                "Metal routing command buffer failed"
            );
        }

        const std::uint32_t* countResult = static_cast<const std::uint32_t*>(nextFrontierCountBuffer.contents);

        frontierCount = *countResult;

        std::swap(frontierBuffer, nextFrontierBuffer);
    }

    const std::uint32_t* results = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);

    return static_cast<float>(results[targetNode]) / 1000.0f;
}