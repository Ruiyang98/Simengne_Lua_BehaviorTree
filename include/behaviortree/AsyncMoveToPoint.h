#ifndef ASYNC_MOVE_TO_POINT_H
#define ASYNC_MOVE_TO_POINT_H

#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>

namespace behaviortree {

// Async move entity to specified coordinate behavior tree node
// This node returns RUNNING while moving and SUCCESS when reached destination
class AsyncMoveToPoint : public BT::StatefulActionNode {
public:
    AsyncMoveToPoint(const std::string& name, const BT::NodeConfiguration& config);

    // Define input ports
    static BT::PortsList providedPorts();

    // Called when the node is first entered
    BT::NodeStatus onStart() override;

    // Called every tick while node is RUNNING
    BT::NodeStatus onRunning() override;

    // Called when the node is halted
    void onHalted() override;

private:
    // Target position
    double targetX_;
    double targetY_;
    double targetZ_;

    // Movement speed (units per tick)
    double speed_;

    // Arrival threshold
    double arrivalThreshold_;

    // Entity ID
    std::string entityId_;

    // Check if entity has arrived at target
    bool hasArrived(double currentX, double currentY, double currentZ) const;

    // Calculate new position moving towards target
    void moveTowards(double& currentX, double& currentY, double& currentZ) const;
};

} // namespace behaviortree

#endif // ASYNC_MOVE_TO_POINT_H
