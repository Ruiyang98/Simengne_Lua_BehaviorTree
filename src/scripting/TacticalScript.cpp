#include "scripting/TacticalScript.h"
#include <iostream>

namespace scripting {

TacticalScript::TacticalScript(const std::string& name, 
                               const std::string& scriptCode,
                               sol::state& luaState, 
                               const std::string& entityId,
                               simulation::SimControlInterface* sim)
    : Script(name, ScriptType::TACTICAL)
    , luaState_(luaState)
    , entityId_(entityId)
    , simInterface_(sim)
    , executeFunc_(sol::nil) {
    
    if (!initializeScript(scriptCode)) {
        std::cerr << "[TacticalScript] Failed to initialize script: " << name << std::endl;
    }
}

TacticalScript::~TacticalScript() {
}

bool TacticalScript::initializeScript(const std::string& scriptCode) {
    try {
        // Execute script code to define execute function
        luaState_.script(scriptCode);
        
        // Get execute function
        executeFunc_ = luaState_["execute"];
        
        if (!executeFunc_.valid()) {
            std::cerr << "[TacticalScript] No 'execute' function found in script: " << name_ << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Error initializing script '" << name_ << "': " << e.what() << std::endl;
        return false;
    }
}

void TacticalScript::execute() {
    if (!enabled_) {
        return;
    }
    
    if (!executeFunc_.valid()) {
        std::cerr << "[TacticalScript] Execute function not valid for script: " << name_ << std::endl;
        return;
    }
    
    try {
        // Call Lua execute function with entity_id and sim
        auto result = executeFunc_(entityId_, simInterface_);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Exception executing script '" << name_ << "': " << e.what() << std::endl;
    }
}

} // namespace scripting
