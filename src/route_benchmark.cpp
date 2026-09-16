#include "../include/route_benchmark.hpp"

#include "../include/road_network.hpp"
#include "../include/osm_loader.hpp"
#include "../include/gpu_graph.hpp"
#include "../include/metal_router.hpp"
#include "../include/route.hpp"
#include "../include/router.hpp"
#include "../include/router.hpp"

#include <iostream>
#include <random>
#include <iomanip>
#include <vector>
#include <chrono>

const unsigned int BENCHMARK_SEED = 12345;
const unsigned int DEFAULT_BENCHMARK_ROUTE_COUNT = 10;

int run_benchmark(int argc, char *argv[]) {

    unsigned int benchmark_route_count = DEFAULT_BENCHMARK_ROUTE_COUNT;

    if (argc == 2) {
        benchmark_route_count = std::stoul(argv[1]);
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
    std::mt19937 generator(BENCHMARK_SEED);

    std::uniform_int_distribution<unsigned int> nodeDistribution(
        0,
        static_cast<unsigned int>(network.nodeCount() - 1)
    );

    std::vector<RoutePair> routes;
    routes.reserve(benchmark_route_count);

    for (unsigned int i = 0;
         i < benchmark_route_count;
         ++i) {

        unsigned int source = nodeDistribution(generator);
        unsigned int target = nodeDistribution(generator);

        while (target == source) {
            target = nodeDistribution(generator);
        }

        routes.push_back({source, target});
    }

    std::cout << "\n-> Generated "
              << routes.size()
              << " benchmark routes\n";

    std::cout << "Seed: "
              << BENCHMARK_SEED
              << '\n';

    // Create GPU graph and router - not timed
    GPUGraph gpuGraph = createGPUGraph(network);
    MetalRouter metalRouter(gpuGraph);


    // --- CPU benchmark ---


    double totalCPUTime = 0.0;
    unsigned int successfulRoutes = 0;

    auto CPUstart = std::chrono::steady_clock::now();

    std::vector<float> cpuResults;
    cpuResults.reserve(routes.size());

    for (const RoutePair& routePair : routes) {

        Route route = findRoute(
            network,
            routePair.source,
            routePair.target
        );

        cpuResults.push_back(route.totalTravelTime);

        if (std::isfinite(route.totalTravelTime)) {
            ++successfulRoutes;
        }
    }

    auto CPUend = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> CPUelapsed = CPUend - CPUstart;

    totalCPUTime = CPUelapsed.count();


    // --- GPU ---


    auto GPUstart = std::chrono::steady_clock::now();

    std::vector<float> gpuResults;
    gpuResults.reserve(routes.size());

    for (const RoutePair& routePair : routes) {

        float time = metalRouter.route(
            routePair.source,
            routePair.target
        );

        gpuResults.push_back(time);
    }

    auto GPUend = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> GPUelapsed = GPUend - GPUstart;

    double totalGPUTime = GPUelapsed.count();


    // ---------------------------------------------------------
    // Verify results
    // ---------------------------------------------------------

    unsigned int matchingRoutes = 0;

    for (std::size_t i = 0; i < routes.size(); ++i) {

        const float cpuTime = cpuResults[i];
        const float gpuTime = gpuResults[i];

        if (std::isinf(cpuTime) && std::isinf(gpuTime)) {

            ++matchingRoutes;
            continue;
        }

        else if (std::abs(cpuTime - gpuTime) < 0.001f) {
            ++matchingRoutes;
        }

        else {
            std::cout << "\nRoute "
                    << i
                    << ": "
                    << routes[i].source
                    << " -> "
                    << routes[i].target
                    << '\n';

            std::cout << "  CPU: "
                    << cpuTime
                    << " mins\n";
                    
            Route route = findRoute(
                network,
                routes[i].source,
                routes[i].target
            );
            std::cout << "\tNum Nodes: " << route.nodes.size() << '\n';
                    
            std::cout << "  GPU: "
                    << gpuTime
                    << " mins\n";
            double distance = route.totalDistance;
        
            std::cout << "  Distance: "
                    << distance
                    << '\n';

            std::cout << "  Difference: "
                    << std::abs(cpuTime - gpuTime)
                    << " mins\n";
        }
    }

    // ---------------------------------------------------------
    // Results
    // ---------------------------------------------------------

    std::cout << std::fixed
              << std::setprecision(3);

    std::cout << "\n========== Benchmark ==========\n";

    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '\n';

    std::cout << "Routes: "
              << routes.size()
              << '\n';

    std::cout << "Successful routes: "
              << successfulRoutes
              << '\n';

    std::cout << "Matching CPU/GPU results: "
              << matchingRoutes
              << "/"
              << routes.size()
              << '\n';

    std::cout << "\nCPU total: "
              << totalCPUTime
              << " ms\n";

    std::cout << "CPU average: "
              << totalCPUTime / routes.size()
              << " ms/route\n";

    std::cout << "\nGPU total: "
              << totalGPUTime
              << " ms\n";

    std::cout << "GPU average: "
              << totalGPUTime / routes.size()
              << " ms/route\n";

    std::cout << "\nGPU speedup: "
              << totalCPUTime / totalGPUTime
              << "x\n";

    std::cout << "================================\n";

    if (matchingRoutes != routes.size()) {
        std::cerr
            << "\nWARNING: CPU and GPU results do not all match.\n";
        return 1;
    }
    return 0;
}