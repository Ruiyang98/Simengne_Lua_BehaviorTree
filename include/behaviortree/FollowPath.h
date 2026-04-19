#ifndef FOLLOW_PATH_H
#define FOLLOW_PATH_H

#include <behaviortree_cpp_v3/action_node.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <vector>
#include <tuple>

namespace behaviortree {

// Waypoint type definition: (x, y, z)
using Waypoint = std::tuple<double, double, double>;
using WaypointList = std::vector<Waypoint>;

// Move entity along a sequence of waypoints behavior tree node
class FollowPath : public BT::SyncActionNode {
public:
    FollowPath(const std::string& name, const BT::NodeConfiguration& config);
    
    // Define input ports
    static BT::PortsList providedPorts();
    
    // Execute path following
    BT::NodeStatus tick() override;
    
private:
    // Parse waypoints string (format: "x1,y1,z1;x2,y2,z2;...")
    WaypointList parseWaypoints(const std::string& waypoints_str);
};

} // namespace behaviortree

#endif // FOLLOW_PATH_H
