#include "simulation/SimControlInterface.h"
#include "simulation/MockSimController.h"

SimControlInterface* SimControlInterface::getInstance() {
    static MockSimController instance;
    return &instance;
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
