# Emergency Response Simulation

## A* Pathfinding: `router.cpp`
Heuristically minimises travel time. Achieves this by considering the pythagorean shortest distance (not geographicaly accurate) and maximum speed limit (which will later be adjusted to the maximum speed limit of the vehicle when we also update edge speeds).

$$
time \ (mins) = \frac{shortest \ distance \ (km)}{max \ speed \ (km/h)} * 60
$$

## Incident Assignment: `dispatch.cpp`
Assigns all high priority incidents, then all medium priority incidents, then all low priority incidents, minimising total time at each stage.

Assignment is done by minimising total time for incidents of the current severity for the available ambulances. A deliberate result of this is that a closer medium severity incident cannot be assigned over a further high severity incident. I considered using a combined metric to balance this, but thought that arbitrary weights would be further from the desired outcome at this point.

## Data
OpenStreetMap data was used. I used a location near to me.

## Project Structure
```vbnet
COMP0002-C-Coursework/
│
├── src/
│   ├── dispatch.cpp
│   ├── main.cpp
│   ├── road_network.cpp
│   ├── router.cpp
│   ├── test_dispatch.cpp
│   └── test_network.cpp
│
├── include/
│   ├── dispatch.hpp
│   ├── emergency_vehicle.hpp
│   ├── incident.hpp
│   ├── road_network.hpp
│   ├── route.hpp
│   ├── router.hpp
│   ├── test_dispatch.hpp
│   └── test_network.hpp
│
└── README.md
```

## Compile and Run
```bash
clang++ -std=c++17 \
    src/main.cpp \
    src/road_network.cpp \
    src/test_network.cpp \
    src/router.cpp \
    src/dispatch.cpp \
    src/test_dispatch.cpp \
    src/incident.cpp \
    src/emergency_vehicle.cpp \
    -framework Metal \
    -framework Foundation \
    -o bin/main
```