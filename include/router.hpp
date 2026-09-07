#pragma once

#include "road_network.hpp"
#include "route.hpp"

Route findRoute(const RoadNetwork& network, uint32_t start, uint32_t destination);