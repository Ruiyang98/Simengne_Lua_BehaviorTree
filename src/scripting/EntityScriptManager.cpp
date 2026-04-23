#include "scripting/EntityScriptManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace scripting {

EntityScriptManager::EntityScriptManager(const std::string& entityId, 
                                         sol::state& globalLuaState,
                                         BT::BehaviorTreeFactory* factory)
    : entityId_(entityId)
    , factory_(factory)
    , luaState_(globalLuaState) {
    
    initializeEntityTable();
    createSandbox();
}

EntityScriptManager::~EntityScriptManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    scripts_.clear();
}

void EntityScriptManager::initializeEntityTable() {
    try {
        // Create entity table
        entityTable_ = luaState_.create_table();
        
        // Set entity.id
        entityTable_["id"] = entityId_;
        
        // Create vars sub-table for variable storage
        variables_ = luaState_.create_table();
        entityTable_["vars"] = variables_;
        
        // Register variable operation methods
        entityTable_.set_function("set_var", [this](const std::string& key, sol::object value) {
            variables_[key] = value;
        });
        
        entityTable_.set_function("get_var", [this](const std::string& key, sol::object defaultValue) -> sol::object {
            sol::object val = variables_[key];
            if (val == sol::nil && defaultValue != sol::nil) {
                return defaultValue;
            }
            return val;
        });
        
        entityTable_.set_function("has_var", [this](const std::string& key) -> bool {
            sol::object val = variables_[key];
            return val != sol::nil;
        });
        
        entityTable_.set_function("remove_var", [this](const std::string& key) {
            variables_[key] = sol::nil;
        });
        
        entityTable_.set_function("clear_vars", [this]() {
            variables_ = luaState_.create_table();
            entityTable_["vars"] = variables_;
        });
        
        entityTable_.set_function("get_all_vars", [this]() -> sol::table {
            return variables_;
        });
        
        // Store entity table in Lua registry using entityId as key
        // This allows scripts to access via global variable, but each entity has its own table
        luaState_["_ENTITIES"] = luaState_["_ENTITIES"] || luaState_.create_table();
        luaState_["_ENTITIES"][entityId_] = entityTable_;
        
    } catch (const std::exception& e) {
        std::cerr << "[EntityScriptManager] Error initializing entity table: " << e.what() << std::endl;
    }
}

void EntityScriptManager::createSandbox() {
    try {
        // Create sandbox environment table
        env_ = luaState_.create_table();
        
        // Set sandbox accessible global variables
        // Allow access to standard libraries
        env_["print"] = luaState_["print"];
        env_["pairs"] = luaState_["pairs"];
        env_["ipairs"] = luaState_["ipairs"];
        env_["next"] = luaState_["next"];
        env_["tonumber"] = luaState_["tonumber"];
        env_["tostring"] = luaState_["tostring"];
        env_["type"] = luaState_["type"];
        env_["math"] = luaState_["math"];
        env_["table"] = luaState_["table"];
        env_["string"] = luaState_["string"];
        env_["coroutine"] = luaState_["coroutine"];
        env_["os"] = luaState_["os"];
        
        // Allow access to sim and bt tables
        env_["sim"] = luaState_["sim"];
        env_["bt"] = luaState_["bt"];
        
        // Set entity table
        env_["entity"] = entityTable_;
        
        // Set metatable to allow access to other global variables (read-only)
        sol::table metaTable = luaState_.create_table();
        metaTable.set_function("__index", [this](sol::table t, const std::string& key) -> sol::object {
            // First search in env_
            sol::object val = env_[key];
            if (val != sol::nil) {
                return val;
            }
            // Then search globally
            return luaState_[key];
        });
        
        env_[sol::metatable_key] = metaTable;
        
    } catch (const std::exception& e) {
        std::cerr << "[EntityScriptManager] Error creating sandbox: " << e.what() << std::endl;
    }
}

bool EntityScriptManager::addTacticalScript(const std::string& scriptName, const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        auto script = std::make_shared<TacticalScript>(
            scriptName, scriptCode, luaState_, entityId_, env_);
        
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
        auto script = std::make_shared<BTScript>(
            scriptName, scriptCode, xmlFile, treeName,
            luaState_, entityId_, factory_, env_);
        
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
    
    // Update entity table reference (prevent Lua GC)
    env_["entity"] = entityTable_;
    
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

} // namespace scripting
