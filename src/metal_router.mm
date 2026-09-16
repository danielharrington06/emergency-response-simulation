#include "../include/metal_router.hpp"
#include "../include/route.hpp"

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <iomanip>

constexpr std::uint32_t MAX_ITERATIONS = 2500;
constexpr std::uint32_t THREADS_PER_THREADGROUP = 256;

struct MetalRouter::MetalState {

    GPUGraph graph;

    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLComputePipelineState> pipeline;
    id<MTLComputePipelineState> resetPipeline;
    id<MTLComputePipelineState> preparePipeline;
    id<MTLComputePipelineState> routingStateResetPipeline;

    id<MTLBuffer> nodeOffsetsBuffer;
    id<MTLBuffer> edgeDestinationsBuffer;
    id<MTLBuffer> edgeTravelTimesBuffer;
    id<MTLBuffer> dispatchArgumentsBuffer;

    NSUInteger threadsPerThreadgroup;

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

        id<MTLFunction> resetFunction =
            [library newFunctionWithName:@"reset_improved"];

        if (resetFunction == nil) {
            throw std::runtime_error("Failed to find reset_improved kernel");
        }

        resetPipeline =
            [device newComputePipelineStateWithFunction:resetFunction error:nil];

        if (resetPipeline == nil) {
            throw std::runtime_error("Failed to create reset pipeline");
        }

        id<MTLFunction> prepareFunction = [library newFunctionWithName:@"prepare_dispatch"];

        if (prepareFunction == nil) {
            throw std::runtime_error("Failed to find prepare_dispatch kernel");
        }

        preparePipeline = [device newComputePipelineStateWithFunction:prepareFunction error:nil];

        if (preparePipeline == nil) {
            throw std::runtime_error("Failed to create prepare pipeline");
        }

        id<MTLFunction> routingStateResetFunction =
            [library newFunctionWithName:@"reset_routing_state"];

        if (routingStateResetFunction == nil) {
            throw std::runtime_error(
                "Failed to find reset_routing_state kernel"
            );
        }

        routingStateResetPipeline =
            [device newComputePipelineStateWithFunction:
                routingStateResetFunction
                error:nil];

        if (routingStateResetPipeline == nil) {
            throw std::runtime_error(
                "Failed to create routing state reset pipeline"
            );
        }

        threadsPerThreadgroup = 256;

        if (threadsPerThreadgroup > pipeline.maxTotalThreadsPerThreadgroup ||
            threadsPerThreadgroup > resetPipeline.maxTotalThreadsPerThreadgroup) {
            throw std::runtime_error(
                "Metal device does not support the required threadgroup size"
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
        
        dispatchArgumentsBuffer =
            [device newBufferWithLength:
                3 * sizeof(std::uint32_t)
                options:MTLResourceStorageModeShared];

        if (nodeOffsetsBuffer == nil ||
            edgeDestinationsBuffer == nil ||
            edgeTravelTimesBuffer == nil ||
            dispatchArgumentsBuffer == nil) {
            throw std::runtime_error("Failed to create Metal buffers");
        }
    }
};

MetalRouter::MetalRouter(const GPUGraph& graph)
    : state(new MetalState(graph)) {
}

MetalRouter::~MetalRouter() {
    delete state;
}

double MetalRouter::calculateStraightlineDistance(std::uint32_t sourceNode, std::uint32_t targetNode) {
    const GPUGraph& graph = state->graph;
    constexpr double PI = 3.14159265358979323846;
    constexpr double EARTH_RADIUS_MILES = 3958.7613;

    float latitude1 = graph.nodeLatitudes.at(sourceNode) * PI / 180.0;

    double latitude2 = graph.nodeLatitudes.at(targetNode) * PI / 180.0;

    double latitudeDifference = (graph.nodeLatitudes.at(targetNode) - graph.nodeLatitudes.at(sourceNode)) * PI / 180.0;

    double longitudeDifference = (graph.nodeLongitudes.at(targetNode) - graph.nodeLongitudes.at(sourceNode)) * PI / 180.0;

    double a =
        std::sin(latitudeDifference / 2.0)
        * std::sin(latitudeDifference / 2.0)
        +
        std::cos(latitude1)
        * std::cos(latitude2)
        * std::sin(longitudeDifference / 2.0)
        * std::sin(longitudeDifference / 2.0);

    double distance = 2.0 * EARTH_RADIUS_MILES * std::asin(std::sqrt(a));
    return distance;
}

Route MetalRouter::findRouteGPU(std::uint32_t sourceNode, std::uint32_t targetNode) {
    const GPUGraph& graph = state->graph;

    const std::size_t nodeCount = graph.nodeOffsets.size() - 1;

    if (sourceNode >= nodeCount || targetNode >= nodeCount) {

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
    
    std::vector<std::uint32_t> improved(nodeCount, 0);
    
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
    
    std::uint32_t currentFrontierCount = 1;

    id<MTLBuffer> currentFrontierCountBuffer =
        [state->device
            newBufferWithBytes:&currentFrontierCount
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
        nextFrontierMinTimeBuffer == nil ||
        currentFrontierCountBuffer == nil) {
        throw std::runtime_error(
            "Failed to create routing buffers"
        );
    }

    auto* dispatchArguments = static_cast<std::uint32_t*>(state->dispatchArgumentsBuffer.contents);

    dispatchArguments[0] = 1;
    dispatchArguments[1] = 1;
    dispatchArguments[2] = 1;

    id<MTLCommandBuffer> commandBuffer = [state->commandQueue commandBuffer];

    double distance = calculateStraightlineDistance(sourceNode, targetNode);
    std::uint32_t iterations = std::min(MAX_ITERATIONS, static_cast<std::uint32_t>(std::round(50.25 * distance + 708.74)));

    for (std::uint32_t iteration = 0; iteration < iterations; ++iteration) {

        // reset routing state

        id<MTLComputeCommandEncoder> stateResetEncoder = [commandBuffer computeCommandEncoder];

        [stateResetEncoder setComputePipelineState:state->routingStateResetPipeline];

        [stateResetEncoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:0];
        [stateResetEncoder setBuffer:nextFrontierMinTimeBuffer offset:0 atIndex:1];

        const std::uint32_t infValue = INF;

        [stateResetEncoder setBytes:&infValue length:sizeof(infValue) atIndex:2];

        [stateResetEncoder
            dispatchThreads:
                MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:
                MTLSizeMake(1, 1, 1)];

        [stateResetEncoder endEncoding];

        // reset improved flags

        id<MTLComputeCommandEncoder> resetEncoder = [commandBuffer computeCommandEncoder];

        [resetEncoder setComputePipelineState:state->resetPipeline];

        [resetEncoder setBuffer:frontierBuffer offset:0 atIndex:0];
        [resetEncoder setBuffer:improvedBuffer offset:0 atIndex:1];
        [resetEncoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:2];

        [resetEncoder
            dispatchThreadgroupsWithIndirectBuffer:state->dispatchArgumentsBuffer
            indirectBufferOffset:0
            threadsPerThreadgroup:
                MTLSizeMake(state->threadsPerThreadgroup, 1, 1)];

        [resetEncoder endEncoding];

        // relax current frontier

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
        [encoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:9];

        [encoder dispatchThreadgroupsWithIndirectBuffer: state->dispatchArgumentsBuffer
            indirectBufferOffset:0
            threadsPerThreadgroup:
                MTLSizeMake(state->threadsPerThreadgroup, 1, 1)];

        [encoder endEncoding];

        // prepare indirect dispatch for next generation

        id<MTLComputeCommandEncoder> prepareEncoder = [commandBuffer computeCommandEncoder];

        [prepareEncoder setComputePipelineState:state->preparePipeline];

        [prepareEncoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:0];
        [prepareEncoder setBuffer:nextFrontierMinTimeBuffer offset:0 atIndex:1];
        [prepareEncoder setBuffer:travelTimesBuffer offset:0 atIndex:2];
        [prepareEncoder setBuffer:state->dispatchArgumentsBuffer offset:0 atIndex:3];
        [prepareEncoder setBytes:&targetNode length:sizeof(targetNode) atIndex:4];
        const std::uint32_t threadsPerThreadgroup = static_cast<std::uint32_t>(state->threadsPerThreadgroup);
        [prepareEncoder setBytes:&threadsPerThreadgroup length:sizeof(threadsPerThreadgroup) atIndex:5];
        [prepareEncoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:6];


        [prepareEncoder
            dispatchThreads:
                MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:
                MTLSizeMake(1, 1, 1)];

        [prepareEncoder endEncoding];

        // swap frontier buffers

        std::swap(
            frontierBuffer,
            nextFrontierBuffer
        );
    }

    auto start = std::chrono::steady_clock::now();

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    if (commandBuffer.status == MTLCommandBufferStatusError) {
        std::string message = "Metal routing command buffer failed";

        if (commandBuffer.error != nil) {
            message += ": ";
            message +=
                [[commandBuffer.error localizedDescription] UTF8String];
        }

        throw std::runtime_error(message);
    }

    const std::uint32_t* results = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);

    Route route;
    route.start = sourceNode;
    route.destination = targetNode;
    if (results[targetNode] == INF) {
        route.status = RouteStatus::NoRoute;
        route.totalTravelTime = std::numeric_limits<float>::infinity();
    }
    else {
        route.status = RouteStatus::Found;
        route.totalTravelTime = static_cast<float>(results[targetNode]) / (1000.0f*60.0f);
    }

    return route;
}


std::vector<Route> MetalRouter::findRouteGPU(std::uint32_t sourceNode, const std::vector<std::uint32_t>& targetNodes) {
    const GPUGraph& graph = state->graph;

    const std::size_t nodeCount = graph.nodeOffsets.size() - 1;

    if (sourceNode >= nodeCount || targetNode >= nodeCount) {

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
    
    std::vector<std::uint32_t> improved(nodeCount, 0);
    
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
    
    std::uint32_t currentFrontierCount = 1;

    id<MTLBuffer> currentFrontierCountBuffer =
        [state->device
            newBufferWithBytes:&currentFrontierCount
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
        nextFrontierMinTimeBuffer == nil ||
        currentFrontierCountBuffer == nil) {
        throw std::runtime_error(
            "Failed to create routing buffers"
        );
    }

    auto* dispatchArguments = static_cast<std::uint32_t*>(state->dispatchArgumentsBuffer.contents);

    dispatchArguments[0] = 1;
    dispatchArguments[1] = 1;
    dispatchArguments[2] = 1;

    id<MTLCommandBuffer> commandBuffer = [state->commandQueue commandBuffer];

    double distance = calculateStraightlineDistance(sourceNode, targetNode);
    std::uint32_t iterations = std::min(MAX_ITERATIONS, static_cast<std::uint32_t>(std::round(50.25 * distance + 708.74)));

    for (std::uint32_t iteration = 0; iteration < iterations; ++iteration) {

        // reset routing state

        id<MTLComputeCommandEncoder> stateResetEncoder = [commandBuffer computeCommandEncoder];

        [stateResetEncoder setComputePipelineState:state->routingStateResetPipeline];

        [stateResetEncoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:0];
        [stateResetEncoder setBuffer:nextFrontierMinTimeBuffer offset:0 atIndex:1];

        const std::uint32_t infValue = INF;

        [stateResetEncoder setBytes:&infValue length:sizeof(infValue) atIndex:2];

        [stateResetEncoder
            dispatchThreads:
                MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:
                MTLSizeMake(1, 1, 1)];

        [stateResetEncoder endEncoding];

        // reset improved flags

        id<MTLComputeCommandEncoder> resetEncoder = [commandBuffer computeCommandEncoder];

        [resetEncoder setComputePipelineState:state->resetPipeline];

        [resetEncoder setBuffer:frontierBuffer offset:0 atIndex:0];
        [resetEncoder setBuffer:improvedBuffer offset:0 atIndex:1];
        [resetEncoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:2];

        [resetEncoder
            dispatchThreadgroupsWithIndirectBuffer:state->dispatchArgumentsBuffer
            indirectBufferOffset:0
            threadsPerThreadgroup:
                MTLSizeMake(state->threadsPerThreadgroup, 1, 1)];

        [resetEncoder endEncoding];

        // relax current frontier

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
        [encoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:9];

        [encoder dispatchThreadgroupsWithIndirectBuffer: state->dispatchArgumentsBuffer
            indirectBufferOffset:0
            threadsPerThreadgroup:
                MTLSizeMake(state->threadsPerThreadgroup, 1, 1)];

        [encoder endEncoding];

        // prepare indirect dispatch for next generation

        id<MTLComputeCommandEncoder> prepareEncoder = [commandBuffer computeCommandEncoder];

        [prepareEncoder setComputePipelineState:state->preparePipeline];

        [prepareEncoder setBuffer:nextFrontierCountBuffer offset:0 atIndex:0];
        [prepareEncoder setBuffer:nextFrontierMinTimeBuffer offset:0 atIndex:1];
        [prepareEncoder setBuffer:travelTimesBuffer offset:0 atIndex:2];
        [prepareEncoder setBuffer:state->dispatchArgumentsBuffer offset:0 atIndex:3];
        [prepareEncoder setBytes:&targetNode length:sizeof(targetNode) atIndex:4];
        const std::uint32_t threadsPerThreadgroup = static_cast<std::uint32_t>(state->threadsPerThreadgroup);
        [prepareEncoder setBytes:&threadsPerThreadgroup length:sizeof(threadsPerThreadgroup) atIndex:5];
        [prepareEncoder setBuffer:currentFrontierCountBuffer offset:0 atIndex:6];


        [prepareEncoder
            dispatchThreads:
                MTLSizeMake(1, 1, 1)
            threadsPerThreadgroup:
                MTLSizeMake(1, 1, 1)];

        [prepareEncoder endEncoding];

        // swap frontier buffers

        std::swap(
            frontierBuffer,
            nextFrontierBuffer
        );
    }

    auto start = std::chrono::steady_clock::now();

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    if (commandBuffer.status == MTLCommandBufferStatusError) {
        std::string message = "Metal routing command buffer failed";

        if (commandBuffer.error != nil) {
            message += ": ";
            message +=
                [[commandBuffer.error localizedDescription] UTF8String];
        }

        throw std::runtime_error(message);
    }

    const std::uint32_t* results = static_cast<const std::uint32_t*>(travelTimesBuffer.contents);

    Route route;
    route.start = sourceNode;
    route.destination = targetNode;
    if (results[targetNode] == INF) {
        route.status = RouteStatus::NoRoute;
        route.totalTravelTime = std::numeric_limits<float>::infinity();
    }
    else {
        route.status = RouteStatus::Found;
        route.totalTravelTime = static_cast<float>(results[targetNode]) / (1000.0f*60.0f);
    }

    return route;
}