#include "../include/test_dispatch.hpp"
#include "../include/dispatch.hpp"

Dispatch createTestDispatch() {
    Dispatch dispatch;

    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, 1);
    dispatch.addIncident(4, 2);
    dispatch.addIncident(2, 3);

    return dispatch;
}