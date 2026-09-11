#include "../include/road_network.hpp"
#include "../include/incident.hpp"
#include "../include/emergency_vehicle.hpp"

#include <random>
#include <unordered_set>

std::vector<Incident> generateRandomIncidents(const RoadNetwork& network, size_t count, unsigned int seed) {
    std::vector<Incident> incidents;
    incidents.reserve(count);

    std::mt19937 generator(seed);

    std::uniform_int_distribution<std::uint32_t> locationDistribution(0, static_cast<std::uint32_t>(network.nodeCount() - 1));

    std::uniform_int_distribution<int> typeDistribution(0, 2); // randomly generate to give variation in proportion
    std::uniform_int_distribution<int> severityDistribution(1, 3); // randomly generate to give variation

    std::unordered_set<uint32_t> usedLocations;

    while (incidents.size() < count) {

        std::uint32_t location = locationDistribution(generator);

        if (!usedLocations.insert(location).second) {
            continue;
        }

        IncidentType type = static_cast<IncidentType>(typeDistribution(generator));

        IncidentSeverity severity = static_cast<IncidentSeverity>( severityDistribution(generator));

        incidents.push_back({
            .id = static_cast<std::uint32_t>(incidents.size()),
            .location = location,
            .type = type,
            .severity = severity
        });
    }

    return incidents;
}

std::vector<EmergencyVehicle> generateRandomVehicles(const RoadNetwork& network, size_t count, unsigned int seed) {
    std::vector<EmergencyVehicle> vehicles;
    vehicles.reserve(count);

    std::mt19937 generator(seed);

    std::uniform_int_distribution<std::uint32_t> locationDistribution(0, static_cast<std::uint32_t>(network.nodeCount() - 1));

    std::unordered_set<std::uint32_t> usedLocations;

    for (std::size_t i = 0; i < count; i++) {

        std::uint32_t location;

        do {
            location = locationDistribution(generator);
        } while (!usedLocations.insert(location).second);

        VehicleType type; // do not randomly generate as we want an equal (or near enough) amount of each

        switch (i % 3) {
            case 0:
                type = VehicleType::Ambulance;
                break;
            case 1:
                type = VehicleType::FireEngine;
                break;
            default:
                type = VehicleType::PoliceCar;
                break;
        }

        vehicles.push_back({
            .id = static_cast<std::uint32_t>(i),
            .location = location,
            .type = type,
            .status = VehicleStatus::Available
        });
    }

    return vehicles;
}