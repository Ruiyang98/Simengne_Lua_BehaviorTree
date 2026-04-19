#include "behaviortree/MoveToPoint.h"
#include "behaviortree/SimControllerPtr.h"
#include "behaviortree/EntityMovement.h"
#include <iostream>

namespace behaviortree {

MoveToPoint::MoveToPoint(const std::string& name, const BT::NodeConfiguration& config)
    : BT::SyncActionNode(name, config)
{
}

BT::PortsList MoveToPoint::providedPorts() {
    return {
        BT::InputPort<std::string>("entity_id", "Entity ID to move"),
        BT::InputPort<double>("x", "Target X coordinate"),
        BT::InputPort<double>("y", "Target Y coordinate"),
        BT::InputPort<double>("z", 0.0, "Target Z coordinate (default 0.0)")
    };
}

BT::NodeStatus MoveToPoint::tick() {
    // Get input parameters
    BT::Optional<std::string> entity_id = getInput<std::string>("entity_id");
    BT::Optional<double> target_x = getInput<double>("x");
    BT::Optional<double> target_y = getInput<double>("y");
    BT::Optional<double> target_z = getInput<double>("z");
    
    // Check required parameters
    if (!entity_id) {
        std::cerr << "[MoveToPoint] Missing required input: entity_id" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    if (!target_x) {
        std::cerr << "[MoveToPoint] Missing required input: x" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    if (!target_y) {
        std::cerr << "[MoveToPoint] Missing required input: y" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Get simulation controller
    simulation::SimControlInterface* sim = getSimController();
    
    // Use centralized movement logic
    double z = target_z.value_or(0.0);
    return moveEntityToPosition(sim, entity_id.value(), target_x.value(), target_y.value(), z);
}

} // namespace behaviortree
