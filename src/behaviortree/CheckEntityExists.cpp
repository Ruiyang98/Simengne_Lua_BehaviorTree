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
        BT::InputPort<std::string>("entity_id", "Entity ID to check")
    };
}

BT::NodeStatus CheckEntityExists::tick() {
    // Get input parameters
    BT::Optional<std::string> entity_id = getInput<std::string>("entity_id");
    
    // Check required parameters
    if (!entity_id) {
        std::cerr << "[CheckEntityExists] Missing required input: entity_id" << std::endl;
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
    if (sim->getEntityPosition(entity_id.value(), x, y, z)) {
        std::cout << "[CheckEntityExists] Entity " << entity_id.value() << " exists at ("
                  << x << ", " << y << ", " << z << ")" << std::endl;
        return BT::NodeStatus::SUCCESS;
    } else {
        std::cout << "[CheckEntityExists] Entity " << entity_id.value() << " does not exist" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
}

} // namespace behaviortree
