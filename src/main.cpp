#include "../include/route_benchmark.hpp"

#include <iostream>

int main(int argc, char *argv[]) {

    std::cout << "Emergency Response Simulation\n";
    
    int result = run_benchmark(argc, argv);
    
    if (result == 1) {
        return 1;
    }
    return 0;
}