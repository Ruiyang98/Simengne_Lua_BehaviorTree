#include "simulation/SimControlInterface.h"

// Initialize static member
SimControlInterface* SimControlInterface::instance_ = nullptr;

SimControlInterface* SimControlInterface::getInstance() {
    return instance_;
}

void SimControlInterface::setInstance(SimControlInterface* instance) {
    instance_ = instance;
}

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
