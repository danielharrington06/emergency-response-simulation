# Emergency Response Simulation

## Compile and Run
```bash
clang++ -std=c++17 \
    src/main.cpp \
    src/road_network.cpp \
    src/test_network.cpp \
    -framework Metal \
    -framework Foundation \
    -o bin/main
```