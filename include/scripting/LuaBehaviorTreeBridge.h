#ifndef LUA_BEHAVIOR_TREE_BRIDGE_H
#define LUA_BEHAVIOR_TREE_BRIDGE_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/condition_node.h>
#include <sol.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace scripting {

// Forward declarations
class LuaSimBinding;

// Tree execution info
struct TreeExecutionInfo {
    std::string treeId;
    std::string treeName;
    std::string entityId;
    BT::Tree tree;
    BT::NodeStatus lastStatus;
    bool isRunning;
    std::chrono::steady_clock::time_point startTime;
    
    TreeExecutionInfo() : lastStatus(BT::NodeStatus::IDLE), isRunning(false) {}
};

// Lua Action Node - wraps Lua function as BT action node
class LuaActionNode : public BT::SyncActionNode {
public:
    LuaActionNode(const std::string& name, const BT::NodeConfiguration& config);
    
    static BT::PortsList providedPorts();
    
    BT::NodeStatus tick() override;
    
    // Set the Lua function to be called
    static void setLuaFunction(const std::string& nodeName, sol::protected_function func);
    static void clearLuaFunction(const std::string& nodeName);
    
private:
    static std::unordered_map<std::string, sol::protected_function> luaFunctions_;
    static std::mutex mutex_;
};

// Lua Condition Node - wraps Lua function as BT condition node
class LuaConditionNode : public BT::ConditionNode {
public:
    LuaConditionNode(const std::string& name, const BT::NodeConfiguration& config);
    
    static BT::PortsList providedPorts();
    
    BT::NodeStatus tick() override;
    
    // Set the Lua function to be called
    static void setLuaFunction(const std::string& nodeName, sol::protected_function func);
    static void clearLuaFunction(const std::string& nodeName);
    
private:
    static std::unordered_map<std::string, sol::protected_function> luaFunctions_;
    static std::mutex mutex_;
};

// Bridge class between Lua and BehaviorTree.CPP
class LuaBehaviorTreeBridge {
public:
    explicit LuaBehaviorTreeBridge(sol::state* luaState, BT::BehaviorTreeFactory* factory);
    ~LuaBehaviorTreeBridge();
    
    // Initialize the bridge
    bool initialize();
    
    // Load behavior tree from XML file
    bool loadBehaviorTreeFromFile(const std::string& xmlFile);
    
    // Load behavior tree from XML string
    bool loadBehaviorTreeFromText(const std::string& xmlText);
    
    // Execute behavior tree
    std::string executeBehaviorTree(const std::string& treeName, 
                                     const std::string& entityId,
                                     sol::optional<sol::table> params);
    
    // Get behavior tree status
    std::string getTreeStatus(const std::string& treeId);
    
    // Stop behavior tree
    bool stopBehaviorTree(const std::string& treeId);
    
    // Set blackboard value
    bool setBlackboardValue(const std::string& treeId, 
                            const std::string& key, 
                            sol::object value);
    
    // Get blackboard value
    sol::object getBlackboardValue(const std::string& treeId, 
                                   const std::string& key);
    
    // Register Lua action node
    bool registerLuaAction(const std::string& name, sol::protected_function func);
    
    // Register Lua condition node
    bool registerLuaCondition(const std::string& name, sol::protected_function func);
    
    // Get last error
    std::string getLastError() const { return lastError_; }
    
    // Check if tree exists
    bool hasTree(const std::string& treeId);
    
private:
    sol::state* luaState_;
    BT::BehaviorTreeFactory* factory_;
    std::unordered_map<std::string, std::shared_ptr<TreeExecutionInfo>> activeTrees_;
    std::mutex treesMutex_;
    std::string lastError_;
    int treeIdCounter_;
    bool initialized_;
    
    // Generate unique tree ID
    std::string generateTreeId();
    
    // Convert Lua object to string for blackboard
    std::string luaObjectToString(sol::object obj);
    
    // Convert blackboard entry to Lua object
    sol::object blackboardEntryToLuaObject(const BT::Any& entry);
    
    // Register bridge functions to Lua
    void registerLuaAPI();
    
    // Register Lua node types to factory
    void registerLuaNodeTypes();
};

} // namespace scripting

#endif // LUA_BEHAVIOR_TREE_BRIDGE_H
