# Emergency Response Simulation

## A* Pathfinding
Uses a pythagorean, shortest distance heuristic currently.

## Dispatch
Assigns all high priority incidents, then all medium priority incidents, then all low priority incidents, minimising total time at each stage.

However, this is currently inconsistent with the A* `router.cpp` which minimises distance.

## Project Structure


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