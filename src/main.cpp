#include "../include/router.hpp"
#include "../include/osm_loader.hpp"
#include "../include/scenario_generator.hpp"
#include "../include/dispatch.hpp"
#include "../include/assignment.hpp"
#include "../include/gpu_graph.hpp"
#include "../include/metal_router.hpp"

#include <iostream>
#include <iomanip>

// const unsigned int incidentSeed = 12345;
// const unsigned int vehicleSeed = 67890;

// const unsigned int incidentCount = 10;
// const unsigned int vehicleCount = 10;

// const unsigned int DEFAULT_COUNT = 10;

int main(int argc, char *argv[]) {
    unsigned int source = 0;
    unsigned int target = 10;

    if (argc == 2) {
        target = std::stoul(argv[1]);
    }
    if (argc == 3) {
        source = std::stoul(argv[1]);
        target = std::stoul(argv[2]);
    }

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

    // CPU

    auto CPUstart = std::chrono::steady_clock::now();
    Route route = findRoute(network, source, target);
    auto CPUend = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> CPUelapsed = CPUend - CPUstart;
    std::cout << std::fixed << std::setprecision(3)
            << "\nCPU Simulation time: "
            << CPUelapsed.count()
            << " ms\n";

    std::cout << "Time: " << route.totalTravelTime << "mins\n";

    // GPU

    GPUGraph gpuGraph = createGPUGraph(network);
    MetalRouter metalRouter(gpuGraph);

    auto GPUstart = std::chrono::steady_clock::now();
    float time = metalRouter.route(source, target);
    auto GPUend = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> GPUelapsed = GPUend - GPUstart;

    std::cout << std::fixed << std::setprecision(3)
            << "\nGPU Simulation time: "
            << GPUelapsed.count()
            << " ms\n";

    std::cout << "Time: " << time/60 << "mins\n";

    return 0;
}