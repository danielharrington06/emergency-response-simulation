#include "../include/router.hpp"
#include "../include/osm_loader.hpp"
#include "../include/scenario_generator.hpp"
#include "../include/dispatch.hpp"

#include <iostream>

const unsigned int incidentSeed = 12345;
const unsigned int vehicleSeed = 67890;

const unsigned int incidentCount = 100;
const unsigned int vehicleCount = 100;

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

    // benchmark - timed

    auto start = std::chrono::steady_clock::now();

    dispatch.findOptimalAssignment();

    auto end = std::chrono::steady_clock::now();

    return 0;
}