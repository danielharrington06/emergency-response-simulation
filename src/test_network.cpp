#include "../include/test_network.hpp"

RoadNetwork createTestNetwork() {

    RoadNetwork network;

    uint32_t node0 = network.addNode(1000, 0.0, 1.0);
    uint32_t node1 = network.addNode(1001, 1.0, 2.0);
    uint32_t node2 = network.addNode(1002, 2.0, 2.0);
    uint32_t node3 = network.addNode(1003, 3.0, 1.0);
    uint32_t node4 = network.addNode(1004, 1.0, 0.0);
    uint32_t node5 = network.addNode(1005, 2.0, 0.0);

    network.addEdge(node0, RoadEdge{
        .destination = node1,
        .distance = 1.0,
        .speedLimit = 30
    });

    network.addEdge(node1, RoadEdge{
        .destination = node0,
        .distance = 1.0,
        .speedLimit = 30
    });

    return network;
}