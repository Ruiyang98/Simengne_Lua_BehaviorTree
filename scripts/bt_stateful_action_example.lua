-- bt_stateful_action_example.lua
-- Demonstrates LuaStatefulAction for continuous movement using new sim interfaces
-- Uses set_entity_move_direction and get_entity_distance

print("[BT Stateful Action Example] Starting...")

-- ============================================
-- State management table (for saving movement state per entity)
-- ============================================
local moveStates = {}

-- ============================================
-- LuaStatefulMoveTo: Stateful move to target point
-- This node executes across multiple ticks until reaching target
-- Uses set_entity_move_direction instead of direct position setting
-- ============================================
bt.register_stateful_action("LuaStatefulMoveTo",
    -- onStart: Called when first entering the node
    function(params)
        local entity_id = params.entity_id or ""
        local target_x = tonumber(params.x) or 0
        local target_y = tonumber(params.y) or 0
        local target_z = tonumber(params.z) or 0
        local threshold = tonumber(params.threshold) or 0.5

        print(string.format("[LuaStatefulMoveTo:onStart] Entity '%s' starting move to (%.1f, %.1f, %.1f)",
                            entity_id, target_x, target_y, target_z))

        -- Get current position
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulMoveTo:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- Use get_entity_distance to check distance
        local dist = sim.get_entity_distance(entity_id, target_x, target_y, target_z)
        if dist <= threshold then
            print(string.format("[LuaStatefulMoveTo:onStart] Already at destination (distance: %.2f)", dist))
            return "SUCCESS"
        end

        -- Calculate direction vector
        local dx = target_x - pos.x
        local dy = target_y - pos.y
        local dz = target_z - pos.z

        -- Set movement direction
        if not sim.set_entity_move_direction(entity_id, dx, dy, dz) then
            print(string.format("[LuaStatefulMoveTo:onStart] ERROR: Failed to set move direction for '%s'", entity_id))
            return "FAILURE"
        end

        -- Save target position for onRunning
        moveStates[entity_id] = {
            targetX = target_x,
            targetY = target_y,
            targetZ = target_z,
            threshold = threshold
        }

        print(string.format("[LuaStatefulMoveTo:onStart] Distance to target: %.2f, movement started", dist))
        return "RUNNING"  -- Return RUNNING to continue execution
    end,

    -- onRunning: Called every tick while node is RUNNING
    function(params)
        local entity_id = params.entity_id or ""

        -- Get movement state
        local state = moveStates[entity_id]
        if not state then
            print(string.format("[LuaStatefulMoveTo:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        -- Use get_entity_distance to check if arrived
        local dist = sim.get_entity_distance(entity_id, state.targetX, state.targetY, state.targetZ)

        if dist <= state.threshold then
            print(string.format("[LuaStatefulMoveTo:onRunning] Entity '%s' arrived at destination (%.1f, %.1f, %.1f)",
                                entity_id, state.targetX, state.targetY, state.targetZ))
            
            -- Stop movement
            sim.set_entity_move_direction(entity_id, 0, 0, 0)
            
            -- Clean up state
            moveStates[entity_id] = nil
            return "SUCCESS"  -- Return SUCCESS to complete
        end

        -- Still moving, continue returning RUNNING
        return "RUNNING"
    end,

    -- onHalted: Called when node is halted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulMoveTo:onHalted] Movement halted for entity '%s'", entity_id))
        
        -- Stop movement
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        
        -- Clean up state
        moveStates[entity_id] = nil
    end
)
print("  [OK] LuaStatefulMoveTo registered")

-- ============================================
-- LuaStatefulPatrol: Stateful patrol node
-- Continuously moves between multiple waypoints
-- ============================================
bt.register_stateful_action("LuaStatefulPatrol",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""
        local waypoints_str = params.waypoints or "0,0;5,0;5,5;0,5"

        print(string.format("[LuaStatefulPatrol:onStart] Entity '%s' starting patrol", entity_id))

        -- Parse waypoints
        local waypoints = {}
        for x, y in string.gmatch(waypoints_str, "([%d%.%-]+),([%d%.%-]+)") do
            table.insert(waypoints, {x = tonumber(x), y = tonumber(y), z = 0})
        end

        if #waypoints == 0 then
            print("[LuaStatefulPatrol:onStart] ERROR: No valid waypoints")
            return "FAILURE"
        end

        -- Get current position
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulPatrol:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- Find nearest waypoint as starting point
        local currentWaypoint = 1
        local minDist = math.huge
        for i, wp in ipairs(waypoints) do
            local dist = math.sqrt((pos.x - wp.x)^2 + (pos.y - wp.y)^2)
            if dist < minDist then
                minDist = dist
                currentWaypoint = i
            end
        end

        -- Initialize patrol state
        local stateKey = entity_id .. "_patrol"
        moveStates[stateKey] = {
            waypoints = waypoints,
            currentIndex = currentWaypoint,
            cycles = tonumber(params.cycles) or 1,
            currentCycle = 1,
            threshold = 0.5
        }

        -- Set initial direction to first waypoint
        local targetWp = waypoints[currentWaypoint]
        local dx = targetWp.x - pos.x
        local dy = targetWp.y - pos.y
        local dz = targetWp.z - pos.z
        sim.set_entity_move_direction(entity_id, dx, dy, dz)

        print(string.format("[LuaStatefulPatrol:onStart] Starting at waypoint %d/%d, cycles: %d",
                            currentWaypoint, #waypoints, moveStates[stateKey].cycles))
        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_patrol"
        local state = moveStates[stateKey]

        if not state then
            print(string.format("[LuaStatefulPatrol:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        -- Get current target waypoint
        local targetWp = state.waypoints[state.currentIndex]
        
        -- Use get_entity_distance to check if reached current waypoint
        local dist = sim.get_entity_distance(entity_id, targetWp.x, targetWp.y, targetWp.z)

        if dist <= state.threshold then
            print(string.format("[LuaStatefulPatrol:onRunning] Reached waypoint %d/%d",
                                state.currentIndex, #state.waypoints))

            -- Move to next waypoint
            state.currentIndex = state.currentIndex + 1

            -- Check if completed all waypoints
            if state.currentIndex > #state.waypoints then
                state.currentCycle = state.currentCycle + 1

                if state.currentCycle > state.cycles then
                    print(string.format("[LuaStatefulPatrol:onRunning] Patrol completed after %d cycles", state.cycles))
                    
                    -- Stop movement
                    sim.set_entity_move_direction(entity_id, 0, 0, 0)
                    
                    moveStates[stateKey] = nil
                    return "SUCCESS"
                end

                -- Start next cycle
                state.currentIndex = 1
                print(string.format("[LuaStatefulPatrol:onRunning] Starting cycle %d/%d",
                                    state.currentCycle, state.cycles))
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

    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulPatrol:onHalted] Patrol halted for entity '%s'", entity_id))
        
        -- Stop movement
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        
        moveStates[entity_id .. "_patrol"] = nil
    end
)
print("  [OK] LuaStatefulPatrol registered")

-- ============================================
-- LuaStatefulWait: Stateful wait node
-- Waits for specified time (requires multiple ticks)
-- ============================================
local waitStates = {}

bt.register_stateful_action("LuaStatefulWait",
    -- onStart
    function(params)
        local duration = tonumber(params.duration) or 1.0
        local entity_id = params.entity_id or "default"

        print(string.format("[LuaStatefulWait:onStart] Starting wait for %.1f seconds", duration))

        waitStates[entity_id] = {
            startTime = os.time(),
            duration = duration
        }

        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or "default"
        local state = waitStates[entity_id]

        if not state then
            return "FAILURE"
        end

        local elapsed = os.time() - state.startTime
        if elapsed >= state.duration then
            print(string.format("[LuaStatefulWait:onRunning] Wait completed after %d seconds", elapsed))
            waitStates[entity_id] = nil
            return "SUCCESS"
        end

        print(string.format("[LuaStatefulWait:onRunning] Waiting... %d/%d seconds", elapsed, state.duration))
        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or "default"
        print(string.format("[LuaStatefulWait:onHalted] Wait interrupted for '%s'", entity_id))
        waitStates[entity_id] = nil
    end
)
print("  [OK] LuaStatefulWait registered")

print("[BT Stateful Action Example] All stateful actions registered!")
print("")
print("Usage:")
print("  1. Load script: bt.load_registry('scripts/bt_stateful_action_example.lua')")
print("  2. Load XML: bt.load_file('bt_xml/lua_stateful_nodes.xml')")
print("  3. Execute: bt.execute_async('LuaStatefulMoveExample', 'entity_001')")
print("")
