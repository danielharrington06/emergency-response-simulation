#include "../include/test_dispatch.hpp"
#include "../include/dispatch.hpp"

Dispatch createTestDispatchEqual(RoadNetwork network) {
    Dispatch dispatch(network);

    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, IncidentSeverity::Low);
    dispatch.addIncident(4, IncidentSeverity::Medium);
    dispatch.addIncident(2, IncidentSeverity::High);

    return dispatch;
}

Dispatch createMoreVehiclesTest(RoadNetwork network) {

    Dispatch dispatch(network);

    // 4 ambulances, 2 incidents
    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(4, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, IncidentSeverity::High);
    dispatch.addIncident(2, IncidentSeverity::Medium);

    return dispatch;
}

Dispatch createMoreIncidentsTest(RoadNetwork network) {

    Dispatch dispatch(network);

    // 2 ambulances, 4 incidents
    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, IncidentSeverity::High);
    dispatch.addIncident(2, IncidentSeverity::High);
    dispatch.addIncident(4, IncidentSeverity::Medium);
    dispatch.addIncident(1, IncidentSeverity::Low);

    return dispatch;
}

Dispatch createPriorityCutoffTest(RoadNetwork network) {

    Dispatch dispatch(network);

    // 2 ambulances
    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);

    // 3 High, 3 Medium, 3 Low
    dispatch.addIncident(3, IncidentSeverity::High);
    dispatch.addIncident(2, IncidentSeverity::High);
    dispatch.addIncident(5, IncidentSeverity::High);

    dispatch.addIncident(4, IncidentSeverity::Medium);
    dispatch.addIncident(1, IncidentSeverity::Medium);
    dispatch.addIncident(0, IncidentSeverity::Medium);

    dispatch.addIncident(2, IncidentSeverity::Low);
    dispatch.addIncident(3, IncidentSeverity::Low);
    dispatch.addIncident(5, IncidentSeverity::Low);

    return dispatch;
}


Dispatch createSingleHighTest(RoadNetwork network) {

    Dispatch dispatch(network);

    // 3 ambulances, 1 high-priority incident
    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    dispatch.addIncident(3, IncidentSeverity::High);

    return dispatch;
}


Dispatch createNoIncidentsTest(RoadNetwork network) {

    Dispatch dispatch(network);

    dispatch.addVehicle(0, VehicleType::Ambulance);
    dispatch.addVehicle(1, VehicleType::Ambulance);
    dispatch.addVehicle(5, VehicleType::Ambulance);

    return dispatch;
}


Dispatch createNoVehiclesTest(RoadNetwork network) {

    Dispatch dispatch(network);

    dispatch.addIncident(3, IncidentSeverity::High);
    dispatch.addIncident(4, IncidentSeverity::Medium);
    dispatch.addIncident(2, IncidentSeverity::Low);

    return dispatch;
}