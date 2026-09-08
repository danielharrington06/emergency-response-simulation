#include "../include/emergency_vehicle.hpp"

#include <string>

std::string vehicleTypeToString(VehicleType type) {

    switch (type) {
        case VehicleType::Ambulance:
            return "Ambulance";
        case VehicleType::FireEngine:
            return "Fire Engine";
        case VehicleType::PoliceCar:
            return "Police Car";
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