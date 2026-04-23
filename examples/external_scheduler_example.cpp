// external_scheduler_example.cpp
// Demonstrates external scheduler for behavior trees with 500ms tick interval
// Shows how to check BT status and implement continuous movement

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "behaviortree/BehaviorTreeScheduler.h"
#include "behaviortree/BehaviorTreeExecutor.h"
#include "simulation/MockSimController.h"

using namespace behaviortree;

// Helper function to convert status to string
const char* statusToString(BT::NodeStatus status) {
    switch (status) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

// Example 1: Simple external scheduler loop
void simpleExternalSchedulerExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Simple External Scheduler Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. Initialize simulation controller singleton
    MockSimController* simController = static_cast<MockSimController*>(SimControlInterface::getInstance());
    simController->setVerbose(true);

    // 2. Create behavior tree executor
    BehaviorTreeExecutor executor;
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize BT executor" << std::endl;
        return;
    }

    // 3. Load behavior tree XML (using AsyncMoveToPoint for continuous movement)
    if (!executor.loadFromFile("bt_xml/async_square_path.xml")) {
        std::cerr << "Failed to load behavior tree" << std::endl;
        return;
    }

    // 4. Create test entity
    VehicleID entityId = simController->addEntity("npc", 0, 0, 0);
    std::cout << "Created entity: vehicle=" << entityId.vehicle << std::endl;

    // 5. Get scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // 6. Create blackboard and set entity ID
    auto blackboard = BT::Blackboard::create();
    blackboard->set("vehicle_id", entityId);

    // 7. Create behavior tree using factory
    BT::Tree tree = executor.getFactory().createTree("AsyncSquarePath", blackboard);

    // 8. Register entity with scheduler, set tick interval to 500ms
    std::string entityKey = std::to_string(entityId.vehicle);
    if (!scheduler.registerEntityWithTreeAndInterval(entityKey, "AsyncSquarePath", std::move(tree), 500, blackboard)) {
        std::cerr << "Failed to register entity with scheduler" << std::endl;
        return;
    }

    std::cout << std::endl;
    std::cout << "=== Starting external scheduler (500ms interval) ===" << std::endl;
    std::cout << std::endl;

    // 9. External scheduler loop
    int tickCount = 0;
    while (true) {
        auto tickStart = std::chrono::steady_clock::now();
        tickCount++;

        // Execute one tick (schedule all registered entities)
        scheduler.tickAll();

        // Get and print current status
        auto info = scheduler.getEntityInfo(entityKey);
        std::cout << "[Tick " << tickCount << "] "
                  << "Entity: vehicle=" << entityId.vehicle
                  << " | Status: " << statusToString(info->lastStatus)
                  << " | Running: " << (info->isRunning ? "Yes" : "No")
                  << " | TickCount: " << info->tickCount
                  << std::endl;

        // If behavior tree completed, exit loop
        if (!info->isRunning) {
            std::cout << std::endl;
            std::cout << "Behavior tree completed with status: " << statusToString(info->lastStatus) << std::endl;
            break;
        }

        // Precise sleep for 500ms
        auto elapsed = std::chrono::steady_clock::now() - tickStart;
        auto sleepTime = std::chrono::milliseconds(500) - elapsed;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    // 10. Cleanup
    scheduler.unregisterEntity(entityKey);
    std::cout << std::endl;
    std::cout << "Example completed!" << std::endl;
}

// Example 2: Multi-entity scheduler
void multiEntitySchedulerExample() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    Multi-Entity Scheduler Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. Initialize
    MockSimController* simController = static_cast<MockSimController*>(SimControlInterface::getInstance());
    simController->setVerbose(false);  // Reduce output

    BehaviorTreeExecutor executor;
    executor.initialize();
    executor.loadFromFile("bt_xml/async_square_path.xml");

    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // 2. Create multiple entities
    std::vector<VehicleID> entityIds;
    std::vector<std::string> entityKeys;
    for (int i = 0; i < 3; ++i) {
        VehicleID entityId = simController->addEntity("npc_" + std::to_string(i), i * 5, 0, 0);
        entityIds.push_back(entityId);
        std::string entityKey = std::to_string(entityId.vehicle);
        entityKeys.push_back(entityKey);

        auto blackboard = BT::Blackboard::create();
        blackboard->set("vehicle_id", entityId);

        BT::Tree tree = executor.getFactory().createTree("AsyncSquarePath", blackboard);

        // Each entity can have different tick interval
        int interval = 500 + i * 100;  // 500ms, 600ms, 700ms
        scheduler.registerEntityWithTreeAndInterval(entityKey, "AsyncSquarePath", std::move(tree), interval, blackboard);

        std::cout << "Created entity: vehicle=" << entityId.vehicle << " with tick interval: " << interval << "ms" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Starting multi-entity scheduler ===" << std::endl;
    std::cout << std::endl;

    // 3. Scheduler loop
    int tickCount = 0;
    while (true) {
        auto tickStart = std::chrono::steady_clock::now();
        tickCount++;

        // Tick all entities
        scheduler.tickAll();

        // Print all entity statuses
        std::cout << "[Tick " << tickCount << "] ";
        bool allCompleted = true;
        for (size_t i = 0; i < entityKeys.size(); ++i) {
            auto status = scheduler.getEntityStatus(entityKeys[i]);
            std::cout << "vehicle=" << entityIds[i].vehicle << ":" << statusToString(status) << " ";
            if (status == BT::NodeStatus::RUNNING) {
                allCompleted = false;
            }
        }
        std::cout << std::endl;

        if (allCompleted) {
            std::cout << std::endl;
            std::cout << "All entities completed!" << std::endl;
            break;
        }

        // Sleep 500ms
        auto elapsed = std::chrono::steady_clock::now() - tickStart;
        auto sleepTime = std::chrono::milliseconds(500) - elapsed;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    // 4. Cleanup
    for (const auto& entityKey : entityKeys) {
        scheduler.unregisterEntity(entityKey);
    }
    std::cout << std::endl;
    std::cout << "Multi-entity example completed!" << std::endl;
}

// Example 3: Game engine integration example
class GameEngineSimulator {
public:
    GameEngineSimulator() : running_(false) {}

    bool initialize() {
        // Get simulation controller singleton
        simController_ = static_cast<MockSimController*>(SimControlInterface::getInstance());
        simController_->setVerbose(false);

        // Create behavior tree executor
        btExecutor_.reset(new BehaviorTreeExecutor());
        if (!btExecutor_->initialize()) {
            std::cerr << "Failed to initialize BT executor" << std::endl;
            return false;
        }

        // Load behavior tree
        if (!btExecutor_->loadFromFile("bt_xml/async_square_path.xml")) {
            std::cerr << "Failed to load behavior tree" << std::endl;
            return false;
        }

        std::cout << "[GameEngine] Initialized successfully" << std::endl;
        return true;
    }

    std::string spawnEntityAndStartBT(const std::string& treeName, double x, double y, double z) {
        // Create entity
        VehicleID entityId = simController_->addEntity("npc", x, y, z);
        std::string entityKey = std::to_string(entityId.vehicle);

        // Create blackboard
        auto blackboard = BT::Blackboard::create();
        blackboard->set("vehicle_id", entityId);

        // Create and register behavior tree
        BT::Tree tree = btExecutor_->getFactory().createTree(treeName, blackboard);
        BehaviorTreeScheduler::getInstance().registerEntityWithTreeAndInterval(
            entityKey, treeName, std::move(tree), 500, blackboard);

        std::cout << "[GameEngine] Spawned entity vehicle=" << entityId.vehicle << " with behavior tree " << treeName << std::endl;
        return entityKey;
    }

    void run() {
        running_ = true;
        std::cout << "[GameEngine] Main loop started (500ms tick interval)" << std::endl;

        auto lastTickTime = std::chrono::steady_clock::now();
        int frameCount = 0;

        while (running_) {
            auto frameStart = std::chrono::steady_clock::now();

            // ========== Other game engine updates ==========
            updatePhysics();
            updateGameLogic();
            // ==============================================

            // ========== Check if need to tick behavior tree ==========
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                frameStart - lastTickTime);

            if (elapsed.count() >= 500) {
                frameCount++;

                // Manually tick behavior tree
                BehaviorTreeScheduler::getInstance().tickAll();
                lastTickTime = frameStart;

                // Print status
                if (frameCount % 5 == 0) {  // Print every 5 ticks
                    printEntityStatus();
                }
            }
            // ======================================================

            // Check if all behavior trees completed
            if (frameCount > 0 && BehaviorTreeScheduler::getInstance().getRegisteredEntityCount() == 0) {
                std::cout << "[GameEngine] All behavior trees completed" << std::endl;
                break;
            }

            // Small sleep to avoid high CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[GameEngine] Main loop ended" << std::endl;
    }

    void stop() {
        running_ = false;
    }

private:
    void updatePhysics() {
        // Simulate physics update
    }

    void updateGameLogic() {
        // Simulate game logic update
    }

    void printEntityStatus() {
        auto& scheduler = BehaviorTreeScheduler::getInstance();
        auto entityIds = scheduler.getRegisteredEntityIds();

        std::cout << "[GameEngine] Active entities: " << entityIds.size() << " | ";
        for (const auto& entityId : entityIds) {
            auto status = scheduler.getEntityStatus(entityId);
            std::cout << entityId << ":" << statusToString(status) << " ";
        }
        std::cout << std::endl;
    }

    MockSimController* simController_;
    std::unique_ptr<BehaviorTreeExecutor> btExecutor_;
    std::atomic<bool> running_;
};

void gameEngineIntegrationExample() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    Game Engine Integration Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    GameEngineSimulator engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize game engine" << std::endl;
        return;
    }

    // Create multiple entities and start behavior trees
    engine.spawnEntityAndStartBT("AsyncSquarePath", 0, 0, 0);
    engine.spawnEntityAndStartBT("AsyncSquarePath", 5, 0, 0);

    // Run game main loop
    engine.run();

    std::cout << std::endl;
    std::cout << "Game engine example completed!" << std::endl;
}

int main(int argc, char* argv[]) {
    // Run Example 1: Simple external scheduler
    simpleExternalSchedulerExample();

    std::cout << std::endl;
    std::cout << "Press Enter to continue to multi-entity example..." << std::endl;
    std::cin.get();

    // Run Example 2: Multi-entity scheduler
    multiEntitySchedulerExample();

    std::cout << std::endl;
    std::cout << "Press Enter to continue to game engine integration example..." << std::endl;
    std::cin.get();

    // Run Example 3: Game engine integration
    gameEngineIntegrationExample();

    return 0;
}
