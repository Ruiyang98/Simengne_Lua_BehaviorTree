#include "scripting/LuaBehaviorTreeBridge.h"
#include "behaviortree/BlackboardKeys.h"
#include "behaviortree/BehaviorTreeScheduler.h"
#include <iostream>
#include <sstream>
#include <chrono>

namespace scripting {

// Static member initialization
std::unordered_map<std::string, sol::protected_function> LuaActionNode::luaFunctions_;
std::mutex LuaActionNode::mutex_;
sol::state* LuaActionNode::luaState_ = nullptr;

std::unordered_map<std::string, sol::protected_function> LuaConditionNode::luaFunctions_;
std::mutex LuaConditionNode::mutex_;
sol::state* LuaConditionNode::luaState_ = nullptr;

// LuaActionNode implementation
LuaActionNode::LuaActionNode(const std::string& name, const BT::NodeConfiguration& config)
    : SyncActionNode(name, config) {}

sol::table LuaActionNode::collectInputPorts() {
    sol::table params;
    if (luaState_) {
        params = luaState_->create_table();
    } else {
        return sol::nil;
    }

    // Iterate through all input ports and collect their values
    for (const auto& port : config().input_ports) {
        const std::string& portName = port.first;

        // Skip the lua_node_name port as it's used to identify the function
        if (portName == "lua_node_name") {
            continue;
        }

        // Try to get the value as different types
        // First try as string (most generic)
        if (auto strVal = getInput<std::string>(portName)) {
            params[portName] = strVal.value();
        }
        // Then try as double
        else if (auto doubleVal = getInput<double>(portName)) {
            params[portName] = doubleVal.value();
        }
        // Then try as int
        else if (auto intVal = getInput<int>(portName)) {
            params[portName] = intVal.value();
        }
        // Finally try as bool
        else if (auto boolVal = getInput<bool>(portName)) {
            params[portName] = boolVal.value();
        }
    }

    return params;
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

    // Collect input parameters
    sol::table params = collectInputPorts();

    sol::protected_function func = it->second;
    auto result = func(params);

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

sol::table LuaConditionNode::collectInputPorts() {
    sol::table params;
    if (luaState_) {
        params = luaState_->create_table();
    } else {
        return sol::nil;
    }

    // Iterate through all input ports and collect their values
    for (const auto& port : config().input_ports) {
        const std::string& portName = port.first;

        // Skip the lua_node_name port as it's used to identify the function
        if (portName == "lua_node_name") {
            continue;
        }

        // Try to get the value as different types
        // First try as string (most generic)
        if (auto strVal = getInput<std::string>(portName)) {
            params[portName] = strVal.value();
        }
        // Then try as double
        else if (auto doubleVal = getInput<double>(portName)) {
            params[portName] = doubleVal.value();
        }
        // Then try as int
        else if (auto intVal = getInput<int>(portName)) {
            params[portName] = intVal.value();
        }
        // Finally try as bool
        else if (auto boolVal = getInput<bool>(portName)) {
            params[portName] = boolVal.value();
        }
    }

    return params;
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

    // Collect input parameters
    sol::table params = collectInputPorts();

    sol::protected_function func = it->second;
    auto result = func(params);

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
    // Set Lua state for parameter collection
    LuaActionNode::setLuaState(luaState_);
    LuaConditionNode::setLuaState(luaState_);

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
    // entityId is now required (not optional)
    btTable.set_function("execute_async", [this](const std::string& treeName,
                                                  const std::string& entityId,
                                                  sol::optional<sol::table> params) -> bool {
        return executeAsync(treeName, entityId, params);
    });

    // Stop async behavior tree by entityId
    btTable.set_function("stop_async", [this](const std::string& entityId) -> bool {
        return stopAsync(entityId);
    });

    // Get async behavior tree status by entityId
    btTable.set_function("get_async_status", [this](const std::string& entityId) -> std::string {
        return getAsyncStatus(entityId);
    });

    // Get all registered async entity IDs
    btTable.set_function("get_async_entities", [this]() -> sol::table {
        return getAsyncEntities();
    });

    // Set complete callback
    btTable.set_function("set_complete_callback", [this](const std::string& entityId,
                                                          sol::protected_function callback) -> bool {
        return setCompleteCallback(entityId, callback);
    });

    // Set tick callback
    btTable.set_function("set_tick_callback", [this](const std::string& entityId,
                                                      sol::protected_function callback) -> bool {
        return setTickCallback(entityId, callback);
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

bool LuaBehaviorTreeBridge::executeAsync(const std::string& treeName,
                                          const std::string& entityId,
                                          sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
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

        // Get global scheduler instance and register entity
        auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
        bool success = scheduler.registerEntityWithTree(entityId, treeName, std::move(tree), blackboard);

        return success;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute async behavior tree: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::stopAsync(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    scheduler.unregisterEntity(entityId);
    return true;
}

std::string LuaBehaviorTreeBridge::getAsyncStatus(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    BT::NodeStatus status = scheduler.getEntityStatus(entityId);
    
    switch (status) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

sol::table LuaBehaviorTreeBridge::getAsyncEntities() {
    sol::table result = luaState_->create_table();

    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    std::vector<std::string> entityIds = scheduler.getRegisteredEntityIds();

    int index = 1;
    for (const auto& entityId : entityIds) {
        result[index++] = entityId;
    }

    return result;
}

bool LuaBehaviorTreeBridge::setCompleteCallback(const std::string& entityId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[entityId].onComplete = callback;
    return true;
}

bool LuaBehaviorTreeBridge::setTickCallback(const std::string& entityId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[entityId].onTick = callback;
    return true;
}

} // namespace scripting
