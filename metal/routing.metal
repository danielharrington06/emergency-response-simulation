#include <metal_stdlib>

using namespace metal;

// for the node on the frontier, relaxes all outgoing edges
// has to use integer travel times, which are 1,000,000 times the actual float time to have atomic_fetch_min_explicit work
kernel void relax_frontier(
    device const uint* nodeOffsets [[buffer(0)]],
    device const uint* edgeDestinations [[buffer(1)]],
    device const float* edgeTravelTimes [[buffer(2)]],

    device const uint* frontier [[buffer(3)]],

    device atomic_uint* travelTimes [[buffer(4)]],
    device atomic_uint* improved [[buffer(5)]],

    device uint* nextFrontier [[buffer(6)]],
    device atomic_uint* nextFrontierCount [[buffer(7)]],
    device atomic_uint* nextFrontierMinTime [[buffer(8)]],

    uint frontierIndex [[thread_position_in_grid]]
) {
    uint node = frontier[frontierIndex];

    uint start = nodeOffsets[node];
    uint end = nodeOffsets[node + 1];

    uint currentTime = atomic_load_explicit(&travelTimes[node], memory_order_relaxed);

    for (uint edge = start; edge < end; ++edge) {

        uint destination = edgeDestinations[edge];
        uint edgeTime = uint(edgeTravelTimes[edge] * 1000.0f + 0.5f); // +0.5f causes rounding behaviour

        uint newTime = currentTime + edgeTime;
        uint oldTime = atomic_fetch_min_explicit(&travelTimes[destination], newTime, memory_order_relaxed);

        if (newTime < oldTime) {
            uint alreadyImproved = atomic_exchange_explicit(&improved[destination], 1, memory_order_relaxed);

            if (alreadyImproved == 0) {
                uint index = atomic_fetch_add_explicit(nextFrontierCount, 1, memory_order_relaxed);
                nextFrontier[index] = destination;
                atomic_fetch_min_explicit(nextFrontierMinTime, newTime, memory_order_relaxed);
            }
        }
    }
}

// resets the improved flags more efficiently
kernel void reset_improved(
    device const uint* frontier [[buffer(0)]],
    device atomic_uint* improved [[buffer(1)]],

    uint frontierIndex [[thread_position_in_grid]]
) {
    uint node = frontier[frontierIndex];

    atomic_store_explicit(&improved[node], 0, memory_order_relaxed);
}