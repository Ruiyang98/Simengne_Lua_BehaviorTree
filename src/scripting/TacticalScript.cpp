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
        // Create isolated environment for this script to avoid global namespace pollution
        sol::environment env(*luaState_, sol::new_table(), luaState_->globals());

        // Execute script in isolated environment
        auto result = luaState_->script(scriptCode, env);

        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }

        // Get execute function from isolated environment
        executeFunc_ = env["execute"];

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
