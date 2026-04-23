#ifndef TACTICAL_SCRIPT_H
#define TACTICAL_SCRIPT_H

#include "scripting/Script.h"
#include "simulation/SimControlInterface.h"
#include <sol.hpp>
#include <string>

namespace scripting {

// Pure Lua tactical script
// Executes in sandbox environment, uses entity.vars to store variables
class TacticalScript : public Script {
public:
    TacticalScript(const std::string& name, 
                   const std::string& scriptCode,
                   sol::state& luaState, 
                   const std::string& entityId,
                   sol::table& env);
    
    ~TacticalScript() override;
    
    // Execute script
    void execute() override;
    
private:
    sol::state& luaState_;
    std::string entityId_;
    sol::function executeFunc_;  // Lua execute function
    sol::table env_;  // Sandbox environment
    
    // Initialize Lua script
    bool initializeScript(const std::string& scriptCode);
};

} // namespace scripting

#endif // TACTICAL_SCRIPT_H
