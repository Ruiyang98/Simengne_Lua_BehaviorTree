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
// Uses global Lua state
// Each script has its own state table for isolation
class EntityScriptManager {
public:
    // Constructor - receives global Lua state reference
    EntityScriptManager(const std::string& entityId, 
                        sol::state& globalLuaState,
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
    
    // Get Lua state (global state)
    sol::state& getLuaState() { return luaState_; }
    
    // Get entity table (contains only id)
    sol::table& getEntityTable() { return entityTable_; }
    
    // Get script state table
    sol::optional<sol::table> getScriptState(const std::string& scriptName);
    
    // Clear script state
    void clearScriptState(const std::string& scriptName);
    
    // Get last error message
    const std::string& getLastError() const { return lastError_; }
    
private:
    std::string entityId_;
    BT::BehaviorTreeFactory* factory_;
    sol::state& luaState_;  // Reference to global Lua state
    
    // Entity table (contains only id)
    sol::table entityTable_;
    
    // Script states: scriptName -> state table
    std::unordered_map<std::string, sol::table> scriptStates_;
    
    std::unordered_map<std::string, std::shared_ptr<Script>> scripts_;
    mutable std::mutex mutex_;
    std::string lastError_;
    
    // Initialize entity table
    void initializeEntityTable();
};

} // namespace scripting

#endif // ENTITY_SCRIPT_MANAGER_H
