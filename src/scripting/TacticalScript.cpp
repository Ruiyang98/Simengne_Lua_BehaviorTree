#include "scripting/TacticalScript.h"
#include "scripting/LuaSimBinding.h"
#include <iostream>

namespace scripting {

TacticalScript::TacticalScript(const std::string& name, 
                               const std::string& scriptCode,
                               const std::string& entityId,
                               sol::table state)
    : Script(name, ScriptType::TACTICAL)
    , luaState_(&LuaSimBinding::getInstance().getState())
    , entityId_(entityId)
    , executeFunc_(sol::nil)
    , state_(state) {
    
    if (!initializeScript(scriptCode)) {
        std::cerr << "[TacticalScript] Failed to initialize script: " << name << std::endl;
    }
}

TacticalScript::~TacticalScript() {
}

bool TacticalScript::initializeScript(const std::string& scriptCode) {
    try {
        // Execute script in global environment to define execute function
        // The function will capture state when called
        auto result = luaState_->script(scriptCode);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        // Get execute function from global environment
        executeFunc_ = (*luaState_)["execute"];
        
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
        // Pass script's own state table to execute function
        auto result = executeFunc_(state_);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Exception executing script '" << name_ << "': " << e.what() << std::endl;
    }
}

} // namespace scripting
