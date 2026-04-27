// obstacle_avoidance_example.cpp
// Demonstrates obstacle avoidance behavior tree with user path following

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include "behaviortree/BehaviorTreeExecutor.h"
#include "simulation/MockSimController.h"
#include "scripting/LuaSimBinding.h"

using namespace behaviortree;
using namespace scripting;

// Helper function to convert VehicleID to string
std::string vehicleIdToString(const VehicleID& vid) {
    return std::to_string(vid.vehicle);
}

// Example 1: Basic obstacle avoidance with user path
void basicExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Example 1: Basic Obstacle Avoidance" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize Lua environment
    if (!LuaSimBinding::getInstance().initialize()) {
        std::cerr << "Failed to initialize Lua binding" << std::endl;
        return;
    }

    // Create simulation controller
    MockSimController* sim = static_cast<MockSimController*>(SimControlInterface::getInstance());
    sim->setVerbose(false);

    // Create behavior tree executor
    BehaviorTreeExecutor& executor = BehaviorTreeExecutor::getInstance();
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize BT executor" << std::endl;
        return;
    }

    // Load obstacle avoidance behavior tree
    if (!executor.loadFromFile("bt_xml/obstacle_avoidance.xml")) {
        std::cerr << "Failed to load behavior tree XML" << std::endl;
        return;
    }

    // Create test entity
    VehicleID vid = sim->addEntity("test_entity", 0, 0, 0);
    std::string entityId = vehicleIdToString(vid);
    std::cout << "Created entity: vehicle=" << vid.vehicle << std::endl;

    // Create blackboard and set parameters
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);
    
    // Set user path (format: "x1,y1;x2,y2;x3,y3")
    std::string userPath = "0,0;10,0;10,10;0,10";
    blackboard->set("user_path", userPath);
    std::cout << "Set user path: " << userPath << std::endl;

    // Execute behavior tree
    std::cout << "Started behavior tree execution" << std::endl;
    std::cout << std::endl;
    std::cout << "Running for 5 seconds..." << std::endl;
    std::cout << std::endl;

    // Run for 5 seconds
    auto startTime = std::chrono::steady_clock::now();
    int tickCount = 0;
    BT::NodeStatus status = BT::NodeStatus::IDLE;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (elapsed.count() >= 5) break;

        // Execute behavior tree
        status = executor.execute("ObstacleAvoidanceTree", blackboard);
        tickCount++;

        // Print status every 10 ticks
        if (tickCount % 10 == 0) {
            std::cout << "Tick " << tickCount << " - Status: ";
            switch (status) {
                case BT::NodeStatus::RUNNING: std::cout << "RUNNING"; break;
                case BT::NodeStatus::SUCCESS: std::cout << "SUCCESS"; break;
                case BT::NodeStatus::FAILURE: std::cout << "FAILURE"; break;
                default: std::cout << "OTHER"; break;
            }
            std::cout << std::endl;
        }

        // Small delay between ticks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    std::cout << "Example 1 completed!" << std::endl;
    std::cout << std::endl;
}

// Example 2: Obstacle avoidance with dynamic obstacle
void dynamicObstacleExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Example 2: Dynamic Obstacle" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize
    if (!LuaSimBinding::getInstance().isInitialized()) {
        LuaSimBinding::getInstance().initialize();
    }

    MockSimController* sim = static_cast<MockSimController*>(SimControlInterface::getInstance());
    
    // Create behavior tree executor
    BehaviorTreeExecutor& executor = BehaviorTreeExecutor::getInstance();
    executor.initialize();
    executor.loadFromFile("bt_xml/obstacle_avoidance.xml");

    // Create main entity
    VehicleID mainVid = sim->addEntity("main_entity", 0, 0, 0);
    std::string mainEntityId = vehicleIdToString(mainVid);
    std::cout << "Created main entity: vehicle=" << mainVid.vehicle << std::endl;

    // Create obstacle entity (placed in front of main entity)
    VehicleID obstacleVid = sim->addEntity("obstacle", 5, 0, 0);
    std::cout << "Created obstacle entity: vehicle=" << obstacleVid.vehicle 
              << " at position (5, 0)" << std::endl;

    // Create blackboard with user path
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", mainEntityId);
    blackboard->set("user_path", "10,0;10,10;0,10");

    std::cout << "Started behavior tree with obstacle in front" << std::endl;
    std::cout << std::endl;
    std::cout << "Running for 5 seconds..." << std::endl;
    std::cout << "Entity should detect obstacle and avoid it by moving 90 degrees" << std::endl;
    std::cout << std::endl;

    // Run for 5 seconds
    auto startTime = std::chrono::steady_clock::now();
    int tickCount = 0;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (elapsed.count() >= 5) break;

        executor.execute("ObstacleAvoidanceTree", blackboard);
        tickCount++;

        if (tickCount % 5 == 0) {
            std::cout << "Tick " << tickCount << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    std::cout << "Example 2 completed!" << std::endl;
    std::cout << std::endl;
}

// Example 3: No user path (should idle)
void noPathExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Example 3: No User Path (Idle)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize
    if (!LuaSimBinding::getInstance().isInitialized()) {
        LuaSimBinding::getInstance().initialize();
    }

    MockSimController* sim = static_cast<MockSimController*>(SimControlInterface::getInstance());
    
    // Create behavior tree executor
    BehaviorTreeExecutor& executor = BehaviorTreeExecutor::getInstance();
    executor.initialize();
    executor.loadFromFile("bt_xml/obstacle_avoidance.xml");

    // Create entity
    VehicleID vid = sim->addEntity("idle_entity", 0, 0, 0);
    std::string entityId = vehicleIdToString(vid);
    std::cout << "Created entity: vehicle=" << vid.vehicle << std::endl;

    // Create blackboard WITHOUT user path
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);
    // Note: No user_path set, entity should idle

    std::cout << "Started behavior tree WITHOUT user path" << std::endl;
    std::cout << "Entity should stay idle" << std::endl;
    std::cout << std::endl;

    // Run for 3 seconds
    auto startTime = std::chrono::steady_clock::now();
    int tickCount = 0;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (elapsed.count() >= 3) break;

        executor.execute("ObstacleAvoidanceTree", blackboard);
        tickCount++;

        if (tickCount % 5 == 0) {
            std::cout << "Tick " << tickCount << " - Entity should be idle" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    std::cout << "Example 3 completed!" << std::endl;
    std::cout << std::endl;
}

// Example 4: Dynamic path update
void dynamicPathUpdateExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Example 4: Dynamic Path Update" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize
    if (!LuaSimBinding::getInstance().isInitialized()) {
        LuaSimBinding::getInstance().initialize();
    }

    MockSimController* sim = static_cast<MockSimController*>(SimControlInterface::getInstance());
    
    // Create behavior tree executor
    BehaviorTreeExecutor& executor = BehaviorTreeExecutor::getInstance();
    executor.initialize();
    executor.loadFromFile("bt_xml/obstacle_avoidance.xml");

    // Create entity
    VehicleID vid = sim->addEntity("dynamic_entity", 0, 0, 0);
    std::string entityId = vehicleIdToString(vid);
    std::cout << "Created entity: vehicle=" << vid.vehicle << std::endl;

    // Create blackboard with initial path
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);
    blackboard->set("user_path", "5,0;5,5");
    std::cout << "Set initial path: 5,0;5,5" << std::endl;

    std::cout << "Started behavior tree" << std::endl;
    std::cout << std::endl;

    // Run for 6 seconds, then update path
    auto startTime = std::chrono::steady_clock::now();
    int tickCount = 0;
    bool pathUpdated = false;

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime);
        
        if (elapsed.count() >= 6) break;

        // Update path after 3 seconds
        if (elapsed.count() >= 3 && !pathUpdated) {
            std::cout << std::endl;
            std::cout << "[C++] Updating user path to: 10,0;10,10;0,10" << std::endl;
            blackboard->set("user_path", "10,0;10,10;0,10");
            pathUpdated = true;
            std::cout << std::endl;
        }

        executor.execute("ObstacleAvoidanceTree", blackboard);
        tickCount++;

        if (tickCount % 5 == 0) {
            std::cout << "Tick " << tickCount << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl;
    std::cout << "Example 4 completed!" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Obstacle Avoidance Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrates obstacle avoidance" << std::endl;
    std::cout << "behavior tree with user path following." << std::endl;
    std::cout << std::endl;
    std::cout << "Logic:" << std::endl;
    std::cout << "  1. If obstacle detected -> Avoid (90 degrees)" << std::endl;
    std::cout << "  2. If user path exists -> Follow path" << std::endl;
    std::cout << "  3. Otherwise -> Idle" << std::endl;
    std::cout << std::endl;

    // Run Example 1
    basicExample();

    std::cout << "Press Enter to continue to Example 2..." << std::endl;
    std::cin.get();

    // Run Example 2
    dynamicObstacleExample();

    std::cout << "Press Enter to continue to Example 3..." << std::endl;
    std::cin.get();

    // Run Example 3
    noPathExample();

    std::cout << "Press Enter to continue to Example 4..." << std::endl;
    std::cin.get();

    // Run Example 4
    dynamicPathUpdateExample();

    std::cout << "========================================" << std::endl;
    std::cout << "  All examples completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Files created:" << std::endl;
    std::cout << "  - scripts/obstacle_avoidance_nodes.lua" << std::endl;
    std::cout << "  - bt_xml/obstacle_avoidance.xml" << std::endl;
    std::cout << "  - examples/obstacle_avoidance_example.cpp" << std::endl;
    std::cout << std::endl;

    return 0;
}
