#include "../include/test_network.hpp"
#include "../include/router.hpp"
#include "../include/test_dispatch.hpp"
#include "../include/osm_loader.hpp"

#include <iostream>

int main() {
    std::cout << "Emergency Response Simulation\n";

    RoadNetwork network = loadOsmNetwork(
        "data/processed/nodes.csv",
        "data/processed/edges.csv"
    );
    
    std::cout << "\n-> Loaded road network\n";

    std::cout << "Nodes: "
              << network.nodeCount()
              << '\n';

    std::cout << "Directed edges: "
              << network.edgeCount()
              << '\n';

    return 0;
}