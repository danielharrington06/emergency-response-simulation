#include "../include/router.hpp"
#include "../include/osm_loader.hpp"
#include "../include/scenario_generator.hpp"
#include "../include/dispatch.hpp"

#include <iostream>
#include <iomanip>

const unsigned int incidentSeed = 12345;
const unsigned int vehicleSeed = 67890;

const unsigned int incidentCount = 3;
const unsigned int vehicleCount = 3;

int main() {

    // setup - not timed

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

    std::vector<Incident> incidents = generateRandomIncidents(network, incidentSeed, incidentSeed);
    std::vector<EmergencyVehicle> vehicles = generateRandomVehicles(network, vehicleCount, vehicleSeed);

    Dispatch dispatch(network);
    dispatch.addVehicles(vehicles);
    dispatch.addIncidents(incidents);

    std::cout << "\n-> Running Simulation on CPU\n";

    // benchmark - timed

    auto start = std::chrono::steady_clock::now();

    //dispatch.findOptimalAssignment();
    Route route = findRoute(network, 1, 271400);
    std::cout << "Nodes visited: "<< route.nodesVisited << '\n';

    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << std::fixed << std::setprecision(3)
            << "Simulation time: "
            << elapsed.count()
            << " ms\n";

    return 0;
}