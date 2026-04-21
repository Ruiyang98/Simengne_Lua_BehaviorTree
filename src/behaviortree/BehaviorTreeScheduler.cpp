#include "behaviortree/BehaviorTreeScheduler.h"
#include <iostream>
#include <sstream>

namespace behaviortree {

BehaviorTreeScheduler::BehaviorTreeScheduler()
    : isRunning_(false)
    , manualMode_(false)
    , tickIntervalMs_(100)
    , treeIdCounter_(0)
    , shouldStop_(false) {}

BehaviorTreeScheduler::~BehaviorTreeScheduler() {
    stop();
}

bool BehaviorTreeScheduler::start(int tickIntervalMs) {
    if (isRunning_) {
        return true;
    }

    if (tickIntervalMs <= 0) {
        lastError_ = "Invalid tick interval: " + std::to_string(tickIntervalMs);
        return false;
    }

    tickIntervalMs_ = tickIntervalMs;
    shouldStop_ = false;

    if (!manualMode_) {
        try {
            schedulerThread_ = std::thread(&BehaviorTreeScheduler::schedulerLoop, this);
            isRunning_ = true;
            std::cout << "[BehaviorTreeScheduler] Started with " << tickIntervalMs_ << "ms interval"
                      << (manualMode_ ? " (manual mode)" : " (threaded mode)") << std::endl;
            return true;
        } catch (const std::exception& e) {
            lastError_ = std::string("Failed to start scheduler thread: ") + e.what();
            return false;
        }
    } else {
        isRunning_ = true;
        std::cout << "[BehaviorTreeScheduler] Started in manual mode" << std::endl;
        return true;
    }
}

void BehaviorTreeScheduler::stop() {
    if (!isRunning_) {
        return;
    }

    shouldStop_ = true;
    isRunning_ = false;

    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : scheduledTrees_) {
            if (pair.second->isRunning) {
                pair.second->tree.haltTree();
                pair.second->isRunning = false;
            }
        }
    }

    std::cout << "[BehaviorTreeScheduler] Stopped" << std::endl;
}

std::string BehaviorTreeScheduler::scheduleTree(const std::string& treeName,
                                                 BT::Tree&& tree,
                                                 const std::string& entityId,
                                                 CompleteCallback onComplete,
                                                 TickCallback onTick) {
    return scheduleTreeWithInterval(treeName, std::move(tree), 0, entityId, onComplete, onTick);
}

std::string BehaviorTreeScheduler::scheduleTreeWithInterval(const std::string& treeName,
                                                             BT::Tree&& tree,
                                                             int tickIntervalMs,
                                                             const std::string& entityId,
                                                             CompleteCallback onComplete,
                                                             TickCallback onTick) {
    std::string treeId = generateTreeId();

    auto info = std::make_shared<ScheduledTreeInfo>();
    info->treeId = treeId;
    info->treeName = treeName;
    info->entityId = entityId;
    info->tree = std::move(tree);
    info->lastStatus = BT::NodeStatus::RUNNING;
    info->isRunning = true;
    info->lastTickTime = std::chrono::steady_clock::now();
    info->nextTickTime = info->lastTickTime;
    info->onCompleteCallback = onComplete;
    info->onTickCallback = onTick;
    info->tickCount = 0;
    info->tickIntervalMs = tickIntervalMs;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        scheduledTrees_[treeId] = info;
    }

    int effectiveInterval = (tickIntervalMs > 0) ? tickIntervalMs : tickIntervalMs_.load();
    std::cout << "[BehaviorTreeScheduler] Scheduled tree '" << treeName
              << "' with ID: " << treeId 
              << " (tick interval: " << effectiveInterval << "ms)" << std::endl;

    if (!isRunning_) {
        start();
    }

    return treeId;
}

bool BehaviorTreeScheduler::unscheduleTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }

    if (it->second->isRunning) {
        it->second->tree.haltTree();
        it->second->isRunning = false;
    }

    scheduledTrees_.erase(it);
    std::cout << "[BehaviorTreeScheduler] Unscheduled tree: " << treeId << std::endl;
    return true;
}

bool BehaviorTreeScheduler::haltTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }

    if (it->second->isRunning) {
        it->second->tree.haltTree();
        it->second->isRunning = false;
        std::cout << "[BehaviorTreeScheduler] Halted tree: " << treeId << std::endl;
    }

    return true;
}

bool BehaviorTreeScheduler::resumeTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }

    if (!it->second->isRunning && it->second->lastStatus == BT::NodeStatus::RUNNING) {
        it->second->isRunning = true;
        std::cout << "[BehaviorTreeScheduler] Resumed tree: " << treeId << std::endl;
    }

    return true;
}

bool BehaviorTreeScheduler::setTreeTickInterval(const std::string& treeId, int tickIntervalMs) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }

    it->second->tickIntervalMs = tickIntervalMs;
    
    auto now = std::chrono::steady_clock::now();
    int effectiveInterval = (tickIntervalMs > 0) ? tickIntervalMs : tickIntervalMs_.load();
    it->second->nextTickTime = now + std::chrono::milliseconds(effectiveInterval);

    std::cout << "[BehaviorTreeScheduler] Set tree '" << treeId 
              << "' tick interval to " << effectiveInterval << "ms" << std::endl;
    return true;
}

int BehaviorTreeScheduler::getTreeTickInterval(const std::string& treeId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        return -1;
    }

    int interval = it->second->tickIntervalMs;
    if (interval <= 0) {
        return tickIntervalMs_.load();
    }
    return interval;
}

void BehaviorTreeScheduler::update() {
    if (!isRunning_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<ScheduledTreeInfo>> treesToTick;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : scheduledTrees_) {
            if (pair.second->isRunning) {
                if (now >= pair.second->nextTickTime) {
                    treesToTick.push_back(pair.second);
                }
            }
        }
    }

    for (auto& info : treesToTick) {
        tickTree(info);
    }
}

BT::NodeStatus BehaviorTreeScheduler::getTreeStatus(const std::string& treeId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        return BT::NodeStatus::IDLE;
    }

    return it->second->lastStatus;
}

bool BehaviorTreeScheduler::hasTree(const std::string& treeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scheduledTrees_.find(treeId) != scheduledTrees_.end();
}

std::shared_ptr<ScheduledTreeInfo> BehaviorTreeScheduler::getTreeInfo(const std::string& treeId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = scheduledTrees_.find(treeId);
    if (it == scheduledTrees_.end()) {
        return nullptr;
    }

    return it->second;
}

std::vector<std::string> BehaviorTreeScheduler::getScheduledTreeIds() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> ids;
    ids.reserve(scheduledTrees_.size());

    for (const auto& pair : scheduledTrees_) {
        ids.push_back(pair.first);
    }

    return ids;
}

size_t BehaviorTreeScheduler::getScheduledTreeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scheduledTrees_.size();
}

void BehaviorTreeScheduler::setTickInterval(int tickIntervalMs) {
    if (tickIntervalMs > 0) {
        tickIntervalMs_ = tickIntervalMs;
    }
}

void BehaviorTreeScheduler::setManualMode(bool manual) {
    if (isRunning_) {
        return;
    }
    manualMode_ = manual;
}

std::string BehaviorTreeScheduler::generateTreeId() {
    std::stringstream ss;
    ss << "scheduled_bt_" << ++treeIdCounter_;
    return ss.str();
}

void BehaviorTreeScheduler::schedulerLoop() {
    std::cout << "[BehaviorTreeScheduler] Scheduler thread started" << std::endl;

    while (!shouldStop_) {
        auto startTime = std::chrono::steady_clock::now();

        update();

        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto sleepDuration = std::chrono::milliseconds(tickIntervalMs_) - elapsed;

        if (sleepDuration > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepDuration);
        }
    }

    std::cout << "[BehaviorTreeScheduler] Scheduler thread stopped" << std::endl;
}

void BehaviorTreeScheduler::tickTree(const std::shared_ptr<ScheduledTreeInfo>& info) {
    if (!info->isRunning) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    info->lastTickTime = now;

    info->lastStatus = info->tree.tickRoot();
    info->tickCount++;

    int effectiveInterval = (info->tickIntervalMs > 0) ? info->tickIntervalMs : tickIntervalMs_.load();
    info->nextTickTime = now + std::chrono::milliseconds(effectiveInterval);

    if (info->onTickCallback) {
        try {
            info->onTickCallback(info->treeId);
        } catch (const std::exception& e) {
            std::cerr << "[BehaviorTreeScheduler] Tick callback error for tree "
                      << info->treeId << ": " << e.what() << std::endl;
        }
    }

    if (info->lastStatus != BT::NodeStatus::RUNNING) {
        info->isRunning = false;

        std::cout << "[BehaviorTreeScheduler] Tree '" << info->treeName
                  << "' (" << info->treeId << ") completed with status: ";
        switch (info->lastStatus) {
            case BT::NodeStatus::SUCCESS:
                std::cout << "SUCCESS";
                break;
            case BT::NodeStatus::FAILURE:
                std::cout << "FAILURE";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << " (ticks: " << info->tickCount << ")" << std::endl;

        if (info->onCompleteCallback) {
            try {
                info->onCompleteCallback(info->treeId, info->lastStatus);
            } catch (const std::exception& e) {
                std::cerr << "[BehaviorTreeScheduler] Completion callback error for tree "
                          << info->treeId << ": " << e.what() << std::endl;
            }
        }
    }
}

void BehaviorTreeScheduler::cleanupCompletedTrees() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = scheduledTrees_.begin(); it != scheduledTrees_.end();) {
        if (!it->second->isRunning && it->second->lastStatus != BT::NodeStatus::RUNNING) {
            it = scheduledTrees_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace behaviortree
