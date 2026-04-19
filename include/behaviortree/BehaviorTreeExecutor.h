#ifndef BEHAVIOR_TREE_EXECUTOR_H
#define BEHAVIOR_TREE_EXECUTOR_H

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>
#include <memory>
#include "simulation/SimControlInterface.h"

namespace behaviortree {

// Behavior tree executor: manages loading and execution of behavior trees
class BehaviorTreeExecutor {
public:
    BehaviorTreeExecutor(simulation::SimControlInterface* simController);
    ~BehaviorTreeExecutor();
    
    // Initialize: register custom nodes
    bool initialize();
    
    // Load behavior tree from XML file
    bool loadFromFile(const std::string& xmlFile);
    
    // Load behavior tree from XML string
    bool loadFromText(const std::string& xmlText);
    
    // Execute behavior tree
    BT::NodeStatus execute(const std::string& treeName = "MainTree", 
                           BT::Blackboard::Ptr blackboard = nullptr);
    
    // Get last error message
    std::string getLastError() const { return lastError_; }
    
    // Get factory (for advanced configuration)
    BT::BehaviorTreeFactory& getFactory() { return factory_; }
    
private:
    simulation::SimControlInterface* simController_;
    BT::BehaviorTreeFactory factory_;
    std::string lastError_;
    bool initialized_;
    
    // Register custom nodes
    void registerNodes();
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_EXECUTOR_H
