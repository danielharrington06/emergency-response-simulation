#include "../include/test_network.hpp"
#include "../include/router.hpp"
#include "../include/test_dispatch.hpp"

#include <iostream>

int main() {
    std::cout << "Emergency Response Simulation\n";

    RoadNetwork network = createTestNetwork();

    Dispatch dispatch = createTestDispatchEqual(network);

    std::cout << "\n-> Calculating Optimal Vehicle-Incident Assignments...\n";
    dispatch.findOptimalAssignment();

    dispatch.printVehicles();
    dispatch.printIncidents();

    return 0;
}