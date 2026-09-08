#include "../include/incident.hpp"

#include <string>

std::string incidentSeverityToString(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::Low:
            return "Low";
        case IncidentSeverity::Medium:
            return "Medium";
        case IncidentSeverity::High:
            return "High";
    }
    
    return "Unknown";
}

std::string incidentTypeToString(IncidentType type) {
    switch (type) {
        case IncidentType::Medical:
            return "Medical";
        case IncidentType::Fire:
            return "Fire";
        case IncidentType::Crime:
            return "Fire";
    }

    return "Unknown";
}