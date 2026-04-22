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
#include "behaviortree/BehaviorTreeScheduler.h"

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
    BehaviorTreeExecutor(simulation::SimControlInterface* simController);
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
    
    // Execute with tree ID return
    std::string executeWithId(const std::string& treeName = "MainTree",
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
    
    // Get tree info (for Lua bridge integration)
    std::shared_ptr<TreeExecutionInfo> getTreeInfo(const std::string& treeId);

    // ==================== Async Execution with Scheduler ====================

    // Execute behavior tree asynchronously (using scheduler)
    // Returns tree ID on success, empty string on failure
    std::string executeAsync(const std::string& treeName = "MainTree",
                              BT::Blackboard::Ptr blackboard = nullptr,
                              int tickIntervalMs = 100);

    // Execute behavior tree asynchronously with custom tick interval (per-instance frequency)
    // tickIntervalMs: 该实例的tick间隔（毫秒），0表示使用全局默认间隔
    std::string executeAsyncWithInterval(const std::string& treeName = "MainTree",
                                          BT::Blackboard::Ptr blackboard = nullptr,
                                          int tickIntervalMs = 0);

    // Set tick interval for a specific async tree instance
    bool setAsyncTreeTickInterval(const std::string& treeId, int tickIntervalMs);

    // Get tick interval for a specific async tree instance
    int getAsyncTreeTickInterval(const std::string& treeId) const;

    // Stop an async behavior tree
    bool stopAsync(const std::string& treeId);

    // Halt an async behavior tree (pause ticking)
    bool haltAsync(const std::string& treeId);

    // Resume a halted async behavior tree
    bool resumeAsync(const std::string& treeId);

    // Get async tree status
    BT::NodeStatus getAsyncStatus(const std::string& treeId) const;

    // Check if async tree exists
    bool hasAsyncTree(const std::string& treeId) const;

    // List all async tree IDs
    std::vector<std::string> listAsyncTrees() const;

    // Get scheduler reference
    BehaviorTreeScheduler& getScheduler() { return scheduler_; }

    // Manual update - call this from game loop if using manual mode
    void updateScheduler();

    // Set scheduler manual mode
    void setSchedulerManualMode(bool manual);

    // Start scheduler (if not already started)
    bool startScheduler(int tickIntervalMs = 100);

    // Stop scheduler
    void stopScheduler();

private:
    simulation::SimControlInterface* simController_;
    BT::BehaviorTreeFactory factory_;
    std::string lastError_;
    bool initialized_;

    // Active trees map for external access
    mutable std::mutex treesMutex_;
    std::unordered_map<std::string, std::shared_ptr<TreeExecutionInfo>> activeTrees_;
    int treeIdCounter_;

    // Scheduler for async execution
    BehaviorTreeScheduler scheduler_;

    // Register custom nodes
    void registerNodes();

    // Generate unique tree ID
    std::string generateTreeId();
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_EXECUTOR_H
