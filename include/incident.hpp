#pragma once

#include <cstdint>
#include <string>

enum class IncidentSeverity {
    Low = 1,
    Medium = 2,
    High = 3
};

enum class IncidentType {
    Medical,
    Fire,
    Crime
};

struct Incident {
    uint32_t id;
    uint32_t location;
    IncidentType type;
    IncidentSeverity severity;
};

std::string incidentTypeToString(IncidentType type);
std::string incidentSeverityToString(IncidentSeverity severity);