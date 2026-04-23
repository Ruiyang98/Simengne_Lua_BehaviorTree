#include "scripting/EntityScriptManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace scripting {

EntityScriptManager::EntityScriptManager(const std::string& entityId, 
                                         simulation::SimControlInterface* sim,
                                         BT::BehaviorTreeFactory* factory)
    : entityId_(entityId)
    , simInterface_(sim)
    , factory_(factory) {
    
    initializeLuaState();
    registerLuaAPI();
}

EntityScriptManager::~EntityScriptManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    scripts_.clear();
}

void EntityScriptManager::initializeLuaState() {
    try {
        // Open standard libraries
        luaState_.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::os,
            sol::lib::math,
            sol::lib::table,
            sol::lib::debug,
            sol::lib::bit32,
            sol::lib::io
        );
        
        // Set entity_id global variable
        luaState_["entity_id"] = entityId_;
        
    } catch (const std::exception& e) {
        std::cerr << "[EntityScriptManager] Error initializing Lua state: " << e.what() << std::endl;
    }
}

void EntityScriptManager::registerLuaAPI() {
    // Register SimAddress type
    luaState_.new_usertype<simulation::SimAddress>("SimAddress",
        "site", &simulation::SimAddress::site,
        "host", &simulation::SimAddress::host
    );

    // Register VehicleID type
    luaState_.new_usertype<simulation::VehicleID>("VehicleID",
        "address", &simulation::VehicleID::address,
        "vehicle", &simulation::VehicleID::vehicle
    );
    
    // Create sim table
    sol::table simTable = luaState_.create_named_table("sim");
    
    // Entity management functions
    simTable.set_function("get_entity_position", [this](const std::string& entityId) -> sol::optional<sol::table> {
        if (!simInterface_) {
            return sol::nullopt;
        }
        
        // Try to parse as vehicle ID
        try {
            int vehicleId = std::stoi(entityId);
            simulation::VehicleID vid;
            vid.address.site = 0;
            vid.address.host = 0;
            vid.vehicle = vehicleId;
            
            double x, y, z;
            if (simInterface_->getEntityPosition(vid, x, y, z)) {
                sol::table pos = luaState_.create_table();
                pos["x"] = x;
                pos["y"] = y;
                pos["z"] = z;
                return pos;
            }
        } catch (...) {
            // If not numeric ID, return empty
        }
        
        return sol::nullopt;
    });
    
    simTable.set_function("get_all_entities", [this]() -> sol::table {
        sol::table entities = luaState_.create_table();
        
        if (simInterface_) {
            auto entityList = simInterface_->getAllEntities();
            for (size_t i = 0; i < entityList.size(); ++i) {
                sol::table entity = luaState_.create_table();
                entity["id"] = entityList[i].id.vehicle;
                entity["type"] = entityList[i].type;
                entity["x"] = entityList[i].x;
                entity["y"] = entityList[i].y;
                entity["z"] = entityList[i].z;
                entities[i + 1] = entity;  // Lua arrays start from 1
            }
        }
        
        return entities;
    });
    
    simTable.set_function("move_entity", [this](const std::string& entityId, double x, double y, double z) -> bool {
        if (!simInterface_) {
            return false;
        }
        
        try {
            int vehicleId = std::stoi(entityId);
            simulation::VehicleID vid;
            vid.address.site = 0;
            vid.address.host = 0;
            vid.vehicle = vehicleId;
            
            return simInterface_->moveEntity(vid, x, y, z);
        } catch (...) {
            return false;
        }
    });
    
    simTable.set_function("get_entity_distance", [this](const std::string& entityId, double x, double y, double z) -> double {
        if (!simInterface_) {
            return -1.0;
        }
        
        try {
            int vehicleId = std::stoi(entityId);
            simulation::VehicleID vid;
            vid.address.site = 0;
            vid.address.host = 0;
            vid.vehicle = vehicleId;
            
            return simInterface_->getEntityDistance(vid, x, y, z);
        } catch (...) {
            return -1.0;
        }
    });
    
    // Create bt table (blackboard operations)
    sol::table btTable = luaState_.create_named_table("bt");
    
    // Note: blackboard operations need to be implemented in BTScript
    // Here we register basic functions, actual functionality provided by BTScript
    btTable.set_function("set_blackboard", [this](const std::string& entityId, const std::string& key, sol::object value) {
        // This function will be overridden in BTScript to provide actual blackboard access
        // Here is just a placeholder
    });
    
    btTable.set_function("get_blackboard", [this](const std::string& entityId, const std::string& key) -> sol::object {
        // Placeholder
        return sol::nil;
    });
}

bool EntityScriptManager::addTacticalScript(const std::string& scriptName, const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        auto script = std::make_shared<TacticalScript>(
            scriptName, scriptCode, luaState_, entityId_, simInterface_);
        
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
            luaState_, entityId_, simInterface_, factory_);
        
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
