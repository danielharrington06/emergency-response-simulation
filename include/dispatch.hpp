#pragma once

#include "emergency_vehicle.hpp"
#include "incident.hpp"
#include "road_network.hpp"

#include <vector>
#include <unordered_map>

class Dispatch {

private:
    std::vector<Incident> incidents;
    std::vector<EmergencyVehicle> vehicles;
    std::unordered_map<uint32_t, uint32_t> vehicleAssignments;

public:
    uint32_t addVehicle(uint32_t location, VehicleType type);
    uint32_t addIncident(uint32_t location, uint32_t severity);

    const EmergencyVehicle& getVehicle(uint32_t id) const;
    const Incident& getIncident(uint32_t id) const;

    std::vector<std::vector<double>> calculateResponseTimes(const RoadNetwork& network) const;

    size_t incidentCount() const;
    size_t vehicleCount() const;

    void printDispatch() const;
    void printResponseTimesMatrix(std::vector<std::vector<double>> responseTimes) const;
};