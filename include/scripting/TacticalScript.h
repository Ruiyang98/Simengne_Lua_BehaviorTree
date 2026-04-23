#ifndef TACTICAL_SCRIPT_H
#define TACTICAL_SCRIPT_H

#include "scripting/Script.h"
#include "simulation/SimControlInterface.h"
#include <sol.hpp>
#include <string>

namespace scripting {

// Pure Lua tactical script
// Each script has its own state table for isolation
class TacticalScript : public Script {
public:
    TacticalScript(const std::string& name, 
                   const std::string& scriptCode,
                   const std::string& entityId,
                   sol::table state);
    
    ~TacticalScript() override;
    
    // Execute script
    void execute() override;
    
    // Get script state
    sol::table& getState() { return state_; }
    
private:
    sol::state* luaState_;
    std::string entityId_;
    sol::function executeFunc_;
    sol::table state_;  // Script's own state table
    
    // Initialize Lua script
    bool initializeScript(const std::string& scriptCode);
};

} // namespace scripting

#endif // TACTICAL_SCRIPT_H
