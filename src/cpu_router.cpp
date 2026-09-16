#include "../include/cpu_router.hpp"
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

Route findRouteCpuAStar(const RoadNetwork& network, uint32_t sourceNode, uint32_t targetNode) { // uses A* algorithm to find route
    Route route;

    route.start = sourceNode;
    route.destination = targetNode;

    const uint32_t nodeCount = network.nodeCount();

    // check valid indexes
    if (sourceNode >= nodeCount || targetNode >= nodeCount) {
        route.status = RouteStatus::InvalidNode;
        return route;
    }

    const double infinity = std::numeric_limits<double>::infinity();

    std::vector<double> gScore(nodeCount, infinity); // currently known least distance to each node from start
    std::vector<uint32_t> parent(nodeCount, std::numeric_limits<uint32_t>::max()); // keep track for backtracking
    std::priority_queue<QueueNode, std::vector<QueueNode>, CompareQueueNode> openSet; // queue is needed for A* pathfinding

    gScore[sourceNode] = 0.0;

    openSet.push(QueueNode{
        .node = sourceNode,
        .fScore = heuristic(network, sourceNode, targetNode)
    });

    while (!openSet.empty()) {
        QueueNode current = openSet.top();
        openSet.pop();

        uint32_t currentNode = current.node;

        // ignore stale entries by calculating expected f Score
        double expectedFScore = gScore[currentNode] + heuristic(network, currentNode, targetNode);

        if (current.fScore > expectedFScore) {
            continue;
        }

        route.nodesVisited++; // for statistics tracking

        if (currentNode == targetNode) {
            break;
        }

        for (const RoadEdge& edge : network.getNeighbours(currentNode)) {
            uint32_t neighbour = edge.destination;
            double tentativeGScore = gScore[currentNode] + (edge.distance/edge.speedLimit) * 60.0;

            if (tentativeGScore < gScore[neighbour]) {
                gScore[neighbour] = tentativeGScore;
                parent[neighbour] = currentNode;

                double fScore = tentativeGScore + heuristic(network, neighbour, targetNode);

                openSet.push(QueueNode{
                    .node = neighbour,
                    .fScore = fScore
                });
            }
        }
    }

    // no route found, return route in state with found = False
    if (gScore[targetNode] == infinity) {
        route.totalTravelTime = infinity;
        return route;
    }

    // route found

    route.status = RouteStatus::Found;

    // so reconstruct node sequeuence

    uint32_t current = targetNode;

    while (current != std::numeric_limits<uint32_t>::max()) {
        route.nodes.push_back(current);
        if (current == sourceNode) break;
        current = parent[current];
    }
    std::reverse(route.nodes.begin(), route.nodes.end());

    route.totalTravelTime = gScore[targetNode];

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

std::vector<Route> findRoutesCpuDijkstraMultiTarget(const RoadNetwork& network, uint32_t sourceNode, const std::vector<uint32_t>& targetNodes) {
    if (targetNodes.empty()) {
        return {};
    }

    const uint32_t nodeCount = static_cast<uint32_t>(network.nodeCount());

    if (sourceNode >= nodeCount) {
        throw std::out_of_range(
            "Source node is out of range"
        );
    }

    for (uint32_t targetNode : targetNodes) {
        if (targetNode >= nodeCount) {
            throw std::out_of_range(
                "Target node is out of range"
            );
        }
    }

    constexpr double INF = std::numeric_limits<double>::infinity();

    struct QueueEntry {
        double distance;
        uint32_t node;

        bool operator>(const QueueEntry& other) const {
            return distance > other.distance;
        }
    };

    std::vector<double> distances(nodeCount, INF);
    std::vector<uint32_t> previousNode(nodeCount, std::numeric_limits<uint32_t>::max());
    std::vector<double> previousEdgeDistance(nodeCount, 0.0);

    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        std::greater<QueueEntry>
    > priorityQueue;

    distances.at(sourceNode) = 0.0;

    priorityQueue.push({
        .distance = 0.0,
        .node = sourceNode
    });

    // Map target node -> its position in targetNodes.
    // This lets us efficiently determine whether a settled
    // node is one of the targets we still need.
    std::unordered_map<uint32_t, std::vector<std::size_t>> targetIndices;

    for (std::size_t i = 0; i < targetNodes.size(); ++i) {
        targetIndices[targetNodes.at(i)].push_back(i);
    }

    std::vector<bool> targetSettled(targetNodes.size(), false);

    std::size_t targetsRemaining = targetNodes.size();

    // Keep track of the number of nodes removed from the queue
    // and settled. This is useful for benchmarking, although
    // Route::nodesVisited is ultimately calculated per route below.
    std::size_t settledNodes = 0;

    while (!priorityQueue.empty()) {

        QueueEntry current = priorityQueue.top();
        priorityQueue.pop();

        const uint32_t currentNode = current.node;
        const double currentDistance = current.distance;

        // Ignore stale priority-queue entries.
        if (currentDistance != distances.at(currentNode)) {
            continue;
        }

        ++settledNodes;

        // If this node is one of our targets, then its shortest
        // path is now final because Dijkstra settles nodes in
        // increasing distance order.
        auto targetIt = targetIndices.find(currentNode);

        if (targetIt != targetIndices.end()) {
            for (std::size_t targetIndex : targetIt->second) {
                if (!targetSettled.at(targetIndex)) {
                    targetSettled.at(targetIndex) = true;
                    --targetsRemaining;
                }
            }

            // We only need the requested targets.
            if (targetsRemaining == 0) {
                break;
            }
        }

        const std::vector<RoadEdge>& neighbours = network.getNeighbours(currentNode);

        for (const RoadEdge& edge : neighbours) {

            if (edge.speedLimit <= 0.0f) {
                continue;
            }

            const double travelTimeMinutes = (static_cast<double>(edge.distance) / static_cast<double>(edge.speedLimit)) * 60.0;

            const double newDistance = currentDistance + travelTimeMinutes;

            const uint32_t nextNode = edge.destination;

            if (newDistance < distances.at(nextNode)) {

                distances.at(nextNode) = newDistance;

                previousNode.at(nextNode) = currentNode;

                previousEdgeDistance.at(nextNode) = static_cast<double>(edge.distance);

                priorityQueue.push({
                    .distance = newDistance,
                    .node = nextNode
                });
            }
        }
    }

    std::vector<Route> routes;
    routes.reserve(targetNodes.size());

    for (uint32_t targetNode : targetNodes) {

        Route route;

        route.start = sourceNode;
        route.destination = targetNode;

        if (!std::isfinite(distances.at(targetNode))) {

            route.status = RouteStatus::NoRoute;
            route.totalTravelTime = std::numeric_limits<float>::infinity();

            route.nodesVisited = 0;
            route.totalDistance = 0.0;

            routes.push_back(route);
            continue;
        }

        route.status = RouteStatus::Found;

        route.totalTravelTime = static_cast<float>(distances.at(targetNode));

        // Reconstruct the path to calculate route-specific
        // distance and number of nodes.
    //     std::size_t nodesInRoute = 1;
    //     double totalDistance = 0.0;

    //     uint32_t currentNode = targetNode;

    //     while (currentNode != sourceNode) {

    //         const uint32_t previous = previousNode.at(currentNode);

    //         if (previous == std::numeric_limits<uint32_t>::max()) {

    //             // Should not happen for a reachable target,
    //             // but prevents an invalid reconstruction loop.
    //             route.status = RouteStatus::NoRoute;
    //             route.totalTravelTime = std::numeric_limits<float>::infinity();

    //             nodesInRoute = 0;
    //             totalDistance = 0.0;

    //             break;
    //         }

    //         totalDistance += previousEdgeDistance.at(currentNode);

    //         currentNode = previous;
    //         ++nodesInRoute;
    //     }

    //     route.nodesVisited = nodesInRoute;
    //     route.totalDistance = totalDistance;

        routes.push_back(route);
    }

    return routes;
}