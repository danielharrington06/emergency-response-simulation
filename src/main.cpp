#include "../include/test_network.hpp"
#include "../include/router.hpp"
#include "../include/test_dispatch.hpp"

#include <iostream>

int main() {
    std::cout << "Emergency Response Simulation\n\n";

    RoadNetwork network = createTestNetwork();
    std::cout << "Nodes: " << network.nodeCount() << '\n';
    std::cout << "Directed edges: " << network.edgeCount() << "\n\n";
    network.printNetwork();
    std::cout << "\n";

    Dispatch dispatch = createTestDispatch();
    std::cout << "Vehicles: " << dispatch.vehicleCount() << '\n';
    std::cout << "Incident: " << dispatch.incidentCount() << '\n';
    dispatch.printDispatch();

    return 0;
}