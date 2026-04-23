#include "behaviortree/BehaviorTreeScheduler.h"
#include <iostream>
#include <sstream>

namespace behaviortree {

BehaviorTreeScheduler& BehaviorTreeScheduler::getInstance() {
    static BehaviorTreeScheduler instance;
    return instance;
}

BehaviorTreeScheduler::BehaviorTreeScheduler()
    : treeIdCounter_(0) {}

BehaviorTreeScheduler::~BehaviorTreeScheduler() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : entities_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
            pair.second->isRunning = false;
        }
    }
}

void BehaviorTreeScheduler::tickAll() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<ScheduledTreeInfo>> treesToTick;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : entities_) {
            if (pair.second->isRunning && !pair.second->paused) {
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

bool BehaviorTreeScheduler::registerEntityWithTree(const std::string& entityId,
                                                    const std::string& treeName,
                                                    BT::Tree&& tree,
                                                    std::shared_ptr<BT::Blackboard> blackboard) {
    return registerEntityWithTreeAndInterval(entityId, treeName, std::move(tree), 0, blackboard);
}

bool BehaviorTreeScheduler::registerEntityWithTreeAndInterval(const std::string& entityId,
                                                               const std::string& treeName,
                                                               BT::Tree&& tree,
                                                               int tickIntervalMs,
                                                               std::shared_ptr<BT::Blackboard> blackboard) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if entity already exists
    if (entities_.find(entityId) != entities_.end()) {
        lastError_ = "Entity already registered: " + entityId;
        return false;
    }

    std::string treeId = generateTreeId();

    auto info = std::make_shared<ScheduledTreeInfo>();
    info->treeId = treeId;
    info->treeName = treeName;
    info->entityId = entityId;
    info->tree = std::move(tree);
    info->lastStatus = BT::NodeStatus::RUNNING;
    info->isRunning = true;
    info->paused = false;
    info->lastTickTime = std::chrono::steady_clock::now();
    info->nextTickTime = info->lastTickTime;
    info->tickCount = 0;
    info->tickIntervalMs = tickIntervalMs;

    entities_[entityId] = info;

    int effectiveInterval = (tickIntervalMs > 0) ? tickIntervalMs : TICK_INTERVAL_MS;
    std::cout << "[BehaviorTreeScheduler] Registered entity '" << entityId
              << "' with tree '" << treeName
              << "' (treeId: " << treeId
              << ", tick interval: " << effectiveInterval << "ms)" << std::endl;

    return true;
}

bool BehaviorTreeScheduler::unregisterEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        lastError_ = "Entity not found: " + entityId;
        return false;
    }

    if (it->second->isRunning) {
        it->second->tree.haltTree();
        it->second->isRunning = false;
    }

    entities_.erase(it);
    std::cout << "[BehaviorTreeScheduler] Unregistered entity: " << entityId << std::endl;
    return true;
}

bool BehaviorTreeScheduler::pauseEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        lastError_ = "Entity not found: " + entityId;
        return false;
    }

    if (!it->second->paused) {
        it->second->paused = true;
        std::cout << "[BehaviorTreeScheduler] Paused entity: " << entityId << std::endl;
    }

    return true;
}

bool BehaviorTreeScheduler::resumeEntity(const std::string& entityId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        lastError_ = "Entity not found: " + entityId;
        return false;
    }

    if (it->second->paused) {
        it->second->paused = false;
        // Update next tick time to resume from now
        it->second->nextTickTime = std::chrono::steady_clock::now();
        std::cout << "[BehaviorTreeScheduler] Resumed entity: " << entityId << std::endl;
    }

    return true;
}

bool BehaviorTreeScheduler::setEntityTickInterval(const std::string& entityId, int tickIntervalMs) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        lastError_ = "Entity not found: " + entityId;
        return false;
    }

    it->second->tickIntervalMs = tickIntervalMs;

    auto now = std::chrono::steady_clock::now();
    int effectiveInterval = (tickIntervalMs > 0) ? tickIntervalMs : TICK_INTERVAL_MS;
    it->second->nextTickTime = now + std::chrono::milliseconds(effectiveInterval);

    std::cout << "[BehaviorTreeScheduler] Set entity '" << entityId
              << "' tick interval to " << effectiveInterval << "ms" << std::endl;
    return true;
}

int BehaviorTreeScheduler::getEntityTickInterval(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        return -1;
    }

    int interval = it->second->tickIntervalMs;
    if (interval <= 0) {
        return TICK_INTERVAL_MS;
    }
    return interval;
}

BT::NodeStatus BehaviorTreeScheduler::getEntityStatus(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        return BT::NodeStatus::IDLE;
    }

    return it->second->lastStatus;
}

bool BehaviorTreeScheduler::hasEntity(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entities_.find(entityId) != entities_.end();
}

std::shared_ptr<ScheduledTreeInfo> BehaviorTreeScheduler::getEntityInfo(const std::string& entityId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        return nullptr;
    }

    return it->second;
}

std::vector<std::string> BehaviorTreeScheduler::getRegisteredEntityIds() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> ids;
    ids.reserve(entities_.size());

    for (const auto& pair : entities_) {
        ids.push_back(pair.first);
    }

    return ids;
}

size_t BehaviorTreeScheduler::getRegisteredEntityCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entities_.size();
}

std::string BehaviorTreeScheduler::generateTreeId() {
    std::stringstream ss;
    ss << "scheduled_bt_" << ++treeIdCounter_;
    return ss.str();
}

void BehaviorTreeScheduler::tickTree(const std::shared_ptr<ScheduledTreeInfo>& info) {
    if (!info->isRunning || info->paused) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    info->lastTickTime = now;

    info->lastStatus = info->tree.tickRoot();
    info->tickCount++;

    int effectiveInterval = (info->tickIntervalMs > 0) ? info->tickIntervalMs : TICK_INTERVAL_MS;
    info->nextTickTime = now + std::chrono::milliseconds(effectiveInterval);

    if (info->lastStatus != BT::NodeStatus::RUNNING) {
        info->isRunning = false;

        std::cout << "[BehaviorTreeScheduler] Tree '" << info->treeName
                  << "' for entity '" << info->entityId
                  << "' (treeId: " << info->treeId << ") completed with status: ";
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
    }
}

} // namespace behaviortree
