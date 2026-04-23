#include "scripting/LuaStatefulActionNode.h"
#include <iostream>

namespace scripting {

// Static member initialization
std::unordered_map<std::string, std::tuple<sol::protected_function, 
                                            sol::protected_function, 
                                            sol::protected_function>> LuaStatefulActionNode::luaFunctions_;
std::mutex LuaStatefulActionNode::mutex_;
sol::state* LuaStatefulActionNode::luaState_ = nullptr;

LuaStatefulActionNode::LuaStatefulActionNode(const std::string& name, const BT::NodeConfiguration& config)
    : BT::StatefulActionNode(name, config)
    , initialized_(false)
{
}

sol::table LuaStatefulActionNode::collectInputPorts() {
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

BT::NodeStatus LuaStatefulActionNode::parseStatus(const sol::protected_function_result& result) {
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[LuaStatefulActionNode] Lua function error: " << err.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Lua function should return "SUCCESS", "FAILURE", or "RUNNING"
    std::string status = result.get<std::string>();
    if (status == "SUCCESS") return BT::NodeStatus::SUCCESS;
    if (status == "FAILURE") return BT::NodeStatus::FAILURE;
    if (status == "RUNNING") return BT::NodeStatus::RUNNING;

    std::cerr << "[LuaStatefulActionNode] Invalid status returned: " << status << std::endl;
    return BT::NodeStatus::FAILURE;
}

BT::NodeStatus LuaStatefulActionNode::onStart() {
    auto nodeName = getInput<std::string>("lua_node_name");
    if (!nodeName) {
        std::cerr << "[LuaStatefulActionNode] Missing lua_node_name port" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    nodeName_ = nodeName.value();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName_);
    if (it == luaFunctions_.end()) {
        std::cerr << "[LuaStatefulActionNode] Lua functions not found: " << nodeName_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get onStart function
    sol::protected_function onStartFunc = std::get<0>(it->second);
    if (!onStartFunc.valid()) {
        std::cerr << "[LuaStatefulActionNode] onStart function not set for: " << nodeName_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Collect input parameters
    sol::table params = collectInputPorts();

    // Call onStart
    auto result = onStartFunc(params);
    BT::NodeStatus status = parseStatus(result);

    if (status == BT::NodeStatus::RUNNING) {
        initialized_ = true;
    }

    return status;
}

BT::NodeStatus LuaStatefulActionNode::onRunning() {
    if (!initialized_) {
        std::cerr << "[LuaStatefulActionNode] Node not initialized: " << nodeName_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName_);
    if (it == luaFunctions_.end()) {
        std::cerr << "[LuaStatefulActionNode] Lua functions not found: " << nodeName_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get onRunning function
    sol::protected_function onRunningFunc = std::get<1>(it->second);
    if (!onRunningFunc.valid()) {
        std::cerr << "[LuaStatefulActionNode] onRunning function not set for: " << nodeName_ << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Collect input parameters
    sol::table params = collectInputPorts();

    // Call onRunning
    auto result = onRunningFunc(params);
    BT::NodeStatus status = parseStatus(result);

    // If completed, reset initialized flag
    if (status != BT::NodeStatus::RUNNING) {
        initialized_ = false;
    }

    return status;
}

void LuaStatefulActionNode::onHalted() {
    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName_);
    if (it == luaFunctions_.end()) {
        return;
    }

    // Get onHalted function
    sol::protected_function onHaltedFunc = std::get<2>(it->second);
    if (!onHaltedFunc.valid()) {
        return;
    }

    // Collect input parameters
    sol::table params = collectInputPorts();

    // Call onHalted (ignore return value)
    auto result = onHaltedFunc(params);
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[LuaStatefulActionNode] onHalted error: " << err.what() << std::endl;
    }

    initialized_ = false;
}

void LuaStatefulActionNode::setLuaFunctions(const std::string& nodeName, 
                                             sol::protected_function onStart,
                                             sol::protected_function onRunning,
                                             sol::protected_function onHalted) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_[nodeName] = std::make_tuple(onStart, onRunning, onHalted);
}

void LuaStatefulActionNode::clearLuaFunctions(const std::string& nodeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    luaFunctions_.erase(nodeName);
}

} // namespace scripting
