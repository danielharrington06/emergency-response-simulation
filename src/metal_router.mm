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

    std::vector<std::uint32_t> frontier = {sourceNode};

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

    auto start = std::chrono::steady_clock::now();

    while (!frontier.empty()) {

        id<MTLBuffer> frontierBuffer =
        [state->device newBufferWithBytes:frontier.data()
                            length:frontier.size() *
                                   sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

        if (frontierBuffer == nil ||
            travelTimesBuffer == nil ||
            improvedBuffer == nil) {
            throw std::runtime_error(
                "Failed to create routing buffers"
            );
        }

        id<MTLCommandBuffer> commandBuffer = [state->commandQueue commandBuffer];

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:state->pipeline];

        [encoder setBuffer:nodeOffsetsBuffer offset:0 atIndex:0];

        [encoder setBuffer:edgeDestinationsBuffer offset:0 atIndex:1];
        [encoder setBuffer:edgeTravelTimesBuffer offset:0 atIndex:2];
        [encoder setBuffer:frontierBuffer offset:0 atIndex:3];
        [encoder setBuffer:travelTimesBuffer offset:0 atIndex:4];
        [encoder setBuffer:improvedBuffer offset:0 atIndex:5];

        const NSUInteger threadgroupSize = std::min(static_cast<NSUInteger>(frontier.size()), state->pipeline.maxTotalThreadsPerThreadgroup );

        [encoder dispatchThreads:
            MTLSizeMake(frontier.size(), 1, 1)
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

        const std::uint32_t* improvedResults = static_cast<const std::uint32_t*>(improvedBuffer.contents);

        std::vector<std::uint32_t> nextFrontier;

        for (std::uint32_t node = 0; node < nodeCount; ++node) {
            if (improvedResults[node] != 0) {
                nextFrontier.push_back(node);
            }
        }

        std::memset(
            improvedBuffer.contents,
            0,
            improved.size() * sizeof(std::uint32_t)
        );

        frontier = std::move(nextFrontier);
    }

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << std::fixed << std::setprecision(3)
            << "Simulation time: "
            << elapsed.count()
            << " ms\n";

    const std::uint32_t* results = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);

    return static_cast<float>(results[targetNode]) / 1000.0f;
}