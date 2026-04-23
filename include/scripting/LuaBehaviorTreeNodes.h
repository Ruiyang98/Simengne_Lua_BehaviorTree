#ifndef LUA_BEHAVIOR_TREE_NODES_H
#define LUA_BEHAVIOR_TREE_NODES_H

#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/condition_node.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <sol.hpp>
#include <string>
#include <unordered_map>
#include <mutex>
#include <tuple>

namespace scripting {

// Lua Action Node - wraps Lua function as BT action node
class LuaActionNode : public BT::SyncActionNode {
public:
    LuaActionNode(const std::string& name, const BT::NodeConfiguration& config);

    // Return empty ports list to allow any input ports (for parameter passing)
    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override;

    // Set the Lua function to be called
    static void setLuaFunction(const std::string& nodeName, sol::protected_function func);
    static void clearLuaFunction(const std::string& nodeName);

    // Set the Lua state for creating parameter tables
    static void setLuaState(sol::state* state) { luaState_ = state; }

private:
    static std::unordered_map<std::string, sol::protected_function> luaFunctions_;
    static std::mutex mutex_;
    static sol::state* luaState_;

    // Collect input ports into a Lua table
    sol::table collectInputPorts();
};

// Lua Condition Node - wraps Lua function as BT condition node
class LuaConditionNode : public BT::ConditionNode {
public:
    LuaConditionNode(const std::string& name, const BT::NodeConfiguration& config);

    // Return empty ports list to allow any input ports (for parameter passing)
    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override;

    // Set the Lua function to be called
    static void setLuaFunction(const std::string& nodeName, sol::protected_function func);
    static void clearLuaFunction(const std::string& nodeName);

    // Set the Lua state for creating parameter tables
    static void setLuaState(sol::state* state) { luaState_ = state; }

private:
    static std::unordered_map<std::string, sol::protected_function> luaFunctions_;
    static std::mutex mutex_;
    static sol::state* luaState_;

    // Collect input ports into a Lua table
    sol::table collectInputPorts();
};

// Lua Stateful Action Node - wraps Lua functions as BT StatefulActionNode
// Supports onStart, onRunning, onHalted lifecycle methods
class LuaStatefulActionNode : public BT::StatefulActionNode {
public:
    LuaStatefulActionNode(const std::string& name, const BT::NodeConfiguration& config);

    // Return empty ports list to allow any input ports (for parameter passing)
    static BT::PortsList providedPorts() { return {}; }

    // Called when the node is first entered
    BT::NodeStatus onStart() override;

    // Called every tick while node is RUNNING
    BT::NodeStatus onRunning() override;

    // Called when the node is halted
    void onHalted() override;

    // Set the Lua functions to be called
    static void setLuaFunctions(const std::string& nodeName, 
                                 sol::protected_function onStart,
                                 sol::protected_function onRunning,
                                 sol::protected_function onHalted);
    static void clearLuaFunctions(const std::string& nodeName);

    // Set the Lua state for creating parameter tables
    static void setLuaState(sol::state* state) { luaState_ = state; }

private:
    static std::unordered_map<std::string, std::tuple<sol::protected_function, 
                                                       sol::protected_function, 
                                                       sol::protected_function>> luaFunctions_;
    static std::mutex mutex_;
    static sol::state* luaState_;

    // Collect input ports into a Lua table
    sol::table collectInputPorts();

    // Parse Lua return value to NodeStatus
    BT::NodeStatus parseStatus(const sol::protected_function_result& result);

    // Node name for looking up Lua functions
    std::string nodeName_;
    bool initialized_;
};

} // namespace scripting

#endif // LUA_BEHAVIOR_TREE_NODES_H
