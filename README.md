# Emergency Response Simulation
The aim of this project was twofold: produce a simulation of an emergency response on a county scale and benchmark a complicated CPU algorithm against a GPU one

## A* Pathfinding: `router.cpp`
Heuristically minimises travel time. Achieves this by considering the pythagorean shortest distance (not geographicaly accurate) and maximum speed limit (which should later be adjusted to the maximum speed limit of the vehicle when we also update edge speeds to reflect the emergency vehicle).

$$
time \ (mins) = \frac{shortest \ distance \ (km)}{max \ speed \ (km/h)} * 60
$$

## Incident Assignment: `dispatch.cpp` and `assignment.cpp`
Assigns all high priority incidents, then all medium priority incidents, then all low priority incidents, minimising total time at each stage.A deliberate result of this is that a closer medium severity incident cannot be assigned over a further high severity incident. I considered using a combined metric to balance this, but thought that arbitrary weights would be further from the desired outcome at this point.

My first approach was an exhaustive one, maximising number of assignments, then minimising total time. However, this has time complexity $O(n!)$ and for 10 incidents and vehicles, took 1.7 seconds, for 11, takes 7 seconds, so is clearly intractable and unsuitable for this problem.

I then made use of the Hungarian Algorithm, again maximising the number of assignments then minimising time. This approach has time complexity $O(n^3)$ and is dwarfed by the route calculation.

## Metal GPU Implementation
For the graph network to work efficiently on the GPU, it needs fixed length arrays with reliable offsets rather than ragged `<vector>` objects as in the `road_network` implementation. `gpu_graph.cpp` formats the graph through a CSR conversion, to setup for the Metal GPU code.

`metal_router.mm` contains a routing algorithm, `findRouteGPU`, which takes the id's of source and target nodes and returns a `Route` struct. This struct is partially not filled as I found it unnecessary to compute the distance and exact route; if this were needed in the simulation (or real world), then it could be done once by the CPU A* implementation.

The `MetalRouter` is reused across multiple routing calls for efficiency.

### GPU Kernels
- relax_frontier: relaxes all outgoing edges for nodes on the frontier; uses integer travel times  which are $1000$ times the float values, so are adjusted and rounded here, then corrected when extracting the result (this is done because `atomic_fetch_min_explicit` was not working correctly on floats)
- reset_improved: resets the improved flags efficiently, these flags are used to populate the next frontier correctly
- prepare_dispatch: calculates the number of threadgroups required for the next frontier, giving 0 if it can safely exit early
- reset_routing_state: resets the `nextFrontierCount` and `nextFrontierMinTime`


### Key Performance Improvements
I initially started by having the CPU deploy each new frontier while the next frontier was not empty. Originally a single route computation took around 500-1000ms when the CPU computation was taking around 40ms, giving a recorded average 0.053x speedup (about 20 times slower).

Halfway through making performance improvements, I discovered a bug in the `process_osm.py` script which gave every road a 30mph speed limit. Fixing this gave the CPU a big performance boost (as the heuristic was now stronger), but no GPU improvement, so recorded GPU speedup fell

#### Performance Improvements:
- Moving the generation of the next frontier from CPU to GPU gave a 40% performance boost.

- Establishing a safe global termination which allowed the computation to end early once no frontier node had a current time less than the target's improved performance by a significant but not recorded amount (as at this stage I did not have the extensive benchmarking I since developed).

- Resetting 'improvement flags', flags that were used to determine whether a node had already been marked as improved (and therefore already in the next frontier) with a kernel gave a 20% performance boost.

- After discovering that just 6% of the GPU wall time was actual GPU execution time, moving all the iterative steps from te CPU to the GPU by utilising a max number of iterations and a kernel to check whether we could finish the computations early (but would still dryly run through the rest of the planned iterations) massively improved performance to a recorded 1.4x speedup.

- Discovering that the processed OSM data was all based on 30mph speed limits meant recorded GPU speedup fell to 1.15x.

- Changing `MAX_ITERATIONS` from 2500 to calculated value, dependent on the straightline distance between the source and target nodes improved performance to give a 1.5x GPU speedup. The formula is derived from regression data and exceeded the number of iterations 99% of 1000 random routes, without compromising correctness on any of the 1000. The exact formula works well for my map, but may need some adjustments for a denser or sparser map. The formula is given below.

$$iterations = min(MAX\_ITERATIONS, \ 50.25 \times distance + 708.74)$$

## Benchmarking
To give reliable results, benchmarking was carried out using a fixed seed.

### Route Benchmarking Results
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

### Assignment Benchmarking Results
Initial assignment benchmarking was carried out where when calculating the response times matrix, both the CPU and GPU used their respective routing algorithms.
```
========== Benchmark ==========
Task: Route Benchmark
2026-09-16 17:30:08
Vehicles: 120
Incidents: 120

CPU assignments: 118
GPU assignments: 118
Same number of assignments: YES
Exact Assignments match: NO

CPU assignment total: 952.145 min
GPU assignment total: 952.144 min
Assignment total difference: 0.0000617241 min
Same total assignment time: YES

CPU total: 88468.957 ms

GPU total: 58573.203 ms

GPU speedup: 1.510x
================================
```

## OpenStreetMap Data
OpenStreetMap is used for a real road network. I used my UK county so that I had familiarity and would have an easier time confirming correctness. To use OSM data, place the downloaded `.osm.pbf` file under `data/osm/`, create a `config.py` file with the format of `config.example.py` (with the correct filename) then process it with the python scripts.

## Python Scripts
`process_osm.py` produces the road network, represented by `nodes.csv` and `edges.csv`. 

`process_facilities.py` produces the facilities in `facilities.csv`, storing data on hospitals, ambulance stations, fire stations and police stations.

`show_map.py` takes a sequence of nodes from `data/processed/route.txt` and uses matplotlib to display the route.

`iter_dist_corr.py` finds a line of regression between the number of GPU iterations and the straight-line distance between the source and target nodes. I used this when determining the formula for proportional GPU iterations. I found a line of regression that meant only 1% of routes across 1000 random routes would not fit (and all of these still were correct as they discovered the correct route, but could not guarantee it).

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
│   ├── routing_one_to_many.metal
│   └── routing_one_to_one.metal
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

Point to Point GPU Algorithm
```bash
xcrun -sdk macosx metal -c \
    metal/routing.metal \ #make sure this is corrected
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

Point to Many GPU Algorithm
```bash
xcrun -sdk macosx metal -c \
    metal/routing.metal \ #make sure this is corrected
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