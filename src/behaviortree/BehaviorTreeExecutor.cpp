#include "behaviortree/BehaviorTreeExecutor.h"
#include "behaviortree/SimControllerPtr.h"
#include "behaviortree/MoveToPoint.h"
#include "behaviortree/FollowPath.h"
#include "behaviortree/CheckEntityExists.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace behaviortree {

// Define global SimControlInterface pointer
simulation::SimControlInterface* g_simController = nullptr;

BehaviorTreeExecutor::BehaviorTreeExecutor(simulation::SimControlInterface* simController)
    : simController_(simController)
    , initialized_(false)
{
    // Set global pointer
    setSimController(simController);
}

BehaviorTreeExecutor::~BehaviorTreeExecutor() {
    // Clear global pointer
    if (g_simController == simController_) {
        setSimController(nullptr);
    }
}

bool BehaviorTreeExecutor::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!simController_) {
        lastError_ = "SimController is null";
        return false;
    }
    
    try {
        registerNodes();
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to initialize: ") + e.what();
        return false;
    }
}

void BehaviorTreeExecutor::registerNodes() {
    // Register MoveToPoint node
    factory_.registerNodeType<MoveToPoint>("MoveToPoint");
    
    // Register FollowPath node
    factory_.registerNodeType<FollowPath>("FollowPath");
    
    // Register CheckEntityExists node
    factory_.registerNodeType<CheckEntityExists>("CheckEntityExists");
    
    std::cout << "[BehaviorTreeExecutor] Registered custom nodes:" << std::endl;
    std::cout << "  - MoveToPoint" << std::endl;
    std::cout << "  - FollowPath" << std::endl;
    std::cout << "  - CheckEntityExists" << std::endl;
}

bool BehaviorTreeExecutor::loadFromFile(const std::string& xmlFile) {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }
    
    try {
        factory_.registerBehaviorTreeFromFile(xmlFile);
        std::cout << "[BehaviorTreeExecutor] Loaded behavior tree from: " << xmlFile << std::endl;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML file: ") + e.what();
        return false;
    }
}

bool BehaviorTreeExecutor::loadFromText(const std::string& xmlText) {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }
    
    try {
        factory_.registerBehaviorTreeFromText(xmlText);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML text: ") + e.what();
        return false;
    }
}

BT::NodeStatus BehaviorTreeExecutor::execute(const std::string& treeName, 
                                              BT::Blackboard::Ptr blackboard) {
    if (!initialized_) {
        lastError_ = "Executor not initialized";
        return BT::NodeStatus::FAILURE;
    }
    
    try {
        // If no blackboard provided, create a default one
        if (!blackboard) {
            blackboard = BT::Blackboard::create();
        }
        
        // Create behavior tree
        auto tree = factory_.createTree(treeName, blackboard);
        
        std::cout << "[BehaviorTreeExecutor] Executing behavior tree: " << treeName << std::endl;
        
        // Execute behavior tree
        BT::NodeStatus status = tree.tickRoot();
        
        std::cout << "[BehaviorTreeExecutor] Behavior tree finished with status: ";
        switch (status) {
            case BT::NodeStatus::SUCCESS:
                std::cout << "SUCCESS";
                break;
            case BT::NodeStatus::FAILURE:
                std::cout << "FAILURE";
                break;
            case BT::NodeStatus::RUNNING:
                std::cout << "RUNNING";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << std::endl;
        
        return status;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute behavior tree: ") + e.what();
        std::cerr << "[BehaviorTreeExecutor] Error: " << lastError_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }
}

} // namespace behaviortree
