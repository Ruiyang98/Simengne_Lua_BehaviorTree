#include "scripting/TacticalScript.h"
#include <iostream>

namespace scripting {

TacticalScript::TacticalScript(const std::string& name, 
                               const std::string& scriptCode,
                               sol::state& luaState, 
                               const std::string& entityId,
                               sol::table& env)
    : Script(name, ScriptType::TACTICAL)
    , luaState_(luaState)
    , entityId_(entityId)
    , executeFunc_(sol::nil)
    , env_(env) {
    
    if (!initializeScript(scriptCode)) {
        std::cerr << "[TacticalScript] Failed to initialize script: " << name << std::endl;
    }
}

TacticalScript::~TacticalScript() {
}

bool TacticalScript::initializeScript(const std::string& scriptCode) {
    try {
        // Load script in sandbox environment
        // Use load function to load script in specified environment
        sol::load_result loadResult = luaState_.load(scriptCode);
        
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            std::cerr << "[TacticalScript] Error loading script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        sol::function scriptFunc = loadResult;
        
        // Execute script to define execute function in sandbox environment
        sol::environment env(luaState_, sol::create, env_);
        auto result = luaState_.script(scriptCode, env);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        // Get execute function from sandbox environment
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
        // Call execute function in sandbox environment
        // No longer pass entity_id, sim, etc. - scripts access via entity table
        auto result = executeFunc_();
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Exception executing script '" << name_ << "': " << e.what() << std::endl;
    }
}

} // namespace scripting
