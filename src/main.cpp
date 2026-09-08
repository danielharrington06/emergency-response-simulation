#include "../include/test_network.hpp"
#include "../include/router.hpp"
#include "../include/test_dispatch.hpp"

#include <iostream>

int main() {
    std::cout << "Emergency Response Simulation\n";

    RoadNetwork network = createTestNetwork();
    //network.printNetwork();

    Dispatch dispatch = createTestDispatch();
    //dispatch.printDispatch();

    std::cout << "\n-> Calculating Optimal Vehicle-Incident Assignments...\n";
    dispatch.findOptimalAssignment();

    dispatch.printAssignments();

    return 0;
}