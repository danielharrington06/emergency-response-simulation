#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

struct Route {
    std::vector<uint32_t> nodes;

    double totalDistance = 0.0;
    double totalTravelTime = 0.0;

    uint32_t nodesVisited = 0;

    bool found = false;

    void printRoute() {
        std::cout << "\n=== Route Calculation ===\n\n";

        std::cout << "Start: Node 0\n";
        std::cout << "Destination: Node 3\n";
        std::cout << "Algorithm: A*\n\n";

        std::cout << "Route found: "
                << (found ? "YES" : "NO")
                << "\n\n";


        if (found) {

            std::cout << "Route: ";

            for (size_t i = 0;
                i < nodes.size();
                i++) {

                std::cout << nodes[i];

                if (i + 1 < nodes.size()) {
                    std::cout << " -> ";
                }
            }

            std::cout << "\n\n";

            std::cout << "Distance: "
                    << totalDistance
                    << " km\n";

            std::cout << "Travel time: "
                    << totalTravelTime
                    << " minutes\n";

            std::cout << "Nodes visited: "
                    << nodesVisited
                    << '\n';
        }
    }
};