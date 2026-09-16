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

## Metal GPU Implementation
For the graph network to work efficiently on the GPU, it needs fixed length arrays with reliable offsets rather than ragged `<vector>` objects as in the `road_network` implementation. `gpu_graph.cpp` formats the graph through a CSR conversion, to setup for the Metal GPU code.

### GPU Performance Improvements and Findings
- GPU kernel processing the next frontier (750->530ms)
- establishing a safe global termination: made GPU 0.15x speedup
- resetting flags through a kernel: made GPU 0.18x speedup
- discovered that roughly 94% of the processing time is outside the GPU execution itself, so moved all the iterative steps onto the GPU by giving max iterations and a way to check if it could finish early
- some routes were incorrect compared to the CPU, this was because they were not given enough iterations
- also discovered the processed osm data was entirely 30mph speed limits, so corrected this which gave the CPU a big performance boost as the heuristic was stronger, so GPU speedup fell
- changing MAX_ITERATIONS from 2000 to a max number of iterations proportionate to straightline distance between the source and target node(covering up to 99th percentile) gave 1.15x -> 1.5x GPU speedup improvement which works for my map but might change for dnser or sparser maps. $iterations = min(MAX_ITERATIONS, 50.25 \times distance + 708.74)$

## Benchmarking
To give reliable results, benchmarking should be carried out on the same seed.

### Route (point a to point b) Benchmarking Results
```
Proportionate Max Iterations
========== Benchmark ==========
2026-09-16 15:01:53
Routes: 1000
Successful routes: 979
Matching CPU/GPU results: 1000/1000

CPU total: 27616.464 ms
CPU average: 27.616 ms/route

GPU total: 17584.946 ms
GPU average: 17.585 ms/route

GPU speedup: 1.570x
================================
```

## Project Structure
```vbnet
COMP0002-C-Coursework/
├── data/
│   ├── osm/
│       └── county.osm.pbf
│   ├── processed/
│       ├── edges.csv
│       ├── facilities.csv
│       └── nodes.csv
│
├── include/
│   ├── assignment.hpp
│   ├── dispatch.hpp
│   ├── emergency-incident.hpp
│   ├── gpu_graph.hpp
│   ├── incident.hpp
│   ├── metal_router.hpp
│   ├── osm_loader.hpp
│   ├── road_network.hpp
│   ├── route_benchmark.hpp
│   ├── route.hpp
│   ├── router.hpp
│   └── scenario_generator.hpp
│
├── metal/
│   └── routing.metal
│
├── scripts/
│   ├── config.exmaple.py
│   ├── config.py
│   ├── process_facilities.py
│   ├── process_osm.py
│   └── show_map.py
│
├── src/
│   ├── assignment.cpp
│   ├── dispatch.cpp
│   ├── emergency-incident.cpp
│   ├── gpu_graph.cpp
│   ├── incident.cpp
│   ├── main.cpp
│   ├── metal_router.mm
│   ├── osm_loader.cpp
│   ├── road_network.cpp
│   ├── route_benchmark.cpp
│   ├── router.cpp
│   └── scenario_generator.cpp
│
└── README.md
```

## Compile and Run
```bash
xcrun -sdk macosx metal -c \
    metal/routing.metal \
    -o bin/routing.air

xcrun -sdk macosx metallib \
    bin/routing.air \
    -o bin/routing.metallib

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
    src/benchmark_route.cpp \
    src/benchmark_assignment.cpp \
    -framework Metal \
    -framework Foundation \
    -o bin/main
```