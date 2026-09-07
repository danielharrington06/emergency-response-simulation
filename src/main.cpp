#include "../include/test_network.hpp"
#include "../include/router.hpp"
#include "../include/test_dispatch.hpp"

#include <iostream>

int main() {
    std::cout << "Emergency Response Simulation\n\n";

    RoadNetwork network = createTestNetwork();
    network.printNetwork();
    std::cout << "\n";

    Dispatch dispatch = createTestDispatch();
    dispatch.printDispatch();

    return 0;
}