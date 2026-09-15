#include "../include/gpu_graph.hpp"

#include <iostream>

// need to convert RoadNetwork into a format that is preferred by the GPU, otherwise it will be much slower to work with

GPUGraph createGPUGraph(const RoadNetwork& network) {
    GPUGraph graph;

    const std::size_t nodeCount = network.nodeCount();
    const std::size_t edgeCount = network.edgeCount();

    graph.nodeOffsets.reserve(nodeCount + 1);
    graph.edgeDestinations.reserve(edgeCount);
    graph.edgeTravelTimes.reserve(edgeCount);
    graph.nodeLatitudes.reserve(nodeCount);
    graph.nodeLongitudes.reserve(nodeCount);

    // The first edge starts at index 0.
    graph.nodeOffsets.push_back(0);

    for (std::uint32_t nodeIndex = 0;
         nodeIndex < nodeCount;
         ++nodeIndex) {

        const RoadNode& node = network.getNode(nodeIndex);

        graph.nodeLatitudes.push_back(
            static_cast<float>(node.latitude)
        );

        graph.nodeLongitudes.push_back(
            static_cast<float>(node.longitude)
        );

        const auto& neighbours = network.getNeighbours(nodeIndex);

        for (const RoadEdge& edge : neighbours) {
            graph.edgeDestinations.push_back(edge.destination);

            if (edge.speedLimit <= 0.0f) {
                throw std::runtime_error(
                    "Road edge has invalid speed limit"
                );
            }

            const float travelTime = edge.distance / edge.speedLimit * 3600;

            graph.edgeTravelTimes.push_back(travelTime);
        }

        // The next node's edges begin here.
        graph.nodeOffsets.push_back(
            static_cast<std::uint32_t>(graph.edgeDestinations.size())
        );
    }

    return graph;
}

void testGPUGraph(const RoadNetwork& network) {
    GPUGraph graph = createGPUGraph(network);

    std::cout << "\nGPU graph:\n";
    std::cout << "Nodes: " << graph.nodeLatitudes.size() << '\n';
    std::cout << "Edges: " << graph.edgeDestinations.size() << '\n';

    std::cout << "Node offsets: "
              << graph.nodeOffsets.size() << '\n';

    std::cout << "First few edges:\n";

    for (std::size_t i = 0;
         i < std::min<std::size_t>(10, graph.edgeDestinations.size());
         ++i) {

        std::cout << "  "
                  << i
                  << " -> "
                  << graph.edgeDestinations[i]
                  << ", "
                  << graph.edgeTravelTimes[i]
                  << " s\n";
    }
}