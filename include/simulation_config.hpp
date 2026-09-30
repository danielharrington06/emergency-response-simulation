#pragma once

#include <cstdint>

class SimulationConfig {
public:
    static SimulationConfig& getInstance();

    uint32_t incidentSeed;
    uint32_t vehicleSeed;
    uint32_t defaultNumIncidents;
    uint32_t defaultNumVehicles;

private:
    SimulationConfig();

    SimulationConfig(const SimulationConfig&) = delete;
    SimulationConfig& operator=(const SimulationConfig&) = delete;
};