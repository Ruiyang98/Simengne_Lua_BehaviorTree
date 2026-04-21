#include "scripting/LuaBehaviorTreeBridge.h"
#include "behaviortree/BlackboardKeys.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace scripting {

// Static member initialization
std::unordered_map<std::string, sol::protected_function> LuaActionNode::luaFunctions_;
std::mutex LuaActionNode::mutex_;

std::unordered_map<std::string, sol::protected_function> LuaConditionNode::luaFunctions_;
std::mutex LuaConditionNode::mutex_;

// LuaActionNode implementation
LuaActionNode::LuaActionNode(const std::string& name, const BT::NodeConfiguration& config)
    : SyncActionNode(name, config) {}

BT::PortsList LuaActionNode::providedPorts() {
    return { BT::InputPort<std::string>("lua_node_name") };
}

BT::NodeStatus LuaActionNode::tick() {
    auto nodeName = getInput<std::string>("lua_node_name");
    if (!nodeName) {
        std::cerr << "[LuaActionNode] Missing lua_node_name port" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName.value());
    if (it == luaFunctions_.end()) {
        std::cerr << "[LuaActionNode] Lua function not found: " << nodeName.value() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    sol::protected_function func = it->second;
    auto result = func();
    
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[LuaActionNode] Lua function error: " << err.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Lua function should return "SUCCESS", "FAILURE", or "RUNNING"
    std::string status = result.get<std::string>();
    if (status == "SUCCESS") return BT::NodeStatus::SUCCESS;
    if (status == "FAILURE") return BT::NodeStatus::FAILURE;
    if (status == "RUNNING") return BT::NodeStatus::RUNNING;
    
    return BT::NodeStatus::FAILURE;
}

void LuaActionNode::setLuaFunction(const std::string& nodeName, sol::protected_function func) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_[nodeName] = func;
}

void LuaActionNode::clearLuaFunction(const std::string& nodeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_.erase(nodeName);
}

// LuaConditionNode implementation
LuaConditionNode::LuaConditionNode(const std::string& name, const BT::NodeConfiguration& config)
    : ConditionNode(name, config) {}

BT::PortsList LuaConditionNode::providedPorts() {
    return { BT::InputPort<std::string>("lua_node_name") };
}

BT::NodeStatus LuaConditionNode::tick() {
    auto nodeName = getInput<std::string>("lua_node_name");
    if (!nodeName) {
        std::cerr << "[LuaConditionNode] Missing lua_node_name port" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName.value());
    if (it == luaFunctions_.end()) {
        std::cerr << "[LuaConditionNode] Lua function not found: " << nodeName.value() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    sol::protected_function func = it->second;
    auto result = func();
    
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[LuaConditionNode] Lua function error: " << err.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Lua function should return boolean
    bool condition = result.get<bool>();
    return condition ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

void LuaConditionNode::setLuaFunction(const std::string& nodeName, sol::protected_function func) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_[nodeName] = func;
}

void LuaConditionNode::clearLuaFunction(const std::string& nodeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_.erase(nodeName);
}

// LuaBehaviorTreeBridge implementation
LuaBehaviorTreeBridge::LuaBehaviorTreeBridge(sol::state* luaState, BT::BehaviorTreeFactory* factory)
    : luaState_(luaState)
    , factory_(factory)
    , treeIdCounter_(0)
    , initialized_(false) {}

LuaBehaviorTreeBridge::~LuaBehaviorTreeBridge() {
    // Stop all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
        }
    }
    activeTrees_.clear();
}

bool LuaBehaviorTreeBridge::initialize() {
    if (!luaState_ || !factory_) {
        lastError_ = "Lua state or BT factory is null";
        return false;
    }
    
    try {
        registerLuaNodeTypes();
        registerLuaAPI();
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to initialize bridge: ") + e.what();
        return false;
    }
}

void LuaBehaviorTreeBridge::registerLuaNodeTypes() {
    // Register Lua action node type
    factory_->registerNodeType<LuaActionNode>("LuaAction");
    
    // Register Lua condition node type
    factory_->registerNodeType<LuaConditionNode>("LuaCondition");
}

void LuaBehaviorTreeBridge::registerLuaAPI() {
    // Create bt table
    sol::table btTable = luaState_->create_named_table("bt");
    
    // Load behavior tree from file
    btTable.set_function("load_file", [this](const std::string& xmlFile) -> bool {
        return loadBehaviorTreeFromFile(xmlFile);
    });
    
    // Load behavior tree from text
    btTable.set_function("load_text", [this](const std::string& xmlText) -> bool {
        return loadBehaviorTreeFromText(xmlText);
    });
    
    // Execute behavior tree
    btTable.set_function("execute", [this](const std::string& treeName,
                                            sol::optional<std::string> entityId,
                                            sol::optional<sol::table> params) -> std::string {
        std::string id = entityId.value_or("");
        return executeBehaviorTree(treeName, id, params);
    });
    
    // Get tree status
    btTable.set_function("get_status", [this](const std::string& treeId) -> std::string {
        return getTreeStatus(treeId);
    });
    
    // Stop tree
    btTable.set_function("stop", [this](const std::string& treeId) -> bool {
        return stopBehaviorTree(treeId);
    });
    
    // Set blackboard value
    btTable.set_function("set_blackboard", [this](const std::string& treeId,
                                                   const std::string& key,
                                                   sol::object value) -> bool {
        return setBlackboardValue(treeId, key, value);
    });
    
    // Get blackboard value
    btTable.set_function("get_blackboard", [this](const std::string& treeId,
                                                   const std::string& key) -> sol::object {
        return getBlackboardValue(treeId, key);
    });
    
    // Register Lua action
    btTable.set_function("register_action", [this](const std::string& name,
                                                    sol::protected_function func) -> bool {
        return registerLuaAction(name, func);
    });
    
    // Register Lua condition
    btTable.set_function("register_condition", [this](const std::string& name,
                                                       sol::protected_function func) -> bool {
        return registerLuaCondition(name, func);
    });
    
    // Check if tree exists
    btTable.set_function("has_tree", [this](const std::string& treeId) -> bool {
        return hasTree(treeId);
    });
    
    // Get last error
    btTable.set_function("get_last_error", [this]() -> std::string {
        return getLastError();
    });

    // ==================== Async Execution API ====================

    // Execute behavior tree asynchronously
    btTable.set_function("execute_async", [this](const std::string& treeName,
                                                  sol::optional<std::string> entityId,
                                                  sol::optional<sol::table> params,
                                                  sol::optional<int> tickIntervalMs) -> std::string {
        std::string id = entityId.value_or("");
        int interval = tickIntervalMs.value_or(100);
        return executeAsync(treeName, id, params, interval);
    });

    // Stop async behavior tree
    btTable.set_function("stop_async", [this](const std::string& treeId) -> bool {
        return stopAsync(treeId);
    });

    // Get async behavior tree status
    btTable.set_function("get_async_status", [this](const std::string& treeId) -> std::string {
        return getAsyncStatus(treeId);
    });

    // List all async behavior trees
    btTable.set_function("list_async_trees", [this]() -> sol::table {
        return listAsyncTrees();
    });

    // Set complete callback
    btTable.set_function("set_complete_callback", [this](const std::string& treeId,
                                                          sol::protected_function callback) -> bool {
        return setCompleteCallback(treeId, callback);
    });

    // Set tick callback
    btTable.set_function("set_tick_callback", [this](const std::string& treeId,
                                                      sol::protected_function callback) -> bool {
        return setTickCallback(treeId, callback);
    });

    // Start scheduler
    btTable.set_function("start_scheduler", [this](sol::optional<int> tickIntervalMs) -> bool {
        return startScheduler(tickIntervalMs);
    });

    // Stop scheduler
    btTable.set_function("stop_scheduler", [this]() {
        stopScheduler();
    });

    // Set scheduler manual mode
    btTable.set_function("set_manual_mode", [this](bool manual) {
        setSchedulerManualMode(manual);
    });

    // Manual update (for manual mode)
    btTable.set_function("update_scheduler", [this]() {
        updateScheduler();
    });
}

bool LuaBehaviorTreeBridge::loadBehaviorTreeFromFile(const std::string& xmlFile) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
    }
    
    try {
        factory_->registerBehaviorTreeFromFile(xmlFile);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML file: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::loadBehaviorTreeFromText(const std::string& xmlText) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
    }
    
    try {
        factory_->registerBehaviorTreeFromText(xmlText);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML text: ") + e.what();
        return false;
    }
}

std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }
    
    try {
        // Create blackboard
        auto blackboard = BT::Blackboard::create();
        
        // Set entity ID
        blackboard->set(behaviortree::BlackboardKeys::ENTITY_ID, entityId);
        
        // Set additional parameters if provided
        if (params) {
            sol::table paramsTable = params.value();
            for (auto& pair : paramsTable) {
                std::string key = pair.first.as<std::string>();
                sol::object value = pair.second;
                
                if (value.is<std::string>()) {
                    blackboard->set(key, value.as<std::string>());
                } else if (value.is<bool>()) {
                    blackboard->set(key, value.as<bool>());
                } else if (value.is<int>()) {
                    blackboard->set(key, value.as<int>());
                } else if (value.is<double>()) {
                    double d = value.as<double>();
                    if (d == static_cast<int>(d)) {
                        blackboard->set(key, static_cast<int>(d));
                    } else {
                        blackboard->set(key, d);
                    }
                }
            }
        }
        
        // Create tree
        auto tree = factory_->createTree(treeName, blackboard);
        
        // Generate tree ID
        std::string treeId = generateTreeId();
        
        // Store tree info
        auto info = std::make_shared<TreeExecutionInfo>();
        info->treeId = treeId;
        info->treeName = treeName;
        info->entityId = entityId;
        info->tree = std::move(tree);
        info->isRunning = true;
        info->startTime = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(treesMutex_);
            activeTrees_[treeId] = info;
        }
        
        // Execute tree (synchronously for now)
        info->lastStatus = info->tree.tickRoot();
        info->isRunning = (info->lastStatus == BT::NodeStatus::RUNNING);
        
        return treeId;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute behavior tree: ") + e.what();
        return "";
    }
}

std::string LuaBehaviorTreeBridge::getTreeStatus(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        return "NOT_FOUND";
    }
    
    switch (it->second->lastStatus) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

bool LuaBehaviorTreeBridge::stopBehaviorTree(const std::string& treeId) {
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

bool LuaBehaviorTreeBridge::setBlackboardValue(const std::string& treeId,
                                                const std::string& key,
                                                sol::object value) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }
    
    try {
        auto blackboard = it->second->tree.rootBlackboard();
        if (!blackboard) {
            lastError_ = "Blackboard is null";
            return false;
        }
        
        if (value.is<std::string>()) {
            blackboard->set(key, value.as<std::string>());
        } else if (value.is<double>()) {
            blackboard->set(key, value.as<double>());
        } else if (value.is<bool>()) {
            blackboard->set(key, value.as<bool>());
        } else if (value.is<int>()) {
            blackboard->set(key, value.as<int>());
        } else {
            lastError_ = "Unsupported value type";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to set blackboard value: ") + e.what();
        return false;
    }
}

sol::object LuaBehaviorTreeBridge::getBlackboardValue(const std::string& treeId,
                                                       const std::string& key) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return sol::nil;
    }
    
    try {
        auto blackboard = it->second->tree.rootBlackboard();
        if (!blackboard) {
            lastError_ = "Blackboard is null";
            return sol::nil;
        }
        
        if (!blackboard->getAny(key)) {
            return sol::nil;
        }
        
        // Try to get as different types
        try {
            return sol::make_object(*luaState_, blackboard->get<std::string>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<double>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<bool>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<int>(key));
        } catch (...) {}
        
        return sol::nil;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to get blackboard value: ") + e.what();
        return sol::nil;
    }
}

bool LuaBehaviorTreeBridge::registerLuaAction(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid Lua function";
        return false;
    }
    
    LuaActionNode::setLuaFunction(name, func);
    return true;
}

bool LuaBehaviorTreeBridge::registerLuaCondition(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid Lua function";
        return false;
    }
    
    LuaConditionNode::setLuaFunction(name, func);
    return true;
}

bool LuaBehaviorTreeBridge::hasTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    return activeTrees_.find(treeId) != activeTrees_.end();
}

std::string LuaBehaviorTreeBridge::generateTreeId() {
    std::stringstream ss;
    ss << "bt_" << ++treeIdCounter_;
    return ss.str();
}

// ==================== Async Execution API ====================

std::string LuaBehaviorTreeBridge::executeAsync(const std::string& treeName,
                                                 const std::string& entityId,
                                                 sol::optional<sol::table> params,
                                                 sol::optional<int> tickIntervalMs) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }

    try {
        // Create blackboard
        auto blackboard = BT::Blackboard::create();

        // Set entity ID
        blackboard->set(behaviortree::BlackboardKeys::ENTITY_ID, entityId);

        // Set additional parameters if provided
        if (params) {
            sol::table paramsTable = params.value();
            for (auto& pair : paramsTable) {
                std::string key = pair.first.as<std::string>();
                sol::object value = pair.second;

                if (value.is<std::string>()) {
                    blackboard->set(key, value.as<std::string>());
                } else if (value.is<bool>()) {
                    blackboard->set(key, value.as<bool>());
                } else if (value.is<int>()) {
                    blackboard->set(key, value.as<int>());
                } else if (value.is<double>()) {
                    double d = value.as<double>();
                    if (d == static_cast<int>(d)) {
                        blackboard->set(key, static_cast<int>(d));
                    } else {
                        blackboard->set(key, d);
                    }
                }
            }
        }

        // Create tree
        auto tree = factory_->createTree(treeName, blackboard);

        // Generate tree ID
        std::string treeId = generateTreeId();

        // Store tree info
        auto info = std::make_shared<TreeExecutionInfo>();
        info->treeId = treeId;
        info->treeName = treeName;
        info->entityId = entityId;
        info->tree = std::move(tree);
        info->isRunning = true;
        info->startTime = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(treesMutex_);
            activeTrees_[treeId] = info;
        }

        // Execute first tick synchronously
        info->lastStatus = info->tree.tickRoot();
        info->isRunning = (info->lastStatus == BT::NodeStatus::RUNNING);

        // If tree is still running, we would need a scheduler to continue ticking
        // For now, return the tree ID for tracking
        // In a full implementation, this would integrate with BehaviorTreeScheduler

        return treeId;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute async behavior tree: ") + e.what();
        return "";
    }
}

bool LuaBehaviorTreeBridge::stopAsync(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }

    it->second->tree.haltTree();
    it->second->isRunning = false;
    it->second->lastStatus = BT::NodeStatus::IDLE;

    // Remove callbacks
    {
        std::lock_guard<std::mutex> cbLock(callbacksMutex_);
        asyncCallbacks_.erase(treeId);
    }

    return true;
}

std::string LuaBehaviorTreeBridge::getAsyncStatus(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        return "NOT_FOUND";
    }

    switch (it->second->lastStatus) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

sol::table LuaBehaviorTreeBridge::listAsyncTrees() {
    sol::table result = luaState_->create_table();

    std::lock_guard<std::mutex> lock(treesMutex_);
    int index = 1;
    for (const auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            sol::table treeInfo = luaState_->create_table();
            treeInfo["id"] = pair.first;
            treeInfo["name"] = pair.second->treeName;
            treeInfo["entity_id"] = pair.second->entityId;

            std::string statusStr;
            switch (pair.second->lastStatus) {
                case BT::NodeStatus::SUCCESS: statusStr = "SUCCESS"; break;
                case BT::NodeStatus::FAILURE: statusStr = "FAILURE"; break;
                case BT::NodeStatus::RUNNING: statusStr = "RUNNING"; break;
                case BT::NodeStatus::IDLE: statusStr = "IDLE"; break;
                default: statusStr = "UNKNOWN"; break;
            }
            treeInfo["status"] = statusStr;

            result[index++] = treeInfo;
        }
    }

    return result;
}

bool LuaBehaviorTreeBridge::setCompleteCallback(const std::string& treeId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[treeId].onComplete = callback;
    return true;
}

bool LuaBehaviorTreeBridge::setTickCallback(const std::string& treeId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[treeId].onTick = callback;
    return true;
}

bool LuaBehaviorTreeBridge::startScheduler(sol::optional<int> tickIntervalMs) {
    // Placeholder - would integrate with BehaviorTreeScheduler
    // For now, just return true as the basic async execution works
    return true;
}

void LuaBehaviorTreeBridge::stopScheduler() {
    // Placeholder - would integrate with BehaviorTreeScheduler
    // Halt all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
            pair.second->isRunning = false;
        }
    }
}

void LuaBehaviorTreeBridge::setSchedulerManualMode(bool manual) {
    // Placeholder - would integrate with BehaviorTreeScheduler
}

void LuaBehaviorTreeBridge::updateScheduler() {
    // Placeholder - would integrate with BehaviorTreeScheduler
    // Tick all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->lastStatus = pair.second->tree.tickRoot();
            pair.second->isRunning = (pair.second->lastStatus == BT::NodeStatus::RUNNING);

            // Call tick callback if set
            std::lock_guard<std::mutex> cbLock(callbacksMutex_);
            auto cbIt = asyncCallbacks_.find(pair.first);
            if (cbIt != asyncCallbacks_.end() && cbIt->second.onTick.valid()) {
                auto result = cbIt->second.onTick(pair.first);
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "[LuaBehaviorTreeBridge] Tick callback error: " << err.what() << std::endl;
                }
            }

            // Call complete callback if finished
            if (!pair.second->isRunning && cbIt != asyncCallbacks_.end() && cbIt->second.onComplete.valid()) {
                std::string statusStr;
                switch (pair.second->lastStatus) {
                    case BT::NodeStatus::SUCCESS: statusStr = "SUCCESS"; break;
                    case BT::NodeStatus::FAILURE: statusStr = "FAILURE"; break;
                    default: statusStr = "UNKNOWN"; break;
                }
                auto result = cbIt->second.onComplete(pair.first, statusStr);
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "[LuaBehaviorTreeBridge] Complete callback error: " << err.what() << std::endl;
                }
            }
        }
    }
}

} // namespace scripting
