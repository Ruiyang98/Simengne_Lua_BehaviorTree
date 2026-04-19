#ifndef ENTITY_MOVEMENT_H
#define ENTITY_MOVEMENT_H

#include <behaviortree_cpp_v3/behavior_tree.h>
#include <string>
#include "simulation/SimControlInterface.h"

namespace behaviortree {

// Move entity to a specific position
// This is the centralized movement logic used by both MoveToPoint and FollowPath
BT::NodeStatus moveEntityToPosition(simulation::SimControlInterface* sim,
                                    const std::string& entity_id, 
                                    double x, double y, double z);

// Get entity current position helper
bool getEntityPosition(simulation::SimControlInterface* sim,
                       const std::string& entity_id,
                       double& x, double& y, double& z);

} // namespace behaviortree

#endif // ENTITY_MOVEMENT_H
