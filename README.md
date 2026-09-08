# Emergency Response Simulation

## A* Pathfinding
Uses a pythagorean, shortest distance heuristic currently.

## Dispatch
Assigns all high priority incidents, then all medium priority incidents, then all low priority incidents, minimising total time at each stage.

However, this is currently inconsistent with the A* `router.cpp` which minimises distance.

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
    -framework Metal \
    -framework Foundation \
    -o bin/main
```