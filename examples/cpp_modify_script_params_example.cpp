// cpp_modify_script_params_example.cpp
// Demonstrates how to modify Lua script parameters from C++

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include "scripting/EntityScriptManager.h"
#include "scripting/LuaSimBinding.h"
#include "simulation/MockSimController.h"

using namespace scripting;

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  C++ Modify Script Parameters Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize Lua environment
    if (!LuaSimBinding::getInstance().initialize()) {
        std::cerr << "Failed to initialize Lua binding" << std::endl;
        return 1;
    }

    // Get simulation controller instance
    MockSimController* sim = static_cast<MockSimController*>(SimControlInterface::getInstance());
    sim->setVerbose(false);

    // Create test entity
    VehicleID vid = sim->addEntity("patrol_guard", 0, 0, 0);
    std::string entityId = std::to_string(vid.vehicle);
    std::cout << "Created entity: vehicle=" << vid.vehicle << std::endl;

    // Get script manager (auto-created)
    auto manager = sim->createScriptManager(entityId);
    if (!manager) {
        std::cerr << "Failed to create script manager" << std::endl;
        return 1;
    }

    // Add patrol script
    bool success = manager->addTacticalScriptFromFile("patrol", 
        "scripts/examples/patrol_with_dynamic_waypoints.lua");
    if (!success) {
        std::cerr << "Failed to add patrol script: " << manager->getLastError() << std::endl;
        return 1;
    }
    std::cout << "Added patrol script successfully" << std::endl;

    // Method 1: Use setScriptWaypoints to set waypoints (dedicated interface)
    std::vector<std::tuple<double, double, double>> waypoints;
    waypoints.push_back(std::make_tuple(0.0, 0.0, 0.0));
    waypoints.push_back(std::make_tuple(10.0, 0.0, 0.0));
    waypoints.push_back(std::make_tuple(10.0, 10.0, 0.0));
    waypoints.push_back(std::make_tuple(0.0, 10.0, 0.0));
    manager->setScriptWaypoints("patrol", waypoints);
    std::cout << "Set initial waypoints (square path)" << std::endl;

    // Method 2: Use setScriptParam to set other parameters
    sol::state& lua = manager->getLuaState();
    manager->setScriptParam("patrol", "patrol_speed", sol::make_object(lua, 2.0));
    manager->setScriptParam("patrol", "loop_mode", sol::make_object(lua, "cycle"));
    std::cout << "Set patrol parameters: speed=2.0, mode=cycle" << std::endl;

    // Method 3: Use setEntityField to set shared data (visible to all scripts)
    manager->setEntityField("alert_level", sol::make_object(lua, 0));
    std::cout << "Set entity field: alert_level=0" << std::endl;

    std::cout << std::endl;
    std::cout << "Running patrol for 3 seconds..." << std::endl;
    std::cout << std::endl;

    // Run for 3 seconds
    auto startTime = std::chrono::steady_clock::now();
    int frameCount = 0;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (elapsed.count() >= 3) break;

        // Execute scripts
        manager->executeAllScripts();
        
        // Note: Simulation update is handled internally by the controller

        frameCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    std::cout << "Executed " << frameCount << " frames" << std::endl;

    // Read script state
    auto currentIdx = manager->getScriptParam("patrol", "current_index");
    if (currentIdx) {
        std::cout << "Final waypoint index: " << currentIdx.value().as<int>() << std::endl;
    }

    auto patrolCount = manager->getScriptParam("patrol", "patrol_count");
    if (patrolCount) {
        std::cout << "Completed patrol loops: " << patrolCount.value().as<int>() << std::endl;
    }

    // Test getScriptWaypoints
    auto readWaypoints = manager->getScriptWaypoints("patrol");
    std::cout << "Read back " << readWaypoints.size() << " waypoints" << std::endl;

    std::cout << std::endl;
    std::cout << "Example completed!" << std::endl;
    std::cout << std::endl;
    std::cout << "Summary of APIs demonstrated:" << std::endl;
    std::cout << "  - setScriptWaypoints() / getScriptWaypoints()" << std::endl;
    std::cout << "  - setScriptParam() / getScriptParam()" << std::endl;
    std::cout << "  - setEntityField() / getEntityField()" << std::endl;
    std::cout << std::endl;

    return 0;
}
