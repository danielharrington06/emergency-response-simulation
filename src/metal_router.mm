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

    id<MTLBuffer> nodeOffsetsBuffer;
    id<MTLBuffer> edgeDestinationsBuffer;
    id<MTLBuffer> edgeTravelTimesBuffer;

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

        NSURL* libraryURL = [NSURL fileURLWithPath:@"bin/routing.metallib"];

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

        pipeline = [device newComputePipelineStateWithFunction:function error:nil];

        if (pipeline == nil) {
            throw std::runtime_error(
                "Failed to create compute pipeline"
            );
        }

        nodeOffsetsBuffer =
            [device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() *
                                   sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

        edgeDestinationsBuffer =
            [device newBufferWithBytes:graph.edgeDestinations.data()
                            length:graph.edgeDestinations.size() *
                                   sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

        edgeTravelTimesBuffer =
            [device newBufferWithBytes:graph.edgeTravelTimes.data()
                            length:graph.edgeTravelTimes.size() *
                                   sizeof(float)
                           options:MTLResourceStorageModeShared];

        if (nodeOffsetsBuffer == nil ||
            edgeDestinationsBuffer == nil ||
            edgeTravelTimesBuffer == nil) {

            throw std::runtime_error(
                "Failed to upload graph to Metal"
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

    const std::uint32_t INF = std::numeric_limits<std::uint32_t>::max();

    id<MTLBuffer> frontierBuffer =
        [state->device newBufferWithLength:nodeCount * sizeof(std::uint32_t)
                        options:MTLResourceStorageModeShared];

    auto* frontierData = static_cast<std::uint32_t*>(frontierBuffer.contents);

    frontierData[0] = sourceNode;

    id<MTLBuffer> nextFrontierBuffer =
        [state->device newBufferWithLength:nodeCount * sizeof(std::uint32_t)
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

    std::uint32_t initialMinTime = INF;

    id<MTLBuffer> nextFrontierMinTimeBuffer =
        [state->device newBufferWithBytes:&initialMinTime
                        length:sizeof(std::uint32_t)
                    options:MTLResourceStorageModeShared];

    if (frontierBuffer == nil ||
        nextFrontierBuffer == nil ||
        travelTimesBuffer == nil ||
        improvedBuffer == nil ||
        nextFrontierCountBuffer == nil ||
        nextFrontierMinTimeBuffer == nil) {
        throw std::runtime_error(
            "Failed to create routing buffers"
        );
    }
    
    std::size_t frontierCount = 1;

    std::size_t iterationCount = 0;

    auto start = std::chrono::steady_clock::now();

    while (frontierCount > 0) {
        ++iterationCount;

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

        std::memcpy(
            nextFrontierMinTimeBuffer.contents,
            &INF,
            sizeof(std::uint32_t)
        );

        id<MTLCommandBuffer> commandBuffer = [state->commandQueue commandBuffer];

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:state->pipeline];

        [encoder setBuffer:state->nodeOffsetsBuffer offset:0 atIndex:0];
        [encoder setBuffer:state->edgeDestinationsBuffer offset:0 atIndex:1];
        [encoder setBuffer:state->edgeTravelTimesBuffer offset:0 atIndex:2];
        [encoder setBuffer:frontierBuffer offset:0 atIndex:3];
        [encoder setBuffer:travelTimesBuffer offset:0 atIndex:4];
        [encoder setBuffer:improvedBuffer offset:0 atIndex:5];
        [encoder setBuffer:nextFrontierBuffer offset:0 atIndex:6];
        [encoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:7];
        [encoder setBuffer:nextFrontierMinTimeBuffer offset:0 atIndex:8];

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

        const std::uint32_t* minTimeResult = static_cast<const std::uint32_t*>(nextFrontierMinTimeBuffer.contents);

        const std::uint32_t targetTime = static_cast<const std::uint32_t*>(travelTimesBuffer.contents)[targetNode];

        if (frontierCount == 0 || targetTime <= *minTimeResult) {
            break;
        }

        std::swap(frontierBuffer, nextFrontierBuffer);
    }

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    const std::uint32_t* results = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);

    return static_cast<float>(results[targetNode]) / (1000.0f*60.0f);
}