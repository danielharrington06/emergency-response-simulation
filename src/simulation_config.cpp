#include "../include/simulation_config.hpp"

const unsigned int VEHICLE_SEED = 54321;
const unsigned int INCIDENT_SEED = 72849;

SimulationConfig::SimulationConfig(): 
    vehicleSeed(VEHICLE_SEED),
    incidentSeed(INCIDENT_SEED),
    defaultNumIncidents(10),
    defaultNumVehicles(10) {
}

SimulationConfig& SimulationConfig::getInstance() {
    static SimulationConfig instance;
    return instance;
}