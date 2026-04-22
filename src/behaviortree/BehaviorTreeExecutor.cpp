#include "behaviortree/BehaviorTreeExecutor.h"
#include "behaviortree/SimControllerPtr.h"
#include "behaviortree/MoveToPoint.h"
#include "behaviortree/AsyncMoveToPoint.h"
#include "behaviortree/FollowPath.h"
#include "behaviortree/CheckEntityExists.h"
#include "behaviortree/BehaviorTreeScheduler.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace behaviortree {

// Define global SimControlInterface pointer
simulation::SimControlInterface* g_simController = nullptr;

BehaviorTreeExecutor::BehaviorTreeExecutor(simulation::SimControlInterface* simController)
    : simController_(simController)
    , initialized_(false)
    , treeIdCounter_(0)
{
    // Set global pointer
    setSimController(simController);
}

BehaviorTreeExecutor::~BehaviorTreeExecutor() {
    // Halt all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
        }
    }
    activeTrees_.clear();
    
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
    // Register MoveToPoint node (sync)
    factory_.registerNodeType<MoveToPoint>("MoveToPoint");

    // Register AsyncMoveToPoint node (async)
    factory_.registerNodeType<AsyncMoveToPoint>("AsyncMoveToPoint");

    // Register FollowPath node
    factory_.registerNodeType<FollowPath>("FollowPath");

    // Register CheckEntityExists node
    factory_.registerNodeType<CheckEntityExists>("CheckEntityExists");

    std::cout << "[BehaviorTreeExecutor] Registered custom nodes:" << std::endl;
    std::cout << "  - MoveToPoint" << std::endl;
    std::cout << "  - AsyncMoveToPoint" << std::endl;
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

std::string BehaviorTreeExecutor::executeWithId(const std::string& treeName,
                                                 BT::Blackboard::Ptr blackboard) {
    if (!initialized_) {
        lastError_ = "Executor not initialized";
        return "";
    }
    
    try {
        // If no blackboard provided, create a default one
        if (!blackboard) {
            blackboard = BT::Blackboard::create();
        }
        
        // Generate tree ID
        std::string treeId = generateTreeId();
        
        // Create behavior tree
        auto tree = factory_.createTree(treeName, blackboard);
        
        // Store tree info
        auto info = std::make_shared<TreeExecutionInfo>();
        info->treeId = treeId;
        info->treeName = treeName;
        info->tree = std::move(tree);
        info->isRunning = true;
        
        std::cout << "[BehaviorTreeExecutor] Executing behavior tree: " << treeName 
                  << " (ID: " << treeId << ")" << std::endl;
        
        // Execute behavior tree
        info->lastStatus = info->tree.tickRoot();
        info->isRunning = (info->lastStatus == BT::NodeStatus::RUNNING);
        
        std::cout << "[BehaviorTreeExecutor] Behavior tree finished with status: ";
        switch (info->lastStatus) {
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
        
        // Store in active trees
        {
            std::lock_guard<std::mutex> lock(treesMutex_);
            activeTrees_[treeId] = info;
        }
        
        return treeId;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute behavior tree: ") + e.what();
        std::cerr << "[BehaviorTreeExecutor] Error: " << lastError_ << std::endl;
        return "";
    }
}

BT::Blackboard::Ptr BehaviorTreeExecutor::getBlackboard(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return nullptr;
    }
    return it->second->tree.rootBlackboard();
}

BT::NodeStatus BehaviorTreeExecutor::getTreeStatus(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return BT::NodeStatus::IDLE;
    }
    return it->second->lastStatus;
}

bool BehaviorTreeExecutor::haltTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }
    
    it->second->tree.haltTree();
    it->second->isRunning = false;
    it->second->lastStatus = BT::NodeStatus::IDLE;
    return true;
}

bool BehaviorTreeExecutor::hasTree(const std::string& treeId) const {
    std::lock_guard<std::mutex> lock(treesMutex_);
    return activeTrees_.find(treeId) != activeTrees_.end();
}

std::shared_ptr<TreeExecutionInfo> BehaviorTreeExecutor::getTreeInfo(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        return nullptr;
    }
    return it->second;
}

std::string BehaviorTreeExecutor::generateTreeId() {
    std::stringstream ss;
    ss << "bt_" << ++treeIdCounter_;
    return ss.str();
}

// ==================== Async Execution with Global Scheduler ====================

bool BehaviorTreeExecutor::executeAsync(const std::string& entityId,
                                        const std::string& treeName,
                                        BT::Blackboard::Ptr blackboard,
                                        int tickIntervalMs) {
    // Delegate to executeAsyncWithInterval, tickIntervalMs is ignored (scheduler uses fixed 500ms)
    return executeAsyncWithInterval(entityId, treeName, blackboard, tickIntervalMs);
}

bool BehaviorTreeExecutor::executeAsyncWithInterval(const std::string& entityId,
                                                    const std::string& treeName,
                                                    BT::Blackboard::Ptr blackboard,
                                                    int tickIntervalMs) {
    if (!initialized_) {
        lastError_ = "Executor not initialized";
        return false;
    }

    if (entityId.empty()) {
        lastError_ = "Entity ID cannot be empty";
        return false;
    }

    try {
        // If no blackboard provided, create a default one
        if (!blackboard) {
            blackboard = BT::Blackboard::create();
        }

        // Create behavior tree
        auto tree = factory_.createTree(treeName, blackboard);

        // Get global scheduler instance
        BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

        // Register entity with tree in the global scheduler
        // tickIntervalMs is ignored, scheduler uses fixed 500ms interval
        bool result = scheduler.registerEntityWithTree(entityId, treeName, std::move(tree), blackboard);

        if (result) {
            // Store in active trees for tracking
            auto info = std::make_shared<TreeExecutionInfo>();
            info->treeId = entityId;
            info->treeName = treeName;
            info->lastStatus = BT::NodeStatus::RUNNING;
            info->isRunning = true;
            info->isAsync = true;

            {
                std::lock_guard<std::mutex> lock(treesMutex_);
                activeTrees_[entityId] = info;
            }

            std::cout << "[BehaviorTreeExecutor] Started async behavior tree for entity: " << entityId
                      << " (tree: " << treeName << ", interval: 500ms [fixed])" << std::endl;
        }

        return result;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute async behavior tree: ") + e.what();
        std::cerr << "[BehaviorTreeExecutor] Error: " << lastError_ << std::endl;
        return false;
    }
}

bool BehaviorTreeExecutor::stopAsync(const std::string& entityId) {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Unregister entity from scheduler
    bool result = scheduler.unregisterEntity(entityId);

    if (result) {
        // Remove from active trees
        std::lock_guard<std::mutex> lock(treesMutex_);
        activeTrees_.erase(entityId);
    }

    return result;
}

bool BehaviorTreeExecutor::haltAsync(const std::string& entityId) {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Pause entity in scheduler
    return scheduler.pauseEntity(entityId);
}

bool BehaviorTreeExecutor::resumeAsync(const std::string& entityId) {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Resume entity in scheduler
    return scheduler.resumeEntity(entityId);
}

BT::NodeStatus BehaviorTreeExecutor::getAsyncStatus(const std::string& entityId) const {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Get entity status from scheduler
    return scheduler.getEntityStatus(entityId);
}

bool BehaviorTreeExecutor::hasAsyncEntity(const std::string& entityId) const {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Check if entity exists in scheduler
    return scheduler.hasEntity(entityId);
}

std::vector<std::string> BehaviorTreeExecutor::getAsyncEntityIds() const {
    // Get global scheduler instance
    BehaviorTreeScheduler& scheduler = BehaviorTreeScheduler::getInstance();

    // Get all registered entity IDs from scheduler
    return scheduler.getRegisteredEntityIds();
}

} // namespace behaviortree
