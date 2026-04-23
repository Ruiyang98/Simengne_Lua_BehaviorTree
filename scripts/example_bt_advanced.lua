-- Behavior Tree Advanced Examples
-- 高级行为树示例合集
-- 包含：高级综合示例、有状态动作、异步执行

print("========================================")
print("    Behavior Tree Advanced Examples")
print("========================================")
print("")

-- ============================================================================
-- Part 1: Stateful Action Example - 有状态动作节点
-- ============================================================================

local function run_stateful_action_example()
    print("--- Part 1: Stateful Action Example ---")
    print("")

    -- State management table (for saving movement state per entity)
    local moveStates = {}

    -- LuaStatefulMoveTo: Stateful move to target point
    print("1.1 Register LuaStatefulMoveTo")
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
    print("   [OK] LuaStatefulMoveTo registered")
    print("")

    -- LuaStatefulPatrol: Stateful patrol node
    print("1.2 Register LuaStatefulPatrol")
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
    print("   [OK] LuaStatefulPatrol registered")
    print("")

    -- LuaStatefulWait: Stateful wait node
    print("1.3 Register LuaStatefulWait")
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
    print("   [OK] LuaStatefulWait registered")
    print("")

    print("[Stateful Action Example] All stateful actions registered!")
    print("")
end

-- ============================================================================
-- Part 2: Async Behavior Tree Example - 异步行为树执行
-- ============================================================================

local function run_async_bt_example()
    print("--- Part 2: Async Behavior Tree Example ---")
    print("")

    -- Get simulation controller
    local sim = require("sim")

    -- Create test entity
    print("2.1 Creating test entity...")
    local entity_id = sim.add_entity("npc", 0, 0, 0)
    print("   Created entity: " .. entity_id)
    print("")

    -- Load behavior tree XML file
    print("2.2 Loading behavior tree XML...")
    if not bt.load_file("bt_xml/async_square_path.xml") then
        print("   ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return
    end
    print("   OK: Behavior tree loaded")
    print("")

    -- Set complete callback
    local function on_complete(tree_id, status)
        print("[Callback] Tree " .. tree_id .. " completed with status: " .. status)
    end

    -- Set tick callback
    local function on_tick(tree_id)
        -- Optional: get entity position per tick
        -- local x, y, z = sim.get_entity_position(entity_id)
        -- print("[Tick] Entity at (" .. x .. ", " .. y .. ")")
    end

    -- Method 1: Use execute_async to start async behavior tree
    print("2.3 Starting async behavior tree (AsyncSquarePath)...")
    local tree_id = bt.execute_async("AsyncSquarePath", entity_id, nil, 100)

    if tree_id == "" then
        print("   ERROR: Failed to start async behavior tree: " .. bt.get_last_error())
        return
    end

    print("   OK: Async behavior tree started with ID: " .. tree_id)
    print("")

    -- Set callbacks
    bt.set_complete_callback(tree_id, on_complete)
    bt.set_tick_callback(tree_id, on_tick)

    -- Monitor behavior tree status
    print("2.4 Monitoring behavior tree status...")
    print("   (Press Ctrl+C to stop)")
    print("")

    local max_wait_seconds = 5  -- Reduced for example
    local waited_seconds = 0

    while waited_seconds < max_wait_seconds do
        local status = bt.get_async_status(tree_id)
        print("   Tree status: " .. status)

        if status == "SUCCESS" or status == "FAILURE" then
            print("")
            print("   Behavior tree finished!")
            break
        end

        -- Check every second
        sim.sleep(1000)
        waited_seconds = waited_seconds + 1
    end

    if waited_seconds >= max_wait_seconds then
        print("")
        print("   Timeout! Stopping behavior tree...")
        bt.stop_async(tree_id)
    end

    -- Get final entity position
    local pos = sim.get_entity_position(entity_id)
    if pos then
        print("")
        print("   Final entity position: (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")
    end
    print("")

    -- Clean up
    sim.remove_entity(entity_id)
    print("   Entity removed")
    print("")
end

-- ============================================================================
-- Part 3: Advanced Comprehensive Example - 高级综合示例
-- ============================================================================

local function run_advanced_example()
    print("--- Part 3: Advanced Comprehensive Example ---")
    print("")

    -- Helper function to print section headers
    local function section(title)
        print("")
        print("--- " .. title .. " ---")
    end

    -- Helper function to create a patrol pattern
    local function create_patrol_waypoints(start_x, start_y, size)
        return string.format("%f,%f,0;%f,%f,0;%f,%f,0;%f,%f,0;%f,%f,0",
            start_x, start_y,
            start_x + size, start_y,
            start_x + size, start_y + size,
            start_x, start_y + size,
            start_x, start_y
        )
    end

    section("Setup: Load Behavior Trees")

    -- Load all available behavior trees
    local bt_files = {
        "bt_xml/path_movement.xml",
        "bt_xml/square_path.xml",
        "bt_xml/square_path_composite.xml",
        "bt_xml/waypoint_patrol.xml"
    }

    for _, file in ipairs(bt_files) do
        if bt.load_file(file) then
            print("Loaded: " .. file)
        else
            print("Failed to load: " .. file .. " - " .. bt.get_last_error())
        end
    end

    section("Setup: Register Custom Lua Nodes")

    -- Register a complex Lua action that uses simulation state
    bt.register_action("SmartPatrol", function()
        print("   [SmartPatrol] Analyzing patrol route...")

        local entities = sim.get_all_entities()
        if #entities == 0 then
            return "FAILURE"
        end

        -- Find the entity that needs patrolling
        local target_entity = nil
        for _, entity in ipairs(entities) do
            if entity.type == "guard" or entity.type == "npc" then
                target_entity = entity
                break
            end
        end

        if not target_entity then
            print("   [SmartPatrol] No suitable entity found")
            return "FAILURE"
        end

        print("   [SmartPatrol] Patrolling entity: " .. target_entity.id)

        -- Move entity in a diamond pattern
        local moves = {
            {x = 5, y = 0},
            {x = 0, y = 5},
            {x = -5, y = 0},
            {x = 0, y = -5}
        }

        for _, move in ipairs(moves) do
            local new_x = target_entity.x + move.x
            local new_y = target_entity.y + move.y
            sim.move_entity(target_entity.id, new_x, new_y, 0.0)
            print("   [SmartPatrol] Moved to (" .. new_x .. ", " .. new_y .. ")")
            sleep(0.1)
        end

        return "SUCCESS"
    end)

    -- Register a condition that checks if simulation is running
    bt.register_condition("SimulationActive", function()
        return sim.is_running()
    end)

    print("   Registered: SmartPatrol action")
    print("   Registered: SimulationActive condition")

    section("Scenario 1: Guard Patrol")

    -- Create a guard entity
    local guard_id = sim.add_entity("guard", 0.0, 0.0, 0.0)
    print("Created guard: " .. guard_id)

    -- Execute square path with the guard
    local tree1 = bt.execute("SquarePath", guard_id)
    if tree1 ~= "" then
        print("Guard patrol completed with status: " .. bt.get_status(tree1))
    end

    section("Scenario 2: Multi-Entity Coordination")

    -- Create multiple entities
    local npc1 = sim.add_entity("npc", 20.0, 20.0, 0.0)
    local npc2 = sim.add_entity("npc", 30.0, 30.0, 0.0)
    local player = sim.add_entity("player", 25.0, 25.0, 0.0)

    print("Created entities:")
    print("  - NPC 1: " .. npc1)
    print("  - NPC 2: " .. npc2)
    print("  - Player: " .. player)

    -- Execute different behavior trees for different entities
    local tree2 = bt.execute("LargeSquarePath", npc1)
    print("NPC 1 patrol status: " .. bt.get_status(tree2))

    -- Use custom waypoints for NPC2
    local tree3 = bt.execute("MainTree", npc2, {
        waypoints = create_patrol_waypoints(30.0, 30.0, 10.0),
        delay_ms = 100
    })
    print("NPC 2 patrol status: " .. bt.get_status(tree3))

    section("Scenario 3: Dynamic Blackboard Manipulation")

    -- Create a tree and manipulate its blackboard
    local tree4 = bt.execute("MoveToSinglePoint", player, {
        target_x = 50.0,
        target_y = 50.0,
        target_z = 0.0
    })

    if tree4 ~= "" then
        print("Initial execution complete")

        -- Read the target position from blackboard
        local tx = bt.get_blackboard(tree4, "target_x")
        local ty = bt.get_blackboard(tree4, "target_y")
        print("Target position from blackboard: (" .. tostring(tx) .. ", " .. tostring(ty) .. ")")

        -- Update the target position
        bt.set_blackboard(tree4, "target_x", 60.0)
        bt.set_blackboard(tree4, "target_y", 60.0)
        print("Updated target to (60.0, 60.0)")

        -- Verify the update
        local new_tx = bt.get_blackboard(tree4, "target_x")
        print("New target_x from blackboard: " .. tostring(new_tx))
    end

    section("Scenario 4: Tree Management")

    -- Show all active trees
    print("Checking active trees:")
    print("  Tree 1 exists: " .. tostring(bt.has_tree(tree1)))
    print("  Tree 2 exists: " .. tostring(bt.has_tree(tree2)))
    print("  Non-existent tree: " .. tostring(bt.has_tree("fake_tree")))

    -- Try to stop a tree
    print("\nAttempting to stop tree 1...")
    local stop_result = bt.stop(tree1)
    print("Stop result: " .. tostring(stop_result))

    section("Cleanup")

    -- Remove all entities
    local all_entities = sim.get_all_entities()
    print("Removing " .. #all_entities .. " entities...")
    for _, entity in ipairs(all_entities) do
        sim.remove_entity(entity.id)
    end
    print("Final entity count: " .. sim.get_entity_count())
    print("")
end

-- ============================================================================
-- Main Execution
-- ============================================================================

print("Starting Advanced Examples...")
print("")

run_stateful_action_example()
run_async_bt_example()
run_advanced_example()

print("========================================")
print("    Advanced Examples Complete")
print("========================================")
print("")
print("Summary:")
print("  - Stateful action nodes (onStart/onRunning/onHalted)")
print("  - Async behavior tree execution with callbacks")
print("  - Multi-entity coordination")
print("  - Dynamic blackboard manipulation")
print("  - Tree lifecycle management")
