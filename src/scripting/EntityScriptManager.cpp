#include "scripting/EntityScriptManager.h"
#include "scripting/LuaSimBinding.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace scripting {

EntityScriptManager::EntityScriptManager(const std::string& entityId)
    : entityId_(entityId)
    , luaState_(&LuaSimBinding::getInstance().getState()) {
    
    initializeEntityTable();
}

EntityScriptManager::~EntityScriptManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    scripts_.clear();
    scriptStates_.clear();
}

void EntityScriptManager::initializeEntityTable() {
    try {
        // Create entity table with only id
        entityTable_ = luaState_->create_table();
        
        // Set entity.id
        entityTable_["id"] = entityId_;
        
        // Store entity table in Lua registry using entityId as key
        (*luaState_)["_ENTITIES"] = (*luaState_)["_ENTITIES"] || luaState_->create_table();
        (*luaState_)["_ENTITIES"][entityId_] = entityTable_;
        
        // Set global entity table for this entity (scripts can access via 'entity')
        (*luaState_)["entity"] = entityTable_;
        
    } catch (const std::exception& e) {
        std::cerr << "[EntityScriptManager] Error initializing entity table: " << e.what() << std::endl;
    }
}

bool EntityScriptManager::addTacticalScript(const std::string& scriptName, const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        // Create script state table
        sol::table scriptState = luaState_->create_table();
        scriptState["_script_name"] = scriptName;
        scriptStates_[scriptName] = scriptState;
        
        // Create script with state
        auto script = std::make_shared<TacticalScript>(
            scriptName, scriptCode, entityId_, scriptState);
        
        scripts_[scriptName] = script;
        std::cout << "[EntityScriptManager] Added tactical script '" << scriptName 
                  << "' for entity " << entityId_ << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create tactical script: ") + e.what();
        return false;
    }
}

bool EntityScriptManager::addTacticalScriptFromFile(const std::string& scriptName, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        lastError_ = "Cannot open script file: " + filePath;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return addTacticalScript(scriptName, buffer.str());
}

bool EntityScriptManager::addBTScript(const std::string& scriptName, 
                                      const std::string& scriptCode,
                                      const std::string& xmlFile, 
                                      const std::string& treeName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        // Create script state table
        sol::table scriptState = luaState_->create_table();
        scriptState["_script_name"] = scriptName;
        scriptStates_[scriptName] = scriptState;
        
        // Create script with state
        auto script = std::make_shared<BTScript>(
            scriptName, scriptCode, xmlFile, treeName,
            entityId_, scriptState);
        
        scripts_[scriptName] = script;
        std::cout << "[EntityScriptManager] Added BT script '" << scriptName 
                  << "' for entity " << entityId_ << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create BT script: ") + e.what();
        return false;
    }
}

bool EntityScriptManager::removeScript(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scripts_.find(scriptName);
    if (it == scripts_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return false;
    }
    
    scripts_.erase(it);
    
    // Clear script state
    auto stateIt = scriptStates_.find(scriptName);
    if (stateIt != scriptStates_.end()) {
        scriptStates_.erase(stateIt);
    }
    
    std::cout << "[EntityScriptManager] Removed script '" << scriptName 
              << "' for entity " << entityId_ << std::endl;
    
    return true;
}

bool EntityScriptManager::enableScript(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scripts_.find(scriptName);
    if (it == scripts_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return false;
    }
    
    it->second->setEnabled(true);
    return true;
}

bool EntityScriptManager::disableScript(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scripts_.find(scriptName);
    if (it == scripts_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return false;
    }
    
    it->second->setEnabled(false);
    return true;
}

void EntityScriptManager::executeAllScripts() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update global entity table reference
    (*luaState_)["entity"] = entityTable_;
    
    for (auto& pair : scripts_) {
        try {
            pair.second->execute();
        } catch (const std::exception& e) {
            std::cerr << "[EntityScriptManager] Error executing script '" 
                      << pair.first << "': " << e.what() << std::endl;
        }
    }
}

std::vector<std::string> EntityScriptManager::getScriptNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    names.reserve(scripts_.size());
    
    for (const auto& pair : scripts_) {
        names.push_back(pair.first);
    }
    
    return names;
}

bool EntityScriptManager::hasScript(const std::string& scriptName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scripts_.find(scriptName) != scripts_.end();
}

size_t EntityScriptManager::getScriptCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scripts_.size();
}

std::shared_ptr<Script> EntityScriptManager::getScript(const std::string& scriptName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scripts_.find(scriptName);
    if (it != scripts_.end()) {
        return it->second;
    }
    
    return nullptr;
}

sol::state& EntityScriptManager::getLuaState() {
    return *luaState_;
}

sol::optional<sol::table> EntityScriptManager::getScriptState(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        return it->second;
    }
    
    return sol::nullopt;
}

void EntityScriptManager::clearScriptState(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        // Create new empty table
        it->second = luaState_->create_table();
        it->second["_script_name"] = scriptName;
    }
}

// ========== C++ Interface Implementation for Modifying Script Parameters ==========

void EntityScriptManager::setScriptParam(const std::string& scriptName, 
                                          const std::string& key, 
                                          sol::object value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        it->second[key] = value;
    }
}

sol::optional<sol::object> EntityScriptManager::getScriptParam(
    const std::string& scriptName, 
    const std::string& key) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        if (it->second[key].valid()) {
            return it->second[key];
        }
    }
    return sol::nullopt;
}

void EntityScriptManager::setScriptWaypoints(
    const std::string& scriptName,
    const std::vector<std::tuple<double, double, double>>& waypoints) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it == scriptStates_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return;
    }
    
    sol::table wpTable = luaState_->create_table();
    for (size_t i = 0; i < waypoints.size(); ++i) {
        sol::table point = luaState_->create_table();
        point["x"] = std::get<0>(waypoints[i]);
        point["y"] = std::get<1>(waypoints[i]);
        point["z"] = std::get<2>(waypoints[i]);
        wpTable[i + 1] = point; // Lua array is 1-indexed
    }
    
    it->second["waypoints"] = wpTable;
}

std::vector<std::tuple<double, double, double>> EntityScriptManager::getScriptWaypoints(
    const std::string& scriptName) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::tuple<double, double, double>> result;
    
    auto it = scriptStates_.find(scriptName);
    if (it == scriptStates_.end()) {
        return result;
    }
    
    sol::table waypoints = it->second["waypoints"];
    if (!waypoints.valid()) {
        return result;
    }
    
    // Lua array is 1-indexed
    for (size_t i = 1; ; ++i) {
        sol::table point = waypoints[i];
        if (!point.valid()) {
            break;
        }
        double x = point["x"].get_or(0.0);
        double y = point["y"].get_or(0.0);
        double z = point["z"].get_or(0.0);
        result.emplace_back(x, y, z);
    }
    
    return result;
}

void EntityScriptManager::setEntityField(const std::string& key, sol::object value) {
    std::lock_guard<std::mutex> lock(mutex_);
    entityTable_[key] = value;
}

sol::object EntityScriptManager::getEntityField(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entityTable_[key].valid()) {
        return entityTable_[key];
    }
    return sol::nil;
}

} // namespace scripting
