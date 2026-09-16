#include "../include/router.hpp"
#include "../include/osm_loader.hpp"
#include "../include/scenario_generator.hpp"
#include "../include/dispatch.hpp"
#include "../include/assignment.hpp"
#include "../include/gpu_graph.hpp"
#include "../include/metal_router.hpp"
#include "../include/route_benchmark.hpp"


struct RoutePair {
    unsigned int source;
    unsigned int target;
};

const unsigned int BENCHMARK_SEED = 12345;
const unsigned int DEFAULT_BENCHMARK_ROUTE_COUNT = 10;

int main(int argc, char *argv[]) {
    unsigned int benchmark_route_count = DEFAULT_BENCHMARK_ROUTE_COUNT;

    if (argc == 2) {
        benchmark_route_count = std::stoul(argv[1]);
    }

    std::cout << "Emergency Response Simulation\n";
    
    int result =run_benchmark(benchmark_route_count);
    
    if (result == 1) {
        return 1;
    }
    return 0;
}