#include "../include/dispatch.hpp"
#include "../include/router.hpp"
#include "../include/incident.hpp"

#include <iostream>
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>

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

std::vector<std::vector<double>> Dispatch::calculateResponseTimes(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority) const { // calculates response times for the given vehicles and incidents
    // may want to change parameter names to just vehicles and incidents

    std::vector<std::vector<double>> responseTimes(availableVehicles.size(), std::vector<double>(incidentsOfThisPriority.size()));

    const double infinity = std::numeric_limits<double>::infinity();

    // loop over vehicles, then incidents to get response times
    for (size_t i = 0; i < availableVehicles.size(); i++) {
        uint32_t vehicleId = availableVehicles.at(i);

        for (size_t j = 0; j < incidentsOfThisPriority.size(); j++) {
            uint32_t incidentId = incidentsOfThisPriority.at(j);

            Route route = findRoute(network, vehicles.at(vehicleId).location, incidents.at(incidentId).location);

            // make response time infinite if vehicle is incompatible
            if (!isVehicleCompatible(vehicles.at(vehicleId).type, incidents.at(incidentId).type)) {
                responseTimes.at(i).at(j) = infinity;
                continue;
            }
            // if route found (and not incompatible) store actual response time
            else if (route.status == RouteStatus::Found) {
                responseTimes.at(i).at(j) = route.totalTravelTime;
            }
            // route is unreachable (rare to execute as would require no route between the vehicle and incident)
            else {
                responseTimes.at(i).at(j) = infinity;
            }
        }
    }
    // for debugging - remove for larger networks and incident-vehicle counts
    printResponseTimesMatrix(responseTimes);

    return responseTimes;
}

void Dispatch::assignAvailableVehiclesToIncidents(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority) {
    // availableVehicles is a list of actual vehicle Id's, same for incidentsOfThisPriority
    // concept here: exhaustive search to find way to minimise total response time, but 'pad' the matrix 
    //.. (not actually, just conceptually through index checks) and any padded values would be unassigned vehicles/incidents
    //.. so are given time of 0

    if (availableVehicles.empty() || incidentsOfThisPriority.empty()) {
        return;
    }

    std::vector<std::vector<double>> responseTimes = calculateResponseTimes(availableVehicles, incidentsOfThisPriority);

    const size_t assignmentSize = std::max(availableVehicles.size(), incidentsOfThisPriority.size());
    std::vector<uint32_t> assignmentOrder(assignmentSize);

    for (uint32_t i = 0; i < assignmentSize; i++) {
        assignmentOrder.at(i) = i;
    }

    double bestTotalTime = std::numeric_limits<double>::infinity();
    size_t bestAssignedCount = 0; // this is needed so that the code doesnt minimise total time by not assigning
    std::vector<DispatchAssignment> bestAssignment;

    do {
        double totalTime = 0.0;
        size_t assignedCount = 0;
        std::vector<DispatchAssignment> currentAssignment;

        for(size_t i = 0; i < assignmentSize; i++) {
            // no real vehicle exists at this position
            if (i >= availableVehicles.size()) {
                continue;
            }
            // no real incident exists at this index
            if (assignmentOrder.at(i) >= incidentsOfThisPriority.size()) {
                continue;
            }

            uint32_t vehicleId = availableVehicles.at(i);
            uint32_t incidentIndex = assignmentOrder.at(i);
            double responseTime = responseTimes.at(i).at(incidentIndex);

            // this vehicle cannot reach this incident
            if (!std::isfinite(responseTime)) {
                totalTime = std::numeric_limits<double>::infinity();
                break;
            }
            totalTime += responseTime;

            assignedCount++;

            currentAssignment.push_back({
                vehicleId,
                incidentsOfThisPriority.at(incidentIndex),
                responseTime
            });
        }

        // aim: maximise the number of incidents assigned, then minimise total response time
        if (assignedCount > bestAssignedCount || (assignedCount == bestAssignedCount && totalTime < bestTotalTime)) {
            bestAssignedCount = assignedCount;
            bestTotalTime = totalTime;
            bestAssignment = currentAssignment;
        }
    } while (std::next_permutation(assignmentOrder.begin(), assignmentOrder.end()));

    // store the best assignment and update vehicle status
    for (const DispatchAssignment& assignment : bestAssignment) {
        vehicleAssignments[assignment.vehicle] = assignment;
        vehicles.at(assignment.vehicle).status = VehicleStatus::Dispatched;
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

void Dispatch::findOptimalAssignment() { // assigns vehicles in priority order, so all high first, then medium, then low, minimising total response time in each section

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
    assignAvailableVehiclesToIncidents(availableVehicles, highSeverityIncidents);

    // rebuild list of remaining available vehicles
    availableVehicles = buildAvailableVehicles();
    if (availableVehicles.empty()) return;

    // assign medium priority incidents
    assignAvailableVehiclesToIncidents(availableVehicles, mediumSeverityIncidents);

    // rebuild list of remaining available vehicles
    availableVehicles = buildAvailableVehicles();
    if (availableVehicles.empty()) return;

    // assign low priority incidents
    assignAvailableVehiclesToIncidents(availableVehicles, lowSeverityIncidents);
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