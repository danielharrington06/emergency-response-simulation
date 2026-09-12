#pragma once

#include <cstddef>
#include <vector>

struct Assignment {
    std::size_t vehicleIndex;
    std::size_t incidentIndex;
};

std::vector<Assignment> findOptimalAssignment(const std::vector<std::vector<double>>& costs);