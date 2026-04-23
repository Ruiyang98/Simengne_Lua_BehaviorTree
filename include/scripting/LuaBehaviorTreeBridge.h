#ifndef LUA_BEHAVIOR_TREE_BRIDGE_H
#define LUA_BEHAVIOR_TREE_BRIDGE_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <sol.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <chrono>

#include "scripting/LuaBehaviorTreeNodes.h"

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

// Bridge class between Lua and BehaviorTree.CPP
// Dependencies are obtained from singletons:
// - sol::state from LuaSimBinding::getInstance()
// - BT::BehaviorTreeFactory from BehaviorTreeExecutor::getInstance()
class LuaBehaviorTreeBridge {
public:
    LuaBehaviorTreeBridge();
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

    // Register Lua stateful action node
    bool registerLuaStatefulAction(const std::string& name,
                                   sol::protected_function onStart,
                                   sol::protected_function onRunning,
                                   sol::protected_function onHalted);

    // Get last error
    std::string getLastError() const { return lastError_; }
    
    // Check if tree exists
    bool hasTree(const std::string& treeId);

    // Load nodes registry from Lua file
    bool loadNodesRegistry(const std::string& registryPath);

    // Preload behavior trees from directory
    bool preloadBehaviorTreesFromDirectory(const std::string& directory);

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

    // Set of loaded tree definitions
    std::unordered_set<std::string> loadedTreeDefinitions_;
};

} // namespace scripting

#endif // LUA_BEHAVIOR_TREE_BRIDGE_H
