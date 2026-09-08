#include "../include/test_dispatch.hpp"
#include "../include/dispatch.hpp"
#include "../include/road_network.hpp"

Dispatch createTestDispatch(RoadNetwork network) {
    Dispatch dispatch(network);

    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, IncidentSeverity::Low);
    dispatch.addIncident(4, IncidentSeverity::Medium);
    dispatch.addIncident(2, IncidentSeverity::High);

    return dispatch;
}