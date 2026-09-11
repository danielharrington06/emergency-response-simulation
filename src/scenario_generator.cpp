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

    std::uniform_int_distribution<int> severityDistribution(1, 3);

    std::unordered_set<uint32_t> usedLocations;

    while (incidents.size() < count) {

        std::uint32_t location = locationDistribution(generator);

        if (!usedLocations.insert(location).second) {
            continue;
        }

        IncidentSeverity severity = static_cast<IncidentSeverity>( severityDistribution(generator));

        incidents.push_back({
            .id = static_cast<std::uint32_t>(incidents.size()),
            .location = location,
            .severity = severity
        });
    }

    return incidents;
}