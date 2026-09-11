#pragma once

#include "road_network.hpp"
#include "incident.hpp"
#include "emergency_vehicle.hpp"

#include <cstddef>
#include <vector>

std::vector<Incident> generateRandomIncidents(const RoadNetwork& network, std::size_t count, unsigned int seed);

std::vector<EmergencyVehicle> generateRandomVehicles(const RoadNetwork& network, std::size_t count, unsigned int seed);