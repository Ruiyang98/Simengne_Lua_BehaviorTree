#ifndef MOVE_TO_POINT_H
#define MOVE_TO_POINT_H

#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/behavior_tree.h>

namespace behaviortree {

// Move entity to specified coordinate behavior tree node
class MoveToPoint : public BT::SyncActionNode {
public:
    MoveToPoint(const std::string& name, const BT::NodeConfiguration& config);
    
    // Define input ports
    static BT::PortsList providedPorts();
    
    // Execute move operation
    BT::NodeStatus tick() override;
};

} // namespace behaviortree

#endif // MOVE_TO_POINT_H
