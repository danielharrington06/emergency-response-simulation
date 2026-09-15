# Emergency Response Simulation

## A* Pathfinding: `router.cpp`
Heuristically minimises travel time. Achieves this by considering the pythagorean shortest distance (not geographicaly accurate) and maximum speed limit (which will later be adjusted to the maximum speed limit of the vehicle when we also update edge speeds).

$$
time \ (mins) = \frac{shortest \ distance \ (km)}{max \ speed \ (km/h)} * 60
$$

## Incident Assignment: `dispatch.cpp` and `assignment.cpp`
Assigns all high priority incidents, then all medium priority incidents, then all low priority incidents, minimising total time at each stage.

Assignment is done by minimising total time for incidents of the current severity for the available ambulances. A deliberate result of this is that a closer medium severity incident cannot be assigned over a further high severity incident. I considered using a combined metric to balance this, but thought that arbitrary weights would be further from the desired outcome at this point.

My first approach was an exhaustive one, maximising number of assignments, then minimising total time. However, this has time complexity $O(n!)$ and for 10 incidents and vehicles, takes 1.7 seconds, for 11, takes 7 seconds, so is clearly intractable and unsuitable for this problem.

The current approach makes use of the Hungarian Algorithm, again maximising the number of assignments then minimising time. This approach has time complexity $O(n^3)$.

## OpenStreetMap Data
OpenStreetMap is used for a real road network. I used a county near me in the UK. To use OSM data, place the `.osm.pbf` file under `data/osm/`, create a `config.py` file with the format of c`onfig.example.py` (with the correct filename) then process it with the python scripts.

`process_osm.py` produces the road network, represented by `nodes.csv` and `edges.csv`. `process_facilities.py` produced the facilities in `facilities.csv`, storing data on hospitals, ambulance stations, fire stations and police stations.

## Metal GPU Implementation
For the graph network to work efficiently on the GPU, it needs fixed length arrays with reliable offsets rather than ragged `<vector>` objects as in the `road_network` implementation. `gpu_graph.cpp` formats the graph through a CSR conversion, to setup for the Metal GPU code.

## Benchmarking
To give reliable results, benchmarking should be carried out on the same seed.

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
    src/router.cpp \
    src/dispatch.cpp \
    src/incident.cpp \
    src/emergency_vehicle.cpp \
    src/osm_loader.cpp \
    src/scenario_generator.cpp \
    src/assignment.cpp \
    src/gpu_graph.cpp \
    src/metal_router.mm \
    -framework Metal \
    -framework Foundation \
    -o bin/main
```