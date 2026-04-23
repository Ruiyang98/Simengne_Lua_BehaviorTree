#ifndef BEHAVIOR_TREE_EXECUTOR_H
#define BEHAVIOR_TREE_EXECUTOR_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>
#include "simulation/SimControlInterface.h"

namespace behaviortree {

// Tree execution info for external access
struct TreeExecutionInfo {
    std::string treeId;
    std::string treeName;
    BT::Tree tree;
    BT::NodeStatus lastStatus;
    bool isRunning;
    bool isAsync;

    TreeExecutionInfo() : lastStatus(BT::NodeStatus::IDLE), isRunning(false), isAsync(false) {}
};

// Behavior tree executor: manages loading and execution of behavior trees
class BehaviorTreeExecutor {
public:
    BehaviorTreeExecutor();
    ~BehaviorTreeExecutor();
    
    // Initialize: register custom nodes
    bool initialize();
    
    // Load behavior tree from XML file
    bool loadFromFile(const std::string& xmlFile);
    
    // Load behavior tree from XML string
    bool loadFromText(const std::string& xmlText);
    
    // Execute behavior tree
    BT::NodeStatus execute(const std::string& treeName = "MainTree",
                           BT::Blackboard::Ptr blackboard = nullptr);

    // Get tree blackboard
    BT::Blackboard::Ptr getBlackboard(const std::string& treeId);
    
    // Get tree status
    BT::NodeStatus getTreeStatus(const std::string& treeId);
    
    // Halt/stop tree
    bool haltTree(const std::string& treeId);
    
    // Check if tree exists
    bool hasTree(const std::string& treeId) const;
    
    // Get last error message
    std::string getLastError() const { return lastError_; }
    
    // Get factory (for advanced configuration)
    BT::BehaviorTreeFactory& getFactory() { return factory_; }

    // ==================== Async Execution with Global Scheduler ====================

    // Execute behavior tree asynchronously (using global scheduler)
    // entityId: entity ID for registration and management in scheduler
    // treeName: behavior tree name
    // blackboard: blackboard data
    // tickIntervalMs: this parameter is reserved but ignored (scheduler uses fixed 500ms)
    // Returns true on success, false on failure
    bool executeAsync(const std::string& entityId,
                      const std::string& treeName = "MainTree",
                      BT::Blackboard::Ptr blackboard = nullptr,
                      int tickIntervalMs = 0);

    // Stop an async behavior tree for entity
    bool stopAsync(const std::string& entityId);

    // Halt an async behavior tree for entity (pause ticking)
    bool haltAsync(const std::string& entityId);

    // Resume a halted async behavior tree for entity
    bool resumeAsync(const std::string& entityId);

    // Get async tree status for entity
    BT::NodeStatus getAsyncStatus(const std::string& entityId) const;

    // Check if async entity exists in scheduler
    bool hasAsyncEntity(const std::string& entityId) const;

    // Get all registered async entity IDs
    std::vector<std::string> getAsyncEntityIds() const;

private:
    BT::BehaviorTreeFactory factory_;
    std::string lastError_;
    bool initialized_;

    // Active trees map for external access
    mutable std::mutex treesMutex_;
    std::unordered_map<std::string, std::shared_ptr<TreeExecutionInfo>> activeTrees_;
    int treeIdCounter_;

    // Register custom nodes
    void registerNodes();

    // Generate unique tree ID
    std::string generateTreeId();
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_EXECUTOR_H
