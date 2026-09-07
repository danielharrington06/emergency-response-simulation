#include "../include/test_network.hpp"
#include "../include/router.hpp"

#include <iostream>

int main() {
    RoadNetwork network = createTestNetwork();

    std::cout << "Nodes: " << network.nodeCount() << '\n';
    std::cout << "Directed edges: " << network.edgeCount() << "\n\n";
    network.printNetwork();

    Route route = findRoute(
        network,
        4,
        7
    );

    route.printRoute();

    return 0;
}