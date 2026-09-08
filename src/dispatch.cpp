#include "../include/dispatch.hpp"
#include "../include/router.hpp"

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

uint32_t Dispatch::addIncident(uint32_t location, uint32_t severity) {
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

std::vector<std::vector<double>> Dispatch::calculateResponseTimes(const RoadNetwork& network) const {
    std::vector<std::vector<double>> responseTimes(vehicles.size(), std::vector<double>(incidents.size()));

    const double infinity = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < vehicles.size(); i++) {
        for (size_t j = 0; j < incidents.size(); j++) {
            Route route = findRoute(network, vehicles.at(i).location, incidents.at(j).location);
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

void Dispatch::assignVehiclesToIncidents(const std::vector<std::vector<double>>& responseTimes) {
    std::vector<DispatchAssignment> bestAssignment;

    double bestTotalTime = std::numeric_limits<double>::infinity();

    std::vector<uint32_t> incidentOrder(incidents.size());

    for (uint32_t i = 0; i < incidents.size(); i++) {
        incidentOrder[i] = i;
    }

    do {
        double totalTime = 0.0;

        for (uint32_t vehicle = 0; vehicle < vehicles.size(); vehicle++) {
            uint32_t incident = incidentOrder.at(vehicle);

            totalTime += responseTimes.at(vehicle).at(incident);
        }

        if (totalTime < bestTotalTime) {
            bestTotalTime = totalTime;
            bestAssignment.clear();

            for (uint32_t vehicle = 0; vehicle < vehicles.size(); vehicle++) {
                uint32_t incident = incidentOrder.at(vehicle);
                bestAssignment.push_back({vehicle, incident, responseTimes.at(vehicle).at(incident)});
            }
        }
    } while (std::next_permutation(incidentOrder.begin(), incidentOrder.end()));

    // now set bestAssignment to actual assignment

    for (uint32_t vehicle = 0; vehicle < vehicles.size(); vehicle++) {
        vehicleAssignments[vehicle] = bestAssignment.at(vehicle);
        vehicles.at(vehicle).status = VehicleStatus::Dispatched;
    }
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
        std::cout << "\tSeverity: " << incidents.at(i).severity << "\n\n";
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