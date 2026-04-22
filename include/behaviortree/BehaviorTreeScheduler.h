#ifndef BEHAVIOR_TREE_SCHEDULER_H
#define BEHAVIOR_TREE_SCHEDULER_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

namespace behaviortree {

// Forward declaration
class BehaviorTreeExecutor;

// Entity scheduling info
struct ScheduledTreeInfo {
    std::string treeId;
    std::string treeName;
    std::string entityId;
    BT::Tree tree;
    BT::NodeStatus lastStatus;
    bool isRunning;
    bool paused;
    std::chrono::steady_clock::time_point lastTickTime;
    int tickCount;

    // Instance-level tick interval (milliseconds), 0 means use global default
    int tickIntervalMs;

    // The next time this instance should tick
    std::chrono::steady_clock::time_point nextTickTime;

    ScheduledTreeInfo()
        : lastStatus(BT::NodeStatus::IDLE)
        , isRunning(false)
        , paused(false)
        , tickCount(0)
        , tickIntervalMs(0) {}
};

// Behavior tree scheduler: singleton pattern, manages periodic ticking of behavior trees
class BehaviorTreeScheduler {
public:
    // Get the global unique instance
    static BehaviorTreeScheduler& getInstance();

    // Deleted copy constructor and assignment operator
    BehaviorTreeScheduler(const BehaviorTreeScheduler&) = delete;
    BehaviorTreeScheduler& operator=(const BehaviorTreeScheduler&) = delete;

    // Tick all scheduled trees once (called externally)
    void tickAll();

    // Register an entity with a behavior tree
    // entityId: the entity identifier (primary key)
    // treeName: the name of the tree
    // tree: the behavior tree instance
    // blackboard: optional blackboard (for future use)
    // Returns true on success, false on failure
    bool registerEntityWithTree(const std::string& entityId,
                                 const std::string& treeName,
                                 BT::Tree&& tree,
                                 std::shared_ptr<BT::Blackboard> blackboard = nullptr);

    // Register an entity without a behavior tree (reserve entity slot)
    // Returns true on success, false on failure
    bool registerEntity(const std::string& entityId);

    // Register an entity with a custom tick interval
    // tickIntervalMs: tick interval for this instance (milliseconds), 0 means use global default
    bool registerEntityWithTreeAndInterval(const std::string& entityId,
                                            const std::string& treeName,
                                            BT::Tree&& tree,
                                            int tickIntervalMs,
                                            std::shared_ptr<BT::Blackboard> blackboard = nullptr);

    // Unregister an entity (stop ticking and remove)
    // Returns true on success, false on failure
    bool unregisterEntity(const std::string& entityId);

    // Pause an entity's behavior tree scheduling
    // Returns true on success, false on failure
    bool pauseEntity(const std::string& entityId);

    // Resume an entity's behavior tree scheduling
    // Returns true on success, false on failure
    bool resumeEntity(const std::string& entityId);

    // Set tick interval for a specific entity
    bool setEntityTickInterval(const std::string& entityId, int tickIntervalMs);

    // Get tick interval for a specific entity (returns global default if using it)
    int getEntityTickInterval(const std::string& entityId) const;

    // Get entity status
    BT::NodeStatus getEntityStatus(const std::string& entityId) const;

    // Check if entity is registered
    bool hasEntity(const std::string& entityId) const;

    // Get entity info
    std::shared_ptr<ScheduledTreeInfo> getEntityInfo(const std::string& entityId) const;

    // Get all registered entity IDs
    std::vector<std::string> getRegisteredEntityIds() const;

    // Get number of registered entities
    size_t getRegisteredEntityCount() const;

    // Get last error message
    std::string getLastError() const { return lastError_; }

    // Fixed tick interval constant (milliseconds)
    static constexpr int TICK_INTERVAL_MS = 500;

private:
    // Private constructor for singleton
    BehaviorTreeScheduler();

    // Private destructor
    ~BehaviorTreeScheduler();

    mutable std::mutex mutex_;
    // Use entityId as primary key
    std::unordered_map<std::string, std::shared_ptr<ScheduledTreeInfo>> entities_;
    std::atomic<int> treeIdCounter_{0};
    std::string lastError_;

    // Generate unique tree ID (internal identifier)
    std::string generateTreeId();

    // Tick a single entity's tree
    void tickTree(const std::shared_ptr<ScheduledTreeInfo>& info);

    // Clean up completed trees (optional, can be called periodically)
    void cleanupCompletedTrees();
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_SCHEDULER_H
