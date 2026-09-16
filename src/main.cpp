#include "../include/benchmark_route.hpp"

#include <iostream>

int main(int argc, char *argv[]) {

    std::cout << "Emergency Response Simulation\n";
    
    int result = runRouteBenchmark(argc, argv);
    
    if (result == 1) {
        return 1;
    }
    return 0;
}