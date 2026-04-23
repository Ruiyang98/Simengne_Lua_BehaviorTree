#ifndef SELECT_TARGET_FROM_LIST_H
#define SELECT_TARGET_FROM_LIST_H

#include <behaviortree_cpp_v3/action_node.h>

namespace behaviortree {

class SelectTargetFromList : public BT::SyncActionNode {
public:
    SelectTargetFromList(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;
};

} // namespace behaviortree

#endif // SELECT_TARGET_FROM_LIST_H
