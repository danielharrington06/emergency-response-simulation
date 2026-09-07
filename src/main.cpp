#include "../include/test_network.hpp"

int main() {
    RoadNetwork network = createTestNetwork();
    network.printNetwork();
    return 0;
}