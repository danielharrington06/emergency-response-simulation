#pragma once

#include <cstdint>

enum class IncidentSeverity {
    Low = 1,
    Medium = 2,
    High = 3
};

struct Incident {
    uint32_t id;
    uint32_t location;
    IncidentSeverity severity;
};