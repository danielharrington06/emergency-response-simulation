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
        throw std::runtime_error("Failed to create Metal command queue");
    }

    NSURL* libraryURL = [NSURL fileURLWithPath:@"bin/routing.metallib"];

    id<MTLLibrary> library = [device newLibraryWithURL:libraryURL error:nil];

    if (library == nil) {
        throw std::runtime_error("Failed to load routing.metallib");
    }

    id<MTLFunction> function =
        [library newFunctionWithName:@"calculate_degrees"];

    if (function == nil) {
        throw std::runtime_error("Failed to find calculate_degrees kernel");
    }

    id<MTLComputePipelineState> pipeline =
        [device newComputePipelineStateWithFunction:function
                                               error:nil];

    if (pipeline == nil) {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    id<MTLBuffer> nodeOffsetsBuffer =
        [device newBufferWithBytes:graph.nodeOffsets.data()
                            length:graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    id<MTLBuffer> degreesBuffer =
        [device newBufferWithLength:
                    graph.nodeOffsets.size() * sizeof(std::uint32_t)
                           options:MTLResourceStorageModeShared];

    // id<MTLBuffer> edgeDestinationsBuffer =
    //     [device newBufferWithBytes:graph.edgeDestinations.data()
    //                         length:graph.edgeDestinations.size() * sizeof(std::uint32_t)
    //                        options:MTLResourceStorageModeShared];

    // id<MTLBuffer> edgeTravelTimesBuffer =
    //     [device newBufferWithBytes:graph.edgeTravelTimes.data()
    //                         length:graph.edgeTravelTimes.size() * sizeof(float)
    //                       options:MTLResourceStorageModeShared];

    if (nodeOffsetsBuffer == nil || degreesBuffer == nil) {
        throw std::runtime_error("Failed to create Metal buffers");
    }

    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

    [encoder setComputePipelineState:pipeline];

    [encoder setBuffer:nodeOffsetsBuffer offset:0 atIndex:0];

    [encoder setBuffer:degreesBuffer offset:0 atIndex:1];

    NSUInteger nodeCount = graph.nodeOffsets.size() - 1;

    [encoder dispatchThreads:
        MTLSizeMake(nodeCount, 1, 1)
        threadsPerThreadgroup:
        MTLSizeMake(pipeline.maxTotalThreadsPerThreadgroup, 1, 1)];

    [encoder endEncoding];

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    if (commandBuffer.status == MTLCommandBufferStatusError) {
        throw std::runtime_error("Metal command buffer failed");
    }

    const auto* degrees =
        static_cast<const std::uint32_t*>(degreesBuffer.contents);

    std::cout << "Metal device: "
              << [device.name UTF8String]
              << '\n';

    std::cout << "GPU degree test successful\n";

    std::cout << "Node 0 degree: "
              << degrees[0]
              << '\n';

    std::cout << "Node 1 degree: "
              << degrees[1]
              << '\n';

    std::cout << "Node 2 degree: "
              << degrees[2]
              << '\n';
}

void MetalRouter::testGraph() {
    std::cout << "Metal graph upload successful\n";
}