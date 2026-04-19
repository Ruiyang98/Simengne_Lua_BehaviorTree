#ifndef SIM_CONTROLLER_PTR_H
#define SIM_CONTROLLER_PTR_H

#include "simulation/SimControlInterface.h"
#include <memory>

namespace behaviortree {

// Global shared SimControlInterface pointer
// Used to access simulation controller in behavior tree nodes
extern simulation::SimControlInterface* g_simController;

// Set global simulation controller pointer
inline void setSimController(simulation::SimControlInterface* controller) {
    g_simController = controller;
}

// Get global simulation controller pointer
inline simulation::SimControlInterface* getSimController() {
    return g_simController;
}

} // namespace behaviortree

#endif // SIM_CONTROLLER_PTR_H
