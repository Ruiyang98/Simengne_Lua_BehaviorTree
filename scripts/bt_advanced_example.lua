-- Behavior Tree Advanced Example
-- Comprehensive demonstration of Lua and Behavior Tree integration

print("========================================")
print("    Advanced Behavior Tree Example")
print("========================================")
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

section("Scenario 4: Complex Lua-driven Behavior Tree")

-- Create a behavior tree XML that combines C++ and Lua nodes
local complex_xml = [[
<root main_tree_to_execute="ComplexMission">
    <BehaviorTree ID="ComplexMission">
        <Sequence name="mission_sequence">
            <!-- Check if simulation is active -->
            <LuaCondition lua_node_name="SimulationActive" />

            <!-- Check if target entity exists -->
            <CheckEntityExists entity_id="{entity_id}" />

            <!-- Execute smart patrol -->
            <LuaAction lua_node_name="SmartPatrol" />

            <!-- Select a target from list -->
            <SelectTargetFromList targets="target1,target2,target3"
                                  strategy="random"
                                  selected_target="{selected_target}" />
        </Sequence>
    </BehaviorTree>
</root>
]]

if bt.load_text(complex_xml) then
    print("Loaded complex mission tree")
    
    -- Create a special agent for this mission
    local agent = sim.add_entity("agent", 100.0, 100.0, 0.0)
    print("Created mission agent: " .. agent)
    
    -- Execute with parameters
    local mission_tree = bt.execute("ComplexMission", agent, {
        waypoints = create_patrol_waypoints(100.0, 100.0, 20.0),
        delay_ms = 50
    })
    
    if mission_tree ~= "" then
        print("Mission completed with status: " .. bt.get_status(mission_tree))
    else
        print("Mission failed: " .. bt.get_last_error())
    end
else
    print("Failed to load complex tree: " .. bt.get_last_error())
end

section("Scenario 5: Tree Management")

-- Show all active trees
print("Checking active trees:")
print("  Tree 1 exists: " .. tostring(bt.has_tree(tree1)))
print("  Tree 2 exists: " .. tostring(bt.has_tree(tree2)))
print("  Non-existent tree: " .. tostring(bt.has_tree("fake_tree")))

-- Try to stop a tree (mostly for async trees, but demonstrates the API)
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

section("Summary")

print("Demonstrated features:")
print("  ✓ Loading multiple behavior trees from XML files")
print("  ✓ Executing behavior trees with different entities")
print("  ✓ Passing parameters via blackboard")
print("  ✓ Reading and writing blackboard values from Lua")
print("  ✓ Registering custom Lua action nodes")
print("  ✓ Registering custom Lua condition nodes")
print("  ✓ Combining C++ and Lua nodes in one tree")
print("  ✓ Tree lifecycle management (status, stop, existence check)")

print("")
print("========================================")
print("    Advanced Example Complete")
print("========================================")
