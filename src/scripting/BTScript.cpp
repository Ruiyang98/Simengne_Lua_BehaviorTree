#include "scripting/BTScript.h"
#include "scripting/LuaSimBinding.h"
#include "behaviortree/BehaviorTreeExecutor.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace scripting {

BTScript::BTScript(const std::string& name, 
                   const std::string& scriptCode,
                   const std::string& xmlFile, 
                   const std::string& treeName,
                   const std::string& entityId,
                   sol::table state)
    : Script(name, ScriptType::BEHAVIOR_TREE)
    , luaState_(&LuaSimBinding::getInstance().getState())
    , entityId_(entityId)
    , factory_(&behaviortree::BehaviorTreeExecutor::getInstance().getFactory())
    , xmlFile_(xmlFile)
    , treeName_(treeName)
    , btInitialized_(false)
    , executeFunc_(sol::nil)
    , state_(state) {
    
    if (!scriptCode.empty()) {
        if (!initializeScript(scriptCode)) {
            std::cerr << "[BTScript] Failed to initialize Lua script: " << name << std::endl;
        }
    }
}

BTScript::~BTScript() {
    if (btInitialized_ && tree_.rootNode()) {
        tree_.haltTree();
    }
}

bool BTScript::initializeScript(const std::string& scriptCode) {
    try {
        // Execute script in global environment to define execute function
        auto result = luaState_->script(scriptCode);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[BTScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        // Get execute function from global environment
        executeFunc_ = (*luaState_)["execute"];
        
        if (!executeFunc_.valid()) {
            std::cerr << "[BTScript] No 'execute' function found in script: " << name_ << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[BTScript] Error initializing script '" << name_ << "': " << e.what() << std::endl;
        return false;
    }
}

bool BTScript::initializeBT() {
    if (btInitialized_) {
        return true;
    }
    
    if (!factory_) {
        std::cerr << "[BTScript] BehaviorTreeFactory is null" << std::endl;
        return false;
    }
    
    try {
        // Read XML file
        std::ifstream file(xmlFile_);
        if (!file.is_open()) {
            std::cerr << "[BTScript] Failed to open XML file: " << xmlFile_ << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        // Create behavior tree from XML text
        tree_ = factory_->createTreeFromText(buffer.str());
        
        // Get blackboard
        blackboard_ = tree_.rootBlackboard();
        
        // Set entity ID to blackboard
        if (blackboard_) {
            // Try to parse entity ID as vehicle ID
            try {
                int vehicleId = std::stoi(entityId_);
                VehicleID vid;
                vid.address.site = 0;
                vid.address.host = 0;
                vid.vehicle = vehicleId;
                blackboard_->set("vehicle_id", vid);
            } catch (...) {
                // If cannot parse as number, set as string
                blackboard_->set("entity_id", entityId_);
            }
        }
        
        btInitialized_ = true;
        std::cout << "[BTScript] Behavior tree initialized: " << treeName_ << " for entity " << entityId_ << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[BTScript] Error initializing behavior tree '" << treeName_ << "': " << e.what() << std::endl;
        return false;
    }
}

void BTScript::execute() {
    if (!enabled_) {
        return;
    }
    
    // 1. Execute Lua logic with state
    if (executeFunc_.valid()) {
        try {
            // Pass script's own state table to execute function
            auto result = executeFunc_(state_);
            
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[BTScript] Lua error in script '" << name_ << "': " << err.what() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[BTScript] Exception in Lua script '" << name_ << "': " << e.what() << std::endl;
        }
    }
    
    // 2. Initialize behavior tree (if not initialized yet)
    if (!btInitialized_) {
        if (!initializeBT()) {
            std::cerr << "[BTScript] Failed to initialize behavior tree for script: " << name_ << std::endl;
            return;
        }
    }
    
    // 3. Directly tick behavior tree
    if (tree_.rootNode()) {
        try {
            BT::NodeStatus status = tree_.tickRoot();
            
            // If behavior tree completed, output status
            if (status != BT::NodeStatus::RUNNING) {
                std::cout << "[BTScript] Behavior tree '" << treeName_ << "' completed with status: " 
                          << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE") << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[BTScript] Error ticking behavior tree '" << treeName_ << "': " << e.what() << std::endl;
        }
    }
}

BT::NodeStatus BTScript::getStatus() const {
    if (!btInitialized_ || !tree_.rootNode()) {
        return BT::NodeStatus::IDLE;
    }
    
    return tree_.rootNode()->status();
}

} // namespace scripting
