#ifndef LUA_SIM_BINDING_H
#define LUA_SIM_BINDING_H

#include "simulation/SimControlInterface.h"
#include <sol.hpp>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace scripting {

class LuaSimBinding {
public:
    explicit LuaSimBinding(simulation::SimControlInterface* simInterface);
    ~LuaSimBinding();

    bool initialize();
    bool executeScript(const std::string& scriptPath);
    bool executeString(const std::string& scriptCode);
    sol::state& getState();
    bool isInitialized() const;
    const std::string& getLastError() const;

private:
    void registerFunctions();
    void registerSimAPI();
    void registerUtilityFunctions();
    void setupCallbacks();

    simulation::SimControlInterface* simInterface_;
    std::unique_ptr<sol::state> luaState_;
    bool initialized_;
    std::string lastError_;
    std::vector<sol::protected_function> luaCallbacks_;
};

} // namespace scripting

#endif // LUA_SIM_BINDING_H
