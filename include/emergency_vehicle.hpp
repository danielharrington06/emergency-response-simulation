#include <cstdint>

enum class VehicleType {
    Ambulance
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