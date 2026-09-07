#include "../include/test_network.hpp"

#include <iostream>

int main() {
    RoadNetwork network = createTestNetwork();

    std::cout << "Nodes: " << network.nodeCount() << '\n';
    std::cout << "Directed edges: " << network.edgeCount() << "\n\n";
    network.printNetwork();

    return 0;
}