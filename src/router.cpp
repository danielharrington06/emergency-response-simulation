#include "../include/router.hpp"
#include "../include/route.hpp"

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

double heuristic(const RoadNetwork& network, uint32_t current, uint32_t destination) {
    const RoadNode& currentNode = network.getNode(current);

    const RoadNode& destinationNode = network.getNode(destination);

    constexpr double PI = 3.14159265358979323846;
    constexpr double EARTH_RADIUS_MILES = 3958.7613;    
    constexpr double MAX_SPEED_MPH = 70.0;

    double latitude1 = currentNode.latitude * PI / 180.0;

    double latitude2 = destinationNode.latitude * PI / 180.0;

    double latitudeDifference = (destinationNode.latitude - currentNode.latitude) * PI / 180.0;

    double longitudeDifference = (destinationNode.longitude - currentNode.longitude) * PI / 180.0;

    double a =
        std::sin(latitudeDifference / 2.0)
        * std::sin(latitudeDifference / 2.0)
        +
        std::cos(latitude1)
        * std::cos(latitude2)
        * std::sin(longitudeDifference / 2.0)
        * std::sin(longitudeDifference / 2.0);

    double distance = 2.0 * EARTH_RADIUS_MILES * std::asin(std::sqrt(a));

    return distance / MAX_SPEED_MPH * 60.0;
}

Route findRouteCPU(const RoadNetwork& network, uint32_t start, uint32_t destination) { // uses A* algorithm to find route
    Route route;

    route.start = start;
    route.destination = destination;

    const uint32_t nodeCount = network.nodeCount();

    // check valid indexes
    if (start >= nodeCount || destination >= nodeCount) {
        route.status = RouteStatus::InvalidNode;
        return route;
    }

    const double infinity = std::numeric_limits<double>::infinity();

    std::vector<double> gScore(nodeCount, infinity); // currently known least distance to each node from start
    std::vector<uint32_t> parent(nodeCount, std::numeric_limits<uint32_t>::max()); // keep track for backtracking
    std::priority_queue<QueueNode, std::vector<QueueNode>, CompareQueueNode> openSet; // queue is needed for A* pathfinding

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

        route.nodesVisited++; // for statistics tracking

        if (currentNode == destination) {
            break;
        }

        for (const RoadEdge& edge : network.getNeighbours(currentNode)) {
            uint32_t neighbour = edge.destination;
            double tentativeGScore = gScore[currentNode] + (edge.distance/edge.speedLimit) * 60.0;

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
        route.totalTravelTime = infinity;
        return route;
    }

    // route found

    route.status = RouteStatus::Found;

    // so reconstruct node sequeuence

    uint32_t current = destination;

    while (current != std::numeric_limits<uint32_t>::max()) {
        route.nodes.push_back(current);
        if (current == start) break;
        current = parent[current];
    }
    std::reverse(route.nodes.begin(), route.nodes.end());

    route.totalTravelTime = gScore[destination];

    // calculate travel distance

    for (size_t i = 0; i + 1 < route.nodes.size(); i++) {
        uint32_t from = route.nodes[i];
        uint32_t to = route.nodes[i + 1];

        for (const RoadEdge& edge : network.getNeighbours(from)) {
            if (edge.destination == to) {
                route.totalDistance += edge.distance;
                break;
            }
        }
    }

    return route;
}