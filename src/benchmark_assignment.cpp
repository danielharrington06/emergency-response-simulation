#include "../include/benchmark_route.hpp"

#include "../include/road_network.hpp"
#include "../include/osm_loader.hpp"
#include "../include/gpu_graph.hpp"
#include "../include/metal_router.hpp"
#include "../include/route.hpp"
#include "../include/router.hpp"
#include "../include/dispatch.hpp"
#include "../include/scenario_generator.hpp"

#include <iostream>
#include <random>
#include <iomanip>
#include <vector>
#include <chrono>

const unsigned int VEHICLE_SEED = 54321;
const unsigned int INCIDENT_SEED = 72849;
const unsigned int DEFAULT_INCIDENT_COUNT = 10;
const unsigned int DEFAULT_VEHICLE_COUNT = 10;

int runAssignmentBenchmarkPTP(int argc, char *argv[]) {

    unsigned int vehicleCount = DEFAULT_INCIDENT_COUNT;
    unsigned int incidentCount = DEFAULT_VEHICLE_COUNT;

    if (argc == 2) {
        vehicleCount = std::stoul(argv[1]);
        incidentCount = std::stoul(argv[1]);
    }
    else if (argc == 3) {
        vehicleCount = std::stoul(argv[1]);
        incidentCount = std::stoul(argv[2]);
    }

    // setup - not timed
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

    // Generate reproducible random routes

    std::uniform_int_distribution<unsigned int> nodeDistribution(
        0,
        static_cast<unsigned int>(network.nodeCount() - 1)
    );

    std::vector<EmergencyVehicle> vehicles = generateRandomVehicles(network, vehicleCount, VEHICLE_SEED);
    std::vector<Incident> incidents = generateRandomIncidents(network, incidentCount, INCIDENT_SEED);

    Dispatch cpuDispatch(network);
    cpuDispatch.addVehicles(vehicles);
    cpuDispatch.addIncidents(incidents);

    Dispatch gpuDispatch(network);
    gpuDispatch.addVehicles(vehicles);
    gpuDispatch.addIncidents(incidents);

    std::cout << "\n-> Generated "
              << vehicles.size()
              << " benchmark vehicles\n";

    std::cout << "Seed: "
              << VEHICLE_SEED
              << '\n';

    std::cout << "\n-> Generated "
              << incidents.size()
              << " benchmark incidents\n";

    std::cout << "Seed: "
              << INCIDENT_SEED
              << '\n';

    // Create GPU graph and router - not timed
    
    GPUGraph gpuGraph = createGPUGraph(network);
    MetalRouter metalRouter(gpuGraph);

    // Routing Functions

    RouteFunction cpuRoute =
        [&](uint32_t source, uint32_t destination) -> Route {

            return findRouteCPU(
                network,
                source,
                destination
            );
        };

    RouteFunction gpuRoute =
        [&](uint32_t source, uint32_t destination) -> Route {

            return metalRouter.findRouteGPU(
                source,
                destination
            );
        };


    // --- CPU benchmark ---


    std::cout << "\n-> Running Assignment on CPU\n";

    auto cpuStart = std::chrono::steady_clock::now();
    cpuDispatch.findOptimalAssignment(cpuRoute);
    auto cpuEnd = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> cpuElapsed = cpuEnd - cpuStart;
    double totalCpuTime = cpuElapsed.count();


    // --- GPU Benchmark---


    std::cout << "\n-> Running Assignment on GPU\n";
    auto gpuStart = std::chrono::steady_clock::now();
    gpuDispatch.findOptimalAssignment(gpuRoute);
    auto gpuEnd = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> gpuElapsed = gpuEnd - gpuStart;
    double totalGpuTime = gpuElapsed.count();


    // --- Verify results ---


    bool assignmentsMatch = true;

    const auto& cpuAssignments = cpuDispatch.getAssignments();
    const auto& gpuAssignments = gpuDispatch.getAssignments();

    if (cpuAssignments.size() != gpuAssignments.size()) {
        assignmentsMatch = false;
    } 
    else {
        for (const auto& [vehicleID, cpuAssignment] : cpuAssignments) {
            auto gpuIt = gpuAssignments.find(vehicleID);

            if (gpuIt == gpuAssignments.end()) {
                assignmentsMatch = false;
                break;
            }

            const DispatchAssignment& gpuAssignment = gpuIt->second;

            if (cpuAssignment.incident != gpuAssignment.incident ||
                std::abs(cpuAssignment.responseTime - gpuAssignment.responseTime) > 0.001) {

                assignmentsMatch = false;

                std::cout << "\nAssignment mismatch for vehicle "
                        << vehicleID << '\n';

                std::cout << "  CPU incident: "
                        << cpuAssignment.incident << '\n';

                std::cout << "  GPU incident: "
                        << gpuAssignment.incident << '\n';

                std::cout << "  CPU response time: "
                        << cpuAssignment.responseTime << '\n';

                std::cout << "  GPU response time: "
                        << gpuAssignment.responseTime << '\n';
            }
        }
    }

    bool showAssignments = false;
    if (showAssignments) {
        std::cout << "\nCPU assignments:\n";
        
        for (const auto& [vehicleID, assignment] : cpuAssignments) {
            std::cout << "  Vehicle " << vehicleID
            << " -> Incident " << assignment.incident
            << " (" << assignment.responseTime << " min)\n";
        }
        
        std::cout << "\nGPU assignments:\n";
        
        for (const auto& [vehicleID, assignment] : gpuAssignments) {
            std::cout << "  Vehicle " << vehicleID
            << " -> Incident " << assignment.incident
            << " (" << assignment.responseTime << " min)\n";
        }
    }
    
    bool assignmentCountMatch = cpuAssignments.size() == gpuAssignments.size();
    
    double cpuTotal = 0.0;
    double gpuTotal = 0.0;

    for (const auto& [vehicleID, assignment] : cpuAssignments) {
        cpuTotal += assignment.responseTime;
    }

    for (const auto& [vehicleID, assignment] : gpuAssignments) {
        gpuTotal += assignment.responseTime;
    }

    double totalDifference = std::abs(cpuTotal - gpuTotal);

    constexpr double TOTAL_TIME_TOLERANCE = 0.001; // minutes

    bool totalTimeMatch = totalDifference <= TOTAL_TIME_TOLERANCE;
    

    // --- Results ---
    

    std::cout << std::fixed
              << std::setprecision(3);

    std::cout << "\n========== Benchmark ==========\n";
    std::cout << "Task: Route Benchmark\n";
    
    std::cout << "Vehicles: "
                << vehicles.size()
                << '\n';

    std::cout << "Incidents: "
                << incidents.size()
                << '\n';

    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n";

    std::cout << "\nCPU assignments: "
            << cpuAssignments.size() << '\n';

    std::cout << "GPU assignments: "
            << gpuAssignments.size() << '\n';

    std::cout << "Same number of assignments: "
            << (assignmentCountMatch ? "YES" : "NO") << '\n';

    std::cout << "Exact Assignments match: "
          << (assignmentsMatch ? "YES" : "NO")
          << '\n';

    std::cout << "\nCPU assignment total: "
            << cpuTotal << " min\n";

    std::cout << "GPU assignment total: "
            << gpuTotal << " min\n";

    std::cout << "Assignment total difference: "
            << std::setprecision(10)
            << totalDifference << " min\n"
            <<std::setprecision(3);

    std::cout << "Same total assignment time: "
            << (totalTimeMatch ? "YES" : "NO") << '\n';

    std::cout << "\nCPU total: "
              << totalCpuTime
              << " ms\n";

    std::cout << "\nGPU total: "
              << totalGpuTime
              << " ms\n";

    std::cout << "\nGPU speedup: "
              << totalCpuTime / totalGpuTime
              << "x\n";

    std::cout << "================================\n";

    if (!assignmentCountMatch || !totalTimeMatch) {
        std::cerr << "\nWARNING: CPU and GPU assignments have different total time.\n";
        return 1;
    }
    return 0;
}