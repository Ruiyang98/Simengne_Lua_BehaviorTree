#ifndef BEHAVIOR_TREE_SCHEDULER_H
#define BEHAVIOR_TREE_SCHEDULER_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

namespace behaviortree {

// Forward declaration
class BehaviorTreeExecutor;

// Tree scheduling info
struct ScheduledTreeInfo {
    std::string treeId;
    std::string treeName;
    std::string entityId;
    BT::Tree tree;
    BT::NodeStatus lastStatus;
    bool isRunning;
    std::chrono::steady_clock::time_point lastTickTime;
    std::function<void(const std::string&, BT::NodeStatus)> onCompleteCallback;
    std::function<void(const std::string&)> onTickCallback;
    int tickCount;
    
    // 实例级tick间隔（毫秒），0表示使用全局默认间隔
    int tickIntervalMs;
    
    // 该实例下一次应该tick的时间点
    std::chrono::steady_clock::time_point nextTickTime;

    ScheduledTreeInfo()
        : lastStatus(BT::NodeStatus::IDLE)
        , isRunning(false)
        , tickCount(0)
        , tickIntervalMs(0) {}
};

// Behavior tree scheduler: manages periodic ticking of behavior trees
class BehaviorTreeScheduler {
public:
    using CompleteCallback = std::function<void(const std::string&, BT::NodeStatus)>;
    using TickCallback = std::function<void(const std::string&)>;

    BehaviorTreeScheduler();
    ~BehaviorTreeScheduler();

    // Start the scheduler thread with specified tick interval (in milliseconds)
    bool start(int tickIntervalMs = 100);

    // Stop the scheduler thread
    void stop();

    // Check if scheduler is running
    bool isRunning() const { return isRunning_; }

    // Schedule a tree for periodic ticking
    // Returns tree ID on success, empty string on failure
    std::string scheduleTree(const std::string& treeName,
                              BT::Tree&& tree,
                              const std::string& entityId = "",
                              CompleteCallback onComplete = nullptr,
                              TickCallback onTick = nullptr);

    // Schedule a tree with custom tick interval (per-instance frequency)
    // tickIntervalMs: 该实例的tick间隔（毫秒），0表示使用全局默认间隔
    std::string scheduleTreeWithInterval(const std::string& treeName,
                                          BT::Tree&& tree,
                                          int tickIntervalMs,
                                          const std::string& entityId = "",
                                          CompleteCallback onComplete = nullptr,
                                          TickCallback onTick = nullptr);

    // Set tick interval for a specific tree instance
    bool setTreeTickInterval(const std::string& treeId, int tickIntervalMs);

    // Get tick interval for a specific tree instance (returns 0 if using global default)
    int getTreeTickInterval(const std::string& treeId) const;

    // Unschedule a tree (stop ticking and remove)
    bool unscheduleTree(const std::string& treeId);

    // Halt a tree (stop ticking but keep in list)
    bool haltTree(const std::string& treeId);

    // Resume a halted tree
    bool resumeTree(const std::string& treeId);

    // Manual update - call this if not using the internal thread (e.g., called from game loop)
    void update();

    // Get tree status
    BT::NodeStatus getTreeStatus(const std::string& treeId) const;

    // Check if tree is scheduled
    bool hasTree(const std::string& treeId) const;

    // Get tree info
    std::shared_ptr<ScheduledTreeInfo> getTreeInfo(const std::string& treeId) const;

    // Get all scheduled tree IDs
    std::vector<std::string> getScheduledTreeIds() const;

    // Get number of scheduled trees
    size_t getScheduledTreeCount() const;

    // Set tick interval (can be changed while running)
    void setTickInterval(int tickIntervalMs);

    // Get tick interval
    int getTickInterval() const { return tickIntervalMs_; }

    // Set manual mode (disable internal thread, user must call update())
    void setManualMode(bool manual);

    // Check if in manual mode
    bool isManualMode() const { return manualMode_; }

    // Get last error message
    std::string getLastError() const { return lastError_; }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ScheduledTreeInfo>> scheduledTrees_;
    std::atomic<int> treeIdCounter_{0};
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> manualMode_{false};
    std::atomic<int> tickIntervalMs_{100};
    std::thread schedulerThread_;
    std::atomic<bool> shouldStop_{false};
    std::string lastError_;

    // Generate unique tree ID
    std::string generateTreeId();

    // Scheduler loop (runs in separate thread)
    void schedulerLoop();

    // Tick a single tree
    void tickTree(const std::shared_ptr<ScheduledTreeInfo>& info);

    // Clean up completed trees (optional, can be called periodically)
    void cleanupCompletedTrees();
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_SCHEDULER_H
