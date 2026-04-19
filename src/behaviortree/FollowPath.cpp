#include "behaviortree/FollowPath.h"
#include "behaviortree/SimControllerPtr.h"
#include "behaviortree/EntityMovement.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace behaviortree {

FollowPath::FollowPath(const std::string& name, const BT::NodeConfiguration& config)
    : BT::SyncActionNode(name, config)
{
}

BT::PortsList FollowPath::providedPorts() {
    return {
        BT::InputPort<std::string>("entity_id", "Entity ID to move"),
        BT::InputPort<std::string>("waypoints", "Waypoints in format: x1,y1,z1;x2,y2,z2;..."),
        BT::InputPort<int>("delay_ms", 500, "Delay between waypoints in milliseconds")
    };
}

WaypointList FollowPath::parseWaypoints(const std::string& waypoints_str) {
    WaypointList waypoints;
    
    if (waypoints_str.empty()) {
        return waypoints;
    }
    
    std::stringstream ss(waypoints_str);
    std::string point_str;
    
    while (std::getline(ss, point_str, ';')) {
        std::stringstream point_ss(point_str);
        std::string coord_str;
        std::vector<double> coords;
        
        while (std::getline(point_ss, coord_str, ',')) {
            try {
                coords.push_back(std::stod(coord_str));
            } catch (...) {
                std::cerr << "[FollowPath] Failed to parse coordinate: " << coord_str << std::endl;
                break;
            }
        }
        
        if (coords.size() >= 2) {
            double x = coords[0];
            double y = coords[1];
            double z = (coords.size() >= 3) ? coords[2] : 0.0;
            waypoints.emplace_back(x, y, z);
        }
    }
    
    return waypoints;
}

BT::NodeStatus FollowPath::tick() {
    // Get input parameters
    BT::Optional<std::string> entity_id = getInput<std::string>("entity_id");
    BT::Optional<std::string> waypoints_str = getInput<std::string>("waypoints");
    BT::Optional<int> delay_ms = getInput<int>("delay_ms");
    
    // Check required parameters
    if (!entity_id) {
        std::cerr << "[FollowPath] Missing required input: entity_id" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    if (!waypoints_str) {
        std::cerr << "[FollowPath] Missing required input: waypoints" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Get simulation controller
    simulation::SimControlInterface* sim = getSimController();
    if (!sim) {
        std::cerr << "[FollowPath] SimController not available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Check if entity exists
    double current_x, current_y, current_z;
    if (!getEntityPosition(sim, entity_id.value(), current_x, current_y, current_z)) {
        std::cerr << "[FollowPath] Entity not found: " << entity_id.value() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // Parse waypoints
    WaypointList waypoints = parseWaypoints(waypoints_str.value());
    
    if (waypoints.empty()) {
        std::cout << "[FollowPath] No valid waypoints provided, nothing to do" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
    
    if (waypoints.size() < 2) {
        std::cout << "[FollowPath] Only one waypoint, moving directly to it" << std::endl;
    }
    
    std::cout << "[FollowPath] Following path with " << waypoints.size() << " waypoints" << std::endl;
    
    // Follow path by calling centralized move function for each waypoint
    int delay = delay_ms.value_or(500);
    for (size_t i = 0; i < waypoints.size(); ++i) {
        double x = std::get<0>(waypoints[i]);
        double y = std::get<1>(waypoints[i]);
        double z = std::get<2>(waypoints[i]);
        
        std::cout << "[FollowPath] Moving to waypoint " << (i + 1) << "/" << waypoints.size()
                  << " (" << x << ", " << y << ", " << z << ")" << std::endl;
        
        // Use centralized movement logic
        BT::NodeStatus status = moveEntityToPosition(sim, entity_id.value(), x, y, z);
        
        if (status != BT::NodeStatus::SUCCESS) {
            std::cerr << "[FollowPath] Failed to move to waypoint " << (i + 1) << std::endl;
            return BT::NodeStatus::FAILURE;
        }
        
        // Add delay between waypoints (except after the last point)
        if (i < waypoints.size() - 1 && delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
    
    std::cout << "[FollowPath] Path following completed successfully" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

} // namespace behaviortree
