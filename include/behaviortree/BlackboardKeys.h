#ifndef BLACKBOARD_KEYS_H
#define BLACKBOARD_KEYS_H

namespace behaviortree {

// Blackboard key name constants
namespace BlackboardKeys {
    // Entity ID
    constexpr const char* ENTITY_ID = "entity_id";
    
    // Target position
    constexpr const char* TARGET_X = "target_x";
    constexpr const char* TARGET_Y = "target_y";
    constexpr const char* TARGET_Z = "target_z";
    
    // Waypoints list (vector of tuples/points)
    constexpr const char* WAYPOINTS = "waypoints";
    
    // Movement speed
    constexpr const char* MOVE_SPEED = "move_speed";
    
    // Waypoint index
    constexpr const char* WAYPOINT_INDEX = "waypoint_index";
} // namespace BlackboardKeys

} // namespace behaviortree

#endif // BLACKBOARD_KEYS_H
