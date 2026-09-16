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

int main(int argc, char *argv[]) {
    

    std::cout << "Emergency Response Simulation\n";
    
    int result = run_benchmark(argc, argv);
    
    if (result == 1) {
        return 1;
    }
    return 0;
}