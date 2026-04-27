-- obstacle_avoidance_nodes.lua
-- Obstacle avoidance behavior tree nodes
-- Provides: HasObstacle, HasUserPath, AvoidObstacle, FollowUserPath, Idle

print("[ObstacleAvoidance] Registering obstacle avoidance nodes...")

-- State management tables
local avoidStates = {}
local pathStates = {}

-- ============================================
-- 1. HasObstacle - Check if there is an obstacle ahead
-- ============================================
bt.register_condition("HasObstacle", function(params)
    local entity_id = params.entity_id or ""
    local check_distance = tonumber(params.check_distance) or 5.0
    local check_angle = tonumber(params.check_angle) or 90.0  -- Forward angle in degrees

    -- Get entity position
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        print(string.format("[HasObstacle] Entity '%s' not found", entity_id))
        return false
    end

    -- Get all entities to check for obstacles
    local entities = sim.get_all_entities()
    
    for _, entity in ipairs(entities) do
        -- Skip self
        if tostring(entity.id) ~= tostring(entity_id) then
            -- Calculate distance
            local dx = entity.x - pos.x
            local dy = entity.y - pos.y
            local dist = math.sqrt(dx * dx + dy * dy)
            
            -- Check if within detection range
            if dist <= check_distance then
                -- Save obstacle info to blackboard
                bt.set_blackboard(entity_id, "obstacle_id", tostring(entity.id))
                bt.set_blackboard(entity_id, "obstacle_position", {x = entity.x, y = entity.y, z = entity.z or 0})
                bt.set_blackboard(entity_id, "obstacle_distance", dist)
                
                -- Calculate obstacle direction relative to entity
                local obstacle_angle = math.atan2(dy, dx) * 180 / math.pi
                bt.set_blackboard(entity_id, "obstacle_angle", obstacle_angle)
                
                print(string.format("[HasObstacle] Entity '%s' detected obstacle '%s' at distance %.2f, angle %.1f",
                                    entity_id, entity.id, dist, obstacle_angle))
                return true
            end
        end
    end

    return false
end)
print("  [OK] HasObstacle registered")

-- ============================================
-- 2. HasUserPath - Check if user path exists
-- ============================================
bt.register_condition("HasUserPath", function(params)
    local entity_id = params.entity_id or ""
    
    -- Check blackboard for user_path
    local path = bt.get_blackboard(entity_id, "user_path")
    local has_path = path ~= nil and path ~= ""
    
    if has_path then
        print(string.format("[HasUserPath] Entity '%s' has user path: %s", entity_id, path))
    else
        print(string.format("[HasUserPath] Entity '%s' has no user path", entity_id))
    end
    
    return has_path
end)
print("  [OK] HasUserPath registered")

-- ============================================
-- 3. AvoidObstacle - Move 90 degrees to avoid obstacle
-- ============================================
bt.register_stateful_action("AvoidObstacle",
    -- onStart: Calculate avoidance direction and start moving
    function(params)
        local entity_id = params.entity_id or ""
        local avoid_distance = tonumber(params.avoid_distance) or 3.0
        
        -- Get obstacle info from blackboard
        local obstacle_pos = bt.get_blackboard(entity_id, "obstacle_position")
        local obstacle_angle = bt.get_blackboard(entity_id, "obstacle_angle")
        
        if not obstacle_pos then
            print(string.format("[AvoidObstacle:onStart] ERROR: No obstacle info for entity '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Get current position
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[AvoidObstacle:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end
        
        -- Calculate direction from entity to obstacle
        local dx = obstacle_pos.x - pos.x
        local dy = obstacle_pos.y - pos.y
        local dist_to_obstacle = math.sqrt(dx * dx + dy * dy)
        
        if dist_to_obstacle == 0 then
            -- If exactly at same position, move in random direction
            dx = 1
            dy = 0
            dist_to_obstacle = 1
        end
        
        -- Calculate 90-degree avoidance direction (perpendicular)
        -- Two options: clockwise or counter-clockwise, choose one
        -- Option 1: Rotate 90 degrees clockwise
        local avoid_dx = dy / dist_to_obstacle
        local avoid_dy = -dx / dist_to_obstacle
        
        -- Calculate target position
        local target_x = pos.x + avoid_dx * avoid_distance
        local target_y = pos.y + avoid_dy * avoid_distance
        local target_z = pos.z
        
        print(string.format("[AvoidObstacle:onStart] Entity '%s' avoiding to (%.1f, %.1f), distance %.1f",
                            entity_id, target_x, target_y, avoid_distance))
        
        -- Set movement direction
        if not sim.set_entity_move_direction(entity_id, avoid_dx, avoid_dy, 0) then
            print(string.format("[AvoidObstacle:onStart] ERROR: Failed to set move direction for '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Save avoidance state
        avoidStates[entity_id] = {
            startX = pos.x,
            startY = pos.y,
            startZ = pos.z,
            targetX = target_x,
            targetY = target_y,
            targetZ = target_z,
            avoidDistance = avoid_distance,
            threshold = 0.5
        }
        
        return "RUNNING"
    end,
    
    -- onRunning: Check if avoidance distance reached
    function(params)
        local entity_id = params.entity_id or ""
        local state = avoidStates[entity_id]
        
        if not state then
            print(string.format("[AvoidObstacle:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Check distance traveled
        local dist = sim.get_entity_distance(entity_id, state.targetX, state.targetY, state.targetZ)
        
        if dist <= state.threshold then
            print(string.format("[AvoidObstacle:onRunning] Entity '%s' completed avoidance maneuver", entity_id))
            
            -- Stop movement
            sim.set_entity_move_direction(entity_id, 0, 0, 0)
            
            -- Clear obstacle info from blackboard
            bt.set_blackboard(entity_id, "obstacle_id", nil)
            bt.set_blackboard(entity_id, "obstacle_position", nil)
            bt.set_blackboard(entity_id, "obstacle_distance", nil)
            bt.set_blackboard(entity_id, "obstacle_angle", nil)
            
            -- Clear state
            avoidStates[entity_id] = nil
            
            return "SUCCESS"
        end
        
        -- Still avoiding
        return "RUNNING"
    end,
    
    -- onHalted: Clean up when interrupted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[AvoidObstacle:onHalted] Avoidance halted for entity '%s'", entity_id))
        
        -- Stop movement
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        
        -- Clear state
        avoidStates[entity_id] = nil
    end
)
print("  [OK] AvoidObstacle registered")

-- ============================================
-- 4. FollowUserPath - Follow user-defined path
-- ============================================
bt.register_stateful_action("FollowUserPath",
    -- onStart: Parse path and start following
    function(params)
        local entity_id = params.entity_id or ""
        
        -- Get user path from blackboard
        local path_str = bt.get_blackboard(entity_id, "user_path")
        if not path_str or path_str == "" then
            print(string.format("[FollowUserPath:onStart] ERROR: No user path for entity '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Parse waypoints from string format: "x1,y1;x2,y2;x3,y3"
        local waypoints = {}
        for x, y in string.gmatch(path_str, "([%d%.%-]+),([%d%.%-]+)") do
            table.insert(waypoints, {x = tonumber(x), y = tonumber(y), z = 0})
        end
        
        if #waypoints == 0 then
            print(string.format("[FollowUserPath:onStart] ERROR: Invalid path format for entity '%s'", entity_id))
            return "FAILURE"
        end
        
        print(string.format("[FollowUserPath:onStart] Entity '%s' following path with %d waypoints",
                            entity_id, #waypoints))
        
        -- Get current position
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[FollowUserPath:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end
        
        -- Find nearest waypoint as starting point
        local currentIndex = 1
        local minDist = math.huge
        for i, wp in ipairs(waypoints) do
            local dist = math.sqrt((pos.x - wp.x)^2 + (pos.y - wp.y)^2)
            if dist < minDist then
                minDist = dist
                currentIndex = i
            end
        end
        
        -- If already at first waypoint, move to next
        if minDist < 0.5 and currentIndex < #waypoints then
            currentIndex = currentIndex + 1
        end
        
        -- Set initial direction
        local targetWp = waypoints[currentIndex]
        local dx = targetWp.x - pos.x
        local dy = targetWp.y - pos.y
        local dz = targetWp.z - pos.z
        
        if not sim.set_entity_move_direction(entity_id, dx, dy, dz) then
            print(string.format("[FollowUserPath:onStart] ERROR: Failed to set move direction for '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Save path state
        pathStates[entity_id] = {
            waypoints = waypoints,
            currentIndex = currentIndex,
            threshold = 0.5
        }
        
        print(string.format("[FollowUserPath:onStart] Starting at waypoint %d/%d: (%.1f, %.1f)",
                            currentIndex, #waypoints, targetWp.x, targetWp.y))
        
        return "RUNNING"
    end,
    
    -- onRunning: Follow waypoints
    function(params)
        local entity_id = params.entity_id or ""
        local state = pathStates[entity_id]
        
        if not state then
            print(string.format("[FollowUserPath:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end
        
        -- Get current target waypoint
        local targetWp = state.waypoints[state.currentIndex]
        
        -- Check if reached current waypoint
        local dist = sim.get_entity_distance(entity_id, targetWp.x, targetWp.y, targetWp.z)
        
        if dist <= state.threshold then
            print(string.format("[FollowUserPath:onRunning] Entity '%s' reached waypoint %d/%d",
                                entity_id, state.currentIndex, #state.waypoints))
            
            -- Move to next waypoint
            state.currentIndex = state.currentIndex + 1
            
            -- Check if all waypoints completed
            if state.currentIndex > #state.waypoints then
                print(string.format("[FollowUserPath:onRunning] Entity '%s' completed user path", entity_id))
                
                -- Stop movement
                sim.set_entity_move_direction(entity_id, 0, 0, 0)
                
                -- Clear user path from blackboard
                bt.set_blackboard(entity_id, "user_path", nil)
                
                -- Clear state
                pathStates[entity_id] = nil
                
                return "SUCCESS"
            end
            
            -- Update direction to next waypoint
            targetWp = state.waypoints[state.currentIndex]
            local pos = sim.get_entity_position(entity_id)
            if pos then
                local dx = targetWp.x - pos.x
                local dy = targetWp.y - pos.y
                local dz = targetWp.z - pos.z
                sim.set_entity_move_direction(entity_id, dx, dy, dz)
            end
        end
        
        return "RUNNING"
    end,
    
    -- onHalted: Clean up when interrupted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[FollowUserPath:onHalted] Path following halted for entity '%s'", entity_id))
        
        -- Stop movement
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        
        -- Clear state
        pathStates[entity_id] = nil
    end
)
print("  [OK] FollowUserPath registered")

-- ============================================
-- 5. Idle - Do nothing
-- ============================================
bt.register_action("Idle", function(params)
    local entity_id = params.entity_id or ""
    -- Ensure entity is not moving
    sim.set_entity_move_direction(entity_id, 0, 0, 0)
    print(string.format("[Idle] Entity '%s' is idle", entity_id))
    return "SUCCESS"
end)
print("  [OK] Idle registered")

print("[ObstacleAvoidance] All obstacle avoidance nodes registered successfully!")
print("")
