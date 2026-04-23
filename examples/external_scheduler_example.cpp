// external_scheduler_example.cpp
// This example demonstrates how to use EntityScriptManager for behavior tree execution
// BehaviorTreeScheduler has been removed - use EntityScriptManager instead

#include <iostream>
#include <thread>
#include <chrono>
#include "simulation/MockSimController.h"

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "    EntityScriptManager BT Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: BehaviorTreeScheduler has been removed." << std::endl;
    std::cout << "Use EntityScriptManager with BTScript instead." << std::endl;
    std::cout << std::endl;

    // Initialize simulation controller
    MockSimController* simController = static_cast<MockSimController*>(SimControlInterface::getInstance());
    simController->setVerbose(true);

    // Create an entity
    VehicleID entityId = simController->addEntity("npc", 0, 0, 0);
    std::cout << "Created entity: vehicle=" << entityId.vehicle << std::endl;

    std::cout << std::endl;
    std::cout << "EntityScriptManager is ready for adding BT scripts." << std::endl;
    std::cout << "Use addBTScript() to add behavior tree scripts." << std::endl;
    std::cout << std::endl;

    return 0;
}
