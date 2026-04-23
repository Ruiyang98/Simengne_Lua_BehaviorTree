#ifndef BT_SCRIPT_H
#define BT_SCRIPT_H

#include "scripting/Script.h"
#include "simulation/SimControlInterface.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <sol.hpp>
#include <string>
#include <memory>

namespace scripting {

// Lua + Behavior Tree hybrid script
class BTScript : public Script {
public:
    BTScript(const std::string& name, 
             const std::string& scriptCode,
             const std::string& xmlFile, 
             const std::string& treeName,
             sol::state& luaState, 
             const std::string& entityId,
             simulation::SimControlInterface* sim,
             BT::BehaviorTreeFactory* factory);
    
    ~BTScript() override;
    
    // Execute script
    void execute() override;
    
    // Initialize behavior tree
    bool initializeBT();
    
    // Get behavior tree status
    BT::NodeStatus getStatus() const;
    
    // Check if behavior tree is initialized
    bool isBTInitialized() const { return btInitialized_; }
    
private:
    sol::state& luaState_;
    std::string entityId_;
    simulation::SimControlInterface* simInterface_;
    BT::BehaviorTreeFactory* factory_;
    
    // Behavior tree related
    std::string xmlFile_;
    std::string treeName_;
    BT::Tree tree_;
    std::shared_ptr<BT::Blackboard> blackboard_;
    bool btInitialized_;
    
    // Lua execute function
    sol::function executeFunc_;
    
    // Initialize Lua script
    bool initializeScript(const std::string& scriptCode);
};

} // namespace scripting

#endif // BT_SCRIPT_H
