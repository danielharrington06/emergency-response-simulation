#include <metal_stdlib>

using namespace metal;

kernel void calculate_degrees(
    device const uint* nodeOffsets [[buffer(0)]],
    device uint* degrees [[buffer(1)]],
    uint nodeIndex [[thread_position_in_grid]]
) {
    degrees[nodeIndex] = nodeOffsets[nodeIndex + 1] - nodeOffsets[nodeIndex];
}