#pragma once

#include "emergency_vehicle.hpp"
#include "incident.hpp"
#include "road_network.hpp"

#include <vector>
#include <unordered_map>

struct DispatchAssignment {
    uint32_t vehicle;
    uint32_t incident;
    double responseTime;
};

class Dispatch {

private:
    std::vector<Incident> incidents;
    std::vector<EmergencyVehicle> vehicles;
    std::unordered_map<uint32_t, DispatchAssignment> vehicleAssignments;
    RoadNetwork network;

    void assignAvailableVehiclesToIncidents(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority, const std::vector<std::vector<double>>& responseTimes);
    std::vector<uint32_t> buildAvailableVehicles();

public:
    Dispatch(RoadNetwork rn) {
        network = rn;
    }

    uint32_t addVehicle(uint32_t location, VehicleType type);
    uint32_t addIncident(uint32_t location, IncidentSeverity severity);

    const EmergencyVehicle& getVehicle(uint32_t id) const;
    const Incident& getIncident(uint32_t id) const;

    std::vector<std::vector<double>> calculateResponseTimes(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority) const;
    void Dispatch::assignAvailableVehiclesToIncidents(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority);
    void findOptimalAssignment();

    size_t incidentCount() const;
    size_t vehicleCount() const;

    void printDispatch() const;
    void printResponseTimesMatrix(std::vector<std::vector<double>>& responseTimes) const;
    void printVehicles() const;
    void printIncidents() const;
};