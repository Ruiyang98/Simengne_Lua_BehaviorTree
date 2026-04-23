#ifndef ENTITY_SCRIPT_MANAGER_H
#define ENTITY_SCRIPT_MANAGER_H

#include "scripting/Script.h"
#include "scripting/TacticalScript.h"
#include "scripting/BTScript.h"
#include "simulation/SimControlInterface.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace scripting {

// Entity script manager - one per entity
class EntityScriptManager {
public:
    // Constructor
    EntityScriptManager(const std::string& entityId, 
                        BT::BehaviorTreeFactory* factory);
    
    ~EntityScriptManager();
    
    // Add pure Lua tactical script
    bool addTacticalScript(const std::string& scriptName, const std::string& scriptCode);
    bool addTacticalScriptFromFile(const std::string& scriptName, const std::string& filePath);
    
    // Add Lua + Behavior Tree hybrid script
    bool addBTScript(const std::string& scriptName, 
                     const std::string& scriptCode,
                     const std::string& xmlFile, 
                     const std::string& treeName);
    
    // Remove script
    bool removeScript(const std::string& scriptName);
    
    // Enable/disable script
    bool enableScript(const std::string& scriptName);
    bool disableScript(const std::string& scriptName);
    
    // Execute all scripts (called by simulation engine every 500ms)
    void executeAllScripts();
    
    // Get entity ID
    const std::string& getEntityId() const { return entityId_; }
    
    // Get script list
    std::vector<std::string> getScriptNames() const;
    
    // Check if script exists
    bool hasScript(const std::string& scriptName) const;
    
    // Get script count
    size_t getScriptCount() const;
    
    // Get script
    std::shared_ptr<Script> getScript(const std::string& scriptName) const;
    
    // Get Lua state
    sol::state& getLuaState() { return luaState_; }
    
    // Get last error message
    const std::string& getLastError() const { return lastError_; }
    
private:
    std::string entityId_;
    BT::BehaviorTreeFactory* factory_;
    std::unordered_map<std::string, std::shared_ptr<Script>> scripts_;
    mutable std::mutex mutex_;
    sol::state luaState_;  // Each manager has its own Lua state
    std::string lastError_;
    
    // Initialize Lua state
    void initializeLuaState();
    
    // Register Lua API
    void registerLuaAPI();
};

} // namespace scripting

#endif // ENTITY_SCRIPT_MANAGER_H
