#include "../include/router.hpp"

#include <cmath>
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>

struct QueueNode {
    uint32_t node;
    double fScore;
};

struct CompareQueueNode { // needed to tell the queue to prioritise the least f score
    bool operator()(const QueueNode& a, const QueueNode& b) const {
        return a.fScore > b.fScore;
    }
};

double heuristic(const RoadNetwork& network, uint32_t current, uint32_t destination) { // for test network, using latitude and longitiude as cartesian coordinates, but on real data, we will need geographic distance calculation
    const RoadNode& currentNode = network.getNode(current);
    const RoadNode& destinationNode = network.getNode(destination);

    double dx = currentNode.latitude - destinationNode.latitude;
    double dy = currentNode.longitude - destinationNode.longitude;

    return std::sqrt(dx*dx + dy*dy);
}

Route findRoute(const RoadNetwork& network, uint32_t start, uint32_t destination) { // uses A* algorithm to find route
    Route route;

    const uint32_t nodeCount = network.nodeCount();

    const double infinity = std::numeric_limits<double>::infinity();

    std::vector<double> gScore(nodeCount, infinity);
    std::vector<uint32_t> parent(nodeCount, std::numeric_limits<uint32_t>::max());
    std::priority_queue<QueueNode, std::vector<QueueNode>, CompareQueueNode> openSet;

    gScore[start] = 0.0;

    openSet.push(QueueNode{
        .node = start,
        .fScore = heuristic(network, start, destination)
    });

    while (!openSet.empty()) {
        QueueNode current = openSet.top();
        openSet.pop();

        uint32_t currentNode = current.node;

        // ignore stale entries by calculating expected f Score
        double expectedFScore = gScore[currentNode] + heuristic(network, currentNode, destination);

        if (current.fScore > expectedFScore) {
            continue;
        }

        route.nodesVisited++;

        if (currentNode == destination) {
            break;
        }

        for (const RoadEdge& edge : network.getNeighbours(currentNode)) {
            uint32_t neighbour = edge.destination;
            double tentativeGScore = gScore[currentNode] + edge.distance;

            if (tentativeGScore < gScore[neighbour]) {
                gScore[neighbour] = tentativeGScore;
                parent[neighbour] = currentNode;

                double fScore = tentativeGScore + heuristic(network, neighbour, destination);

                openSet.push(QueueNode{
                    .node = neighbour,
                    .fScore = fScore
                });
            }
        }
    }

    // no route found, return route in state with found = False
    if (gScore[destination] == infinity) {
        return route;
    }

    // route found

    route.found = true;

    // so reconstruct node sequeuence

    uint32_t current = destination;
        while (current != std::numeric_limits<uint32_t>::max()) {

        route.nodes.push_back(current);

        if (current == start) {
            break;
        }

        current = parent[current];
    }

    std::reverse(route.nodes.begin(), route.nodes.end());

    route.totalDistance = gScore[destination];

    // calculate travel time

    for (size_t i = 0; i + 1 < route.nodes.size(); i++) {

        uint32_t from = route.nodes[i];
        uint32_t to = route.nodes[i + 1];

        for (const RoadEdge& edge : network.getNeighbours(from)) {

            if (edge.destination == to) {

                route.totalTravelTime += (edge.distance / edge.speedLimit) * 60.0;

                break;
            }
        }
    }

    return route;
}