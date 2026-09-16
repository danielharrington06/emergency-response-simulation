#pragma once

#include "emergency_vehicle.hpp"
#include "incident.hpp"
#include "road_network.hpp"
#include "route.hpp"

#include <vector>
#include <unordered_map>
#include <functional>

struct DispatchAssignment {
    uint32_t vehicle;
    uint32_t incident;
    double responseTime;
};

using RouteFunction = std::function<Route(uint32_t, uint32_t)>;
using RouteFunctionPointToMany = std::function<std::vector<Route>(uint32_t, std::vector<uint32_t>)>;

class Dispatch {

private:
    std::vector<Incident> incidents;
    std::vector<EmergencyVehicle> vehicles;
    std::unordered_map<uint32_t, DispatchAssignment> vehicleAssignments;
    RoadNetwork network;

    std::vector<uint32_t> buildAvailableVehicles();

public:
    Dispatch(const RoadNetwork& rn)
        : network(rn) {}

    uint32_t addVehicle(uint32_t location, VehicleType type);
    uint32_t addIncident(uint32_t location, IncidentType, IncidentSeverity severity);

    void addVehicles(const std::vector<EmergencyVehicle>& newVehicles);
    void addIncidents(const std::vector<Incident>& newIncidents);

    const EmergencyVehicle& getVehicle(uint32_t id) const;
    const Incident& getIncident(uint32_t id) const;
    const std::unordered_map<uint32_t, DispatchAssignment>& getAssignments() const;

    void assignAvailableVehiclesToIncidents(std::vector<uint32_t> givenVehicles, std::vector<uint32_t> givenIncidents, std::vector<std::vector<double>> responseTimes);

    std::vector<std::vector<double>> calculateResponseTimes(std::vector<uint32_t> givenVehicles, std::vector<uint32_t> givenIncidents, const RouteFunction& routeFunction) const;
    void findOptimalAssignment(RouteFunction routeFunction);

    std::vector<std::vector<double>> calculateResponseTimesPointToMany(std::vector<uint32_t> givenVehicles, std::vector<uint32_t> givenIncidents, const RouteFunctionPointToMany& routeFunction) const;
    void findOptimalAssignmentPointToMany(RouteFunctionPointToMany routeFunction);

    size_t incidentCount() const;
    size_t vehicleCount() const;

    void printResponseTimesMatrix(std::vector<std::vector<double>>& responseTimes) const;
    void printVehicles() const;
    void printIncidents() const;
};