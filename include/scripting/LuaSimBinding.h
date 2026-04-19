#ifndef LUA_SIM_BINDING_H
#define LUA_SIM_BINDING_H

#include "simulation/SimControlInterface.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <sol.hpp>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace scripting {

// Forward declaration
class LuaBehaviorTreeBridge;

class LuaSimBinding {
public:
    explicit LuaSimBinding(simulation::SimControlInterface* simInterface);
    ~LuaSimBinding();

    bool initialize();
    bool executeScript(const std::string& scriptPath);
    bool executeString(const std::string& scriptCode);
    sol::state& getState();
    bool isInitialized() const;
    const std::string& getLastError() const;
    
    // Get behavior tree bridge
    LuaBehaviorTreeBridge* getBehaviorTreeBridge() const { return btBridge_.get(); }
    
    // Initialize behavior tree integration (must be called after initialize())
    bool initializeBehaviorTree(BT::BehaviorTreeFactory* factory);
    bool isBehaviorTreeInitialized() const { return btBridge_ != nullptr; }

private:
    void registerFunctions();
    void registerSimAPI();
    void registerUtilityFunctions();
    void setupCallbacks();

    simulation::SimControlInterface* simInterface_;
    std::unique_ptr<sol::state> luaState_;
    std::unique_ptr<LuaBehaviorTreeBridge> btBridge_;
    bool initialized_;
    std::string lastError_;
    std::vector<sol::protected_function> luaCallbacks_;
};

} // namespace scripting

#endif // LUA_SIM_BINDING_H
