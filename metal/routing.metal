#include <metal_stdlib>

using namespace metal;

kernel void calculate_degrees(
    device const uint* nodeOffsets [[buffer(0)]],
    device uint* degrees [[buffer(1)]],
    uint nodeIndex [[thread_position_in_grid]]
) {
    degrees[nodeIndex] = nodeOffsets[nodeIndex + 1] - nodeOffsets[nodeIndex];
}

// for the node on the frontier, relaxes all outgoing edges
// has to use integer travel times, which are 1,000,000 times the actual float time to have atomic_fetch_min_explicit work
kernel void relax_frontier(
    device const uint* nodeOffsets [[buffer(0)]],
    device const uint* edgeDestinations [[buffer(1)]],
    device const float* edgeTravelTimes [[buffer(2)]],
    device const uint* frontier [[buffer(3)]],
    device const uint* travelTimes [[buffer(4)]],
    device atomic_uint* nextTravelTimes [[buffer(5)]],
    uint frontierIndex [[thread_position_in_grid]]
) {
    uint node = frontier[frontierIndex];

    uint start = nodeOffsets[node];
    uint end = nodeOffsets[node + 1];

    uint currentTime = travelTimes[node];

    for (uint edge = start; edge < end; ++edge) {
        uint destination = edgeDestinations[edge];

        uint edgeTime = uint(edgeTravelTimes[edge] * 1000000.0f);

        uint newTime = currentTime + edgeTime;

        atomic_fetch_min_explicit(&nextTravelTimes[destination], newTime, memory_order_relaxed);
    }
}