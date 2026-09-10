#pragma once

#include "road_network.hpp"

#include <string>

RoadNetwork loadOsmNetwork(const std::string& nodesFile, const std::string& edgesFile);