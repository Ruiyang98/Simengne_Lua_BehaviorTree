#include "behaviortree/EntityMovement.h"
#include <iostream>

namespace behaviortree {

BT::NodeStatus moveEntityToPosition(simulation::SimControlInterface* sim,
                                    const std::string& entity_id, 
                                    double x, double y, double z) {
    if (!sim) {
        std::cerr << "[EntityMovement] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Check if entity exists
    double current_x, current_y, current_z;
    if (!sim->getEntityPosition(entity_id, current_x, current_y, current_z)) {
        std::cerr << "[EntityMovement] Entity not found: " << entity_id << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Execute move
    std::cout << "[EntityMovement] Moving entity " << entity_id 
              << " from (" << current_x << ", " << current_y << ", " << current_z << ")"
              << " to (" << x << ", " << y << ", " << z << ")" << std::endl;
    
    if (sim->moveEntity(entity_id, x, y, z)) {
        std::cout << "[EntityMovement] Move successful" << std::endl;
        return BT::NodeStatus::SUCCESS;
    } else {
        std::cerr << "[EntityMovement] Move failed" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
}

bool getEntityPosition(simulation::SimControlInterface* sim,
                       const std::string& entity_id,
                       double& x, double& y, double& z) {
    if (!sim) {
        return false;
    }
    return sim->getEntityPosition(entity_id, x, y, z);
}

} // namespace behaviortree
