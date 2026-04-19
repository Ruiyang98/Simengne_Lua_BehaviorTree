#include "simulation/SimControlInterface.h"

namespace simulation {

std::string SimControlInterface::stateToString(SimState state) {
    switch (state) {
        case STOPPED:
            return "STOPPED";
        case RUNNING:
            return "RUNNING";
        case PAUSED:
            return "PAUSED";
        default:
            return "UNKNOWN";
    }
}

} // namespace simulation
