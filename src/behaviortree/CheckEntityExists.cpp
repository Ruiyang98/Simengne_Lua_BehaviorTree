#include "behaviortree/CheckEntityExists.h"
#include "behaviortree/SimControllerPtr.h"
#include <iostream>

namespace behaviortree {

CheckEntityExists::CheckEntityExists(const std::string& name, const BT::NodeConfiguration& config)
    : BT::ConditionNode(name, config)
{
}

BT::PortsList CheckEntityExists::providedPorts() {
    return {
        BT::InputPort<simulation::VehicleID>("vehicle_id", "Vehicle ID to check")
    };
}

BT::NodeStatus CheckEntityExists::tick() {
    // Get input parameters
    BT::Optional<simulation::VehicleID> vehicle_id = getInput<simulation::VehicleID>("vehicle_id");

    // Check required parameters
    if (!vehicle_id) {
        std::cerr << "[CheckEntityExists] Missing required input: vehicle_id" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Get simulation controller
    simulation::SimControlInterface* sim = getSimController();
    if (!sim) {
        std::cerr << "[CheckEntityExists] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // Check if entity exists
    double x, y, z;
    if (sim->getEntityPosition(vehicle_id.value(), x, y, z)) {
        std::cout << "[CheckEntityExists] Entity vehicle=" << vehicle_id.value().vehicle << " exists at ("
                  << x << ", " << y << ", " << z << ")" << std::endl;
        return BT::NodeStatus::SUCCESS;
    } else {
        std::cout << "[CheckEntityExists] Entity vehicle=" << vehicle_id.value().vehicle << " does not exist" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
}

} // namespace behaviortree
