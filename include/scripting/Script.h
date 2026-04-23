#ifndef SCRIPT_H
#define SCRIPT_H

#include <string>

namespace scripting {

// Script type enumeration
enum class ScriptType {
    TACTICAL,       // Pure Lua tactical rules
    BEHAVIOR_TREE   // Lua + Behavior Tree hybrid
};

// Script base class
class Script {
public:
    Script(const std::string& name, ScriptType type);
    virtual ~Script();
    
    // Execute script (pure virtual, implemented by subclasses)
    virtual void execute() = 0;
    
    // Getters
    const std::string& getName() const { return name_; }
    ScriptType getType() const { return type_; }
    bool isEnabled() const { return enabled_; }
    
    // Setters
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
protected:
    std::string name_;
    ScriptType type_;
    bool enabled_;
};

} // namespace scripting

#endif // SCRIPT_H
