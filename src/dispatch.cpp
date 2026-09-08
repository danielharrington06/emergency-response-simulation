#include "../include/dispatch.hpp"
#include "../include/router.hpp"
#include "../include/incident.hpp"

#include <iostream>
#include <string>

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

uint32_t Dispatch::addIncident(uint32_t location, IncidentSeverity severity) {
    uint32_t index = incidents.size();

    Incident incident {
        .id = index,
        .location = location,
        .severity = severity
    };

    incidents.push_back(incident);
    return index;
}

const EmergencyVehicle& Dispatch::getVehicle(uint32_t id) const {
    return vehicles.at(id);
}

const Incident& Dispatch::getIncident(uint32_t id) const {
    return incidents.at(id);
}

std::vector<std::vector<double>> Dispatch::calculateResponseTimes(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority) const {

    std::vector<std::vector<double>> responseTimes(availableVehicles.size(), std::vector<double>(incidentsOfThisPriority.size()));

    const double infinity = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < availableVehicles.size(); i++) {
        uint32_t vehicleId = availableVehicles.at(i);
        for (size_t j = 0; j < incidentsOfThisPriority.size(); j++) {
            uint32_t incidentId = incidentsOfThisPriority.at(j);

            Route route = findRoute(network, vehicles.at(vehicleId).location, incidents.at(incidentId).location);
            if (route.status == RouteStatus::Found) {
                responseTimes.at(i).at(j) = route.totalTravelTime;
            }
            else {
                responseTimes.at(i).at(j) = infinity;
            }
        }
    }

    return responseTimes;
}

void Dispatch::assignAvailableVehiclesToIncidents(std::vector<uint32_t> availableVehicles, std::vector<uint32_t> incidentsOfThisPriority) {
    if (availableVehicles.empty() || incidentsOfThisPriority.empty()) {
        return;
    }

    std::vector<std::vector<double>> responseTimes = calculateResponseTimes(availableVehicles, incidentsOfThisPriority);

    double bestTotalTime = std::numeric_limits<double>::infinity();
    size_t bestAssignedCount = 0;

    std::vector<DispatchAssignment> currentAssignment;
    std::vector<DispatchAssignment> bestAssignment;

    std::vector<bool> incidentUsed(
        incidentsOfThisPriority.size(),
        false
    );
}

void Dispatch::findOptimalAssignment() { // assigns vehicles in priority order, so all high first, then medium, then low, minimising total response time in each section

    vehicleAssignments.clear();

    // deal with no vehicles or no incidents
    if (vehicles.empty() || incidents.empty()) {
        return;
    }

    // TODO go in order of priority and assign
}



size_t Dispatch::incidentCount() const {
    return incidents.size();
}

size_t Dispatch::vehicleCount() const {
    return vehicles.size();
}

std::string vehicleTypeToString(VehicleType type) {

    switch (type) {
        case VehicleType::Ambulance:
            return "Ambulance";
    }

    return "Unknown";
}

std::string vehicleStatusToString(VehicleStatus status) {

    switch (status) {
        case VehicleStatus::Available:
            return "Available";

        case VehicleStatus::Dispatched:
            return "Dispatched";
    }

    return "Unknown";
}

std::string incidentSeverityToString(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::Low:
            return "Low";
        case IncidentSeverity::Medium:
            return "Medium";
        case IncidentSeverity::High:
            return "High";
    }
    
    return "Unknown";
}

void Dispatch::printDispatch() const {
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

void Dispatch::printAssignments() const {
    std::cout << "\n=== Dispatch Assignments ===\n\n";

    for (uint32_t vehicleId = 0; vehicleId < vehicles.size(); vehicleId++) {

        const DispatchAssignment& assignment = vehicleAssignments.at(vehicleId);

        std::cout << "Vehicle " << assignment.vehicle << " -> "
                  << "Incident " << assignment.incident << '\n';

        std::cout << "\tResponse time: "
                  << assignment.responseTime
                  << " minutes\n\n";
    }
}