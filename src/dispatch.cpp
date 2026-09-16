#include "../include/dispatch.hpp"
#include "../include/router.hpp"
#include "../include/incident.hpp"
#include "../include/assignment.hpp"
#include "../include/router.hpp"

#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>
#include <chrono>

bool isVehicleCompatible(VehicleType vehicleType, IncidentType incidentType) {
    switch (incidentType) {
        case IncidentType::Medical:
            return vehicleType == VehicleType::Ambulance;

        case IncidentType::Fire:
            return vehicleType == VehicleType::FireEngine;

        case IncidentType::Crime:
            return vehicleType == VehicleType::PoliceCar;
    }

    return false;
}

uint32_t Dispatch::addVehicle(uint32_t location, VehicleType type) {
    uint32_t index = vehicles.size();
    
    EmergencyVehicle vehicle {
        .id = index,
        .location = location,
        .type = type,
        .status = VehicleStatus::Available
    };

    vehicles.push_back(vehicle);
    return index;
}

uint32_t Dispatch::addIncident(uint32_t location, IncidentType type, IncidentSeverity severity) {
    uint32_t index = incidents.size();

    Incident incident {
        .id = index,
        .location = location,
        .type = type,
        .severity = severity
    };

    incidents.push_back(incident);
    return index;
}

void Dispatch::addVehicles(const std::vector<EmergencyVehicle>& newVehicles) {
    for (const EmergencyVehicle& vehicle : newVehicles) {
        vehicles.push_back(vehicle);
    }
}

void Dispatch::addIncidents(const std::vector<Incident>& newIncidents) {
    for (const Incident& incident : newIncidents) {
        incidents.push_back(incident);
    }
}

const EmergencyVehicle& Dispatch::getVehicle(uint32_t id) const {
    return vehicles.at(id);
}

const Incident& Dispatch::getIncident(uint32_t id) const {
    return incidents.at(id);
}

std::vector<std::vector<double>> Dispatch::calculateResponseTimes(std::vector<uint32_t> givenVehicles, std::vector<uint32_t> givenIncidents, const RouteFunction& routeFunction) const {
    std::vector<std::vector<double>> responseTimes(givenVehicles.size(), std::vector<double>(givenIncidents.size()));

    std::size_t totalNodesVisited = 0;
    std::size_t routeCount = 0;

    double totalDistance = 0;
    double totalTime = 0;
    double totalHeuristic = 0;

    const double infinity = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < givenVehicles.size(); i++) {
        uint32_t vehicleId = givenVehicles.at(i);

        for (size_t j = 0; j < givenIncidents.size(); j++) {
            uint32_t incidentId = givenIncidents.at(j);

            const EmergencyVehicle& vehicle = vehicles.at(vehicleId);

            const Incident& incident = incidents.at(incidentId);

            // Do not calculate a route if this vehicle
            // can never respond to this incident.
            if (!isVehicleCompatible( vehicle.type, incident.type )) {
                responseTimes.at(i).at(j) = infinity;
                continue;
            }

            Route route = routeFunction(
                vehicle.location,
                incident.location
            );

            if (route.status == RouteStatus::Found) {
                responseTimes.at(i).at(j) = route.totalTravelTime;
                totalNodesVisited += route.nodesVisited;
                totalDistance += route.totalDistance;
                totalTime += route.totalTravelTime;
                totalHeuristic += heuristic(network, vehicle.location, incident.location);
                routeCount++;
            }
            else {
                responseTimes.at(i).at(j) = infinity;
            }
        }
    }
    return responseTimes;
}

void Dispatch::assignAvailableVehiclesToIncidents(std::vector<uint32_t> givenVehicles, std::vector<uint32_t> givenIncidents, const RouteFunction& routeFunction) {
    // previously used exhuastive search but this was factorial time efficiency, so had to replace with ...
    //.. with hungarian assignment algorithm in assignment.cpp

    if (givenVehicles.empty() ||
        givenIncidents.empty()) {
        return;
    }

    auto routeStart = std::chrono::steady_clock::now();
    std::vector<std::vector<double>> responseTimes = calculateResponseTimes(givenVehicles, givenIncidents, routeFunction);
    auto routeEnd = std::chrono::steady_clock::now();

    auto assignmentStart = std::chrono::steady_clock::now();
    std::vector<Assignment> assignments = findOptimalAssignmentHungarian(responseTimes);
    auto assignmentEnd = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> routeElapsed =
    routeEnd - routeStart;

    std::chrono::duration<double, std::milli> assignmentElapsed =
        assignmentEnd - assignmentStart;

    std::cout << "  Route calculations: "
            << routeElapsed.count()
            << " ms\n";

    std::cout << "  Hungarian assignment: "
            << assignmentElapsed.count()
            << " ms\n";

    for (const Assignment& assignment : assignments) {
        uint32_t vehicleId = givenVehicles.at(assignment.vehicleIndex);

        uint32_t incidentId = givenIncidents.at(assignment.incidentIndex);

        double responseTime = responseTimes.at(assignment.vehicleIndex).at(assignment.incidentIndex);

        vehicleAssignments[vehicleId] = {
            .vehicle = vehicleId,
            .incident = incidentId,
            .responseTime = responseTime
        };

        vehicles.at(vehicleId).status = VehicleStatus::Dispatched;
    }
}

std::vector<uint32_t> Dispatch::buildAvailableVehicles() { // gets a vector of the currently available vehicles
    std::vector<uint32_t> availableVehicles;
    for (const EmergencyVehicle& vehicle : vehicles) {
        if (vehicle.status == VehicleStatus::Available) {
            availableVehicles.push_back(vehicle.id);
        }
    }
    return availableVehicles;
}

void Dispatch::findOptimalAssignment(RouteFunction routeFunction) { // assigns vehicles in priority order, so all high first, then medium, then low, minimising total response time in each section

    vehicleAssignments.clear(); // probably will not want to do this when dynamically doing stuff 

    // deal with no vehicles or no incidents
    if (vehicles.empty() || incidents.empty()) {
        return;
    }

    // group incidents by severity
    std::vector<uint32_t> highSeverityIncidents;
    std::vector<uint32_t> mediumSeverityIncidents;
    std::vector<uint32_t> lowSeverityIncidents;
    
    for (const Incident& incident : incidents) {
        switch(incident.severity) {
            case IncidentSeverity::High:
            highSeverityIncidents.push_back(incident.id);
            break;
            case IncidentSeverity::Medium:
            mediumSeverityIncidents.push_back(incident.id);
            break;
            case IncidentSeverity::Low:
            lowSeverityIncidents.push_back(incident.id);
            break;
        }
    }

    
    // start with every available vehicle
    std::vector<uint32_t> availableVehicles = buildAvailableVehicles();
    if (availableVehicles.empty()) return;
    
    // assign high priority incidents
    std::cout << "\nHigh Severity:\n";
    assignAvailableVehiclesToIncidents(availableVehicles, highSeverityIncidents, routeFunction);
    
    // rebuild list of remaining available vehicles
    availableVehicles = buildAvailableVehicles();
    if (availableVehicles.empty()) return;
    
    // assign medium priority incidents
    std::cout << "\nMedium Severity:\n";
    assignAvailableVehiclesToIncidents(availableVehicles, mediumSeverityIncidents, routeFunction);
    
    // rebuild list of remaining available vehicles
    availableVehicles = buildAvailableVehicles();
    if (availableVehicles.empty()) return;
    
    // assign low priority incidents
    std::cout << "\nLow Severity:\n";
    assignAvailableVehiclesToIncidents(availableVehicles, lowSeverityIncidents, routeFunction);
}


size_t Dispatch::incidentCount() const {
    return incidents.size();
}

size_t Dispatch::vehicleCount() const {
    return vehicles.size();
}

void Dispatch::printVehicles() const {
    std::cout << "\n=== Vehicles ===\n";
    std::cout << "Count: " << vehicleCount() << "\n\n";
    for (size_t i = 0; i < vehicles.size(); i++) {
        std::cout << "Vehicle " << i << ":\n";
        std::cout << "\tLocation: Node " << vehicles.at(i).location << '\n';
        std::cout << "\tType: " << vehicleTypeToString(vehicles.at(i).type) << '\n';
        VehicleStatus status = vehicles.at(i).status;
        std::cout << "\tStatus: " << vehicleStatusToString(status) << '\n';
        if (status == VehicleStatus::Dispatched) {
            std::cout << "\tAssigned to: Incident " << vehicleAssignments.at(i).incident << '\n';
            std::cout << "\tResponse Time: " << vehicleAssignments.at(i).responseTime << " mins\n";
        }
        std::cout << '\n';
    }
}

void Dispatch::printIncidents() const {
    std::cout << "\n=== Incidents ===\n";
    std::cout << "Count: " << incidentCount() << "\n\n";
    for (size_t i = 0; i < incidents.size(); i++) {
        std::cout << "Incident " << i << ":\n";
        std::cout << "\tLocation: Node " << incidents.at(i).location << '\n';
        std::cout << "\tSeverity: " << incidentSeverityToString(incidents.at(i).severity) << "\n\n";
    }
}

void Dispatch::printResponseTimesMatrix(std::vector<std::vector<double>>& responseTimes) const {
    std::cout << "\n=== Response Time Matrix ===\n\n";

    for (size_t i = 0; i < responseTimes.size(); i++) {

        std::cout << "Vehicle " << i << ":\t";

        for (size_t j = 0; j < responseTimes.at(i).size(); j++) {

            std::cout << responseTimes.at(i).at(j)
                    << " min";

            if (j + 1 < responseTimes.at(i).size()) {
                std::cout << " | ";
            }
        }

        std::cout << '\n';
    }
}