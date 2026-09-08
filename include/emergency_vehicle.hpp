#pragma once

#include <cstdint>
#include <string>

enum class VehicleType {
    Ambulance,
    FireEngine,
    PoliceCar
};

enum class VehicleStatus {
    Available,
    Dispatched
};

struct EmergencyVehicle {
    uint32_t id;
    uint32_t location;
    VehicleType type;
    VehicleStatus status;
};

std::string vehicleTypeToString(VehicleType type);
std::string vehicleStatusToString(VehicleStatus status);