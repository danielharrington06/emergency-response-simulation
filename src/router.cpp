#include "../include/router.hpp"

#include <cmath>

Route findRoute(const RoadNetwork& network, uint32_t start, uint32_t destination) {
    Route route;

    return route;
}

double heuristic(const RoadNetwork& network, uint32_t current, uint32_t destination) { // for test network, using latitude and longitiude as cartesian coordinates, but on real data, we will need geographic distance calculation
    const RoadNode& currentNode = network.getNode(current);
    const RoadNode& destinationNode = network.getNode(destination);

    double dx = currentNode.latitude - destinationNode.latitude;
    double dy = currentNode.longitude - destinationNode.longitude;

    return std::sqrt(dx*dx + dy*dy);
}