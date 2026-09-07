#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

enum class RouteStatus {
    Found,
    NoRoute,
    InvalidNode
};

struct Route {
    std::vector<uint32_t> nodes;

    uint32_t start;
    uint32_t destination;

    double totalDistance = 0.0;
    double totalTravelTime = 0.0;

    uint32_t nodesVisited = 0;

    RouteStatus status = RouteStatus::NoRoute;

    void printRoute() {
        std::cout << "\n=== Route Calculation ===\n\n";

        std::cout << "Start: Node " << start << '\n';
        std::cout << "Destination: Node " << destination << '\n';
        std::cout << "Algorithm: A*\n\n";

        std::cout << "Route found: "
                << (status==RouteStatus::Found ? "YES" : "NO")
                << "\n";
        std::cout << "Node Index Error: "
                << (status==RouteStatus::InvalidNode ? "YES" : "NO")
                << "\n\n";

        if (status==RouteStatus::Found) {

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