#pragma once

#include "road_network.hpp"
#include "route.hpp"

double heuristic(const RoadNetwork& network, uint32_t current, uint32_t destination);

Route findRouteCpuAStar(const RoadNetwork& network, uint32_t sourceNode, uint32_t targetNode);
std::vector<Route> findRoutesCpuDijkstraMultiTarget(const RoadNetwork& network, uint32_t sourceNode, const std::vector<uint32_t>& targetNodes);