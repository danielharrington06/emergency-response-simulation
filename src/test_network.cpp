#include "../include/test_network.hpp"

RoadNetwork createTestNetwork() {

    RoadNetwork network;

    // Nodes
    uint32_t node0 = network.addNode(1000, 0.0, 2.0);
    uint32_t node1 = network.addNode(1001, 1.5, 4.0);
    uint32_t node2 = network.addNode(1002, 4.0, 4.2);
    uint32_t node3 = network.addNode(1003, 5.5, 2.0);
    uint32_t node4 = network.addNode(1004, 1.7, 0.2);
    uint32_t node5 = network.addNode(1005, 4.2, 0.0);
    uint32_t node6 = network.addNode(1006, 100.0, 0.0);

    // 0 <-> 1
    network.addEdge(node0, RoadEdge{
        .destination = node1,
        .distance = 2.7,
        .speedLimit = 30
    });

    network.addEdge(node1, RoadEdge{
        .destination = node0,
        .distance = 2.7,
        .speedLimit = 30
    });

    // 1 <-> 2
    network.addEdge(node1, RoadEdge{
        .destination = node2,
        .distance = 2.5,
        .speedLimit = 40
    });

    network.addEdge(node2, RoadEdge{
        .destination = node1,
        .distance = 2.5,
        .speedLimit = 40
    });

    // 2 <-> 3
    network.addEdge(node2, RoadEdge{
        .destination = node3,
        .distance = 2.8,
        .speedLimit = 30
    });

    network.addEdge(node3, RoadEdge{
        .destination = node2,
        .distance = 2.8,
        .speedLimit = 30
    });

    // 0 <-> 4
    network.addEdge(node0, RoadEdge{
        .destination = node4,
        .distance = 2.6,
        .speedLimit = 30
    });

    network.addEdge(node4, RoadEdge{
        .destination = node0,
        .distance = 2.6,
        .speedLimit = 30
    });

    // 4 <-> 5
    network.addEdge(node4, RoadEdge{
        .destination = node5,
        .distance = 2.5,
        .speedLimit = 30
    });

    network.addEdge(node5, RoadEdge{
        .destination = node4,
        .distance = 2.5,
        .speedLimit = 30
    });

    // 5 <-> 3
    network.addEdge(node5, RoadEdge{
        .destination = node3,
        .distance = 2.6,
        .speedLimit = 40
    });

    network.addEdge(node3, RoadEdge{
        .destination = node5,
        .distance = 2.6,
        .speedLimit = 40
    });

    // 1 <-> 4
    network.addEdge(node1, RoadEdge{
        .destination = node4,
        .distance = 3.9,
        .speedLimit = 30
    });

    network.addEdge(node4, RoadEdge{
        .destination = node1,
        .distance = 3.9,
        .speedLimit = 30
    });

    // 2 <-> 5
    network.addEdge(node2, RoadEdge{
        .destination = node5,
        .distance = 4.2,
        .speedLimit = 30
    });

    network.addEdge(node5, RoadEdge{
        .destination = node2,
        .distance = 4.2,
        .speedLimit = 30
    });

    return network;
}