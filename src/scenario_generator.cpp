#include "../include/road_network.hpp"
#include "../include/incident.hpp"
#include "../include/emergency_vehicle.hpp"

#include <random>

std::vector<Incident> generateRandomIncidents(RoadNetwork network, size_t count, unsigned int seed) {
    std::vector<Incident> incidents;
    incidents.reserve(count);

    std::mt19937 generator(seed);

    std::uniform_int_distribution<std::uint32_t> locationDistribution(
        0,
        static_cast<std::uint32_t>(network.nodeCount() - 1)
    );

    std::uniform_int_distribution<int> severityDistribution(
        1,
        3
    );

    for (std::size_t i = 0; i < count; i++) {

        std::uint32_t location = locationDistribution(generator);

        IncidentSeverity severity = static_cast<IncidentSeverity>(severityDistribution(generator));

        incidents.push_back({
            .id = static_cast<std::uint32_t>(i),
            .location = location,
            .severity = severity
        });
    }

    return incidents;
}