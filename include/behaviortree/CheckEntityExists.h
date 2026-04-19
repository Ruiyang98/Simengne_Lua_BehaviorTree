#ifndef CHECK_ENTITY_EXISTS_H
#define CHECK_ENTITY_EXISTS_H

#include <behaviortree_cpp_v3/condition_node.h>
#include <behaviortree_cpp_v3/behavior_tree.h>

namespace behaviortree {

// Check if entity exists condition node
class CheckEntityExists : public BT::ConditionNode {
public:
    CheckEntityExists(const std::string& name, const BT::NodeConfiguration& config);
    
    // Define input ports
    static BT::PortsList providedPorts();
    
    // Execute check
    BT::NodeStatus tick() override;
};

} // namespace behaviortree

#endif // CHECK_ENTITY_EXISTS_H
