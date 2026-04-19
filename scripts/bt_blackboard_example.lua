-- Behavior Tree Blackboard Example
-- Demonstrates how to use blackboard to pass data between Lua and Behavior Tree

print("========================================")
print("    Blackboard Operations Example")
print("========================================")
print("")

-- Load behavior tree
print("1. Load behavior tree")
if not bt.load_file("bt_xml/path_movement.xml") then
    print("   ERROR: Failed to load behavior tree: " .. bt.get_last_error())
    return
end
print("   OK: Behavior tree loaded")
print("")

-- Create entity
print("2. Create test entity")
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Entity ID: " .. entity_id)
print("")

-- Test 1: Execute with parameters table
print("3. Execute with parameters via table")
local params = {
    waypoints = "0,0,0;5,0,0;5,5,0;0,5,0;0,0,0",
    delay_ms = 200
}

local tree_id = bt.execute("MainTree", entity_id, params)
if tree_id ~= "" then
    print("   OK: Tree executed with ID: " .. tree_id)
    print("   Status: " .. bt.get_status(tree_id))
else
    print("   ERROR: " .. bt.get_last_error())
end
print("")

-- Test 2: Set and get blackboard values
print("4. Test blackboard operations")

-- Create another entity and tree for blackboard testing
local entity2_id = sim.add_entity("player", 10.0, 10.0, 0.0)
local tree_id2 = bt.execute("MoveToSinglePoint", entity2_id, {
    target_x = 20.0,
    target_y = 20.0,
    target_z = 0.0
})

if tree_id2 ~= "" then
    print("   Created tree: " .. tree_id2)
    
    -- Get blackboard values
    print("   Reading blackboard values:")
    local entity_id_from_bb = bt.get_blackboard(tree_id2, "entity_id")
    if entity_id_from_bb then
        print("   - entity_id: " .. tostring(entity_id_from_bb))
    else
        print("   - entity_id: nil")
    end
    
    local target_x = bt.get_blackboard(tree_id2, "target_x")
    if target_x then
        print("   - target_x: " .. tostring(target_x))
    else
        print("   - target_x: nil")
    end
    
    -- Set new blackboard value
    print("   Setting custom_value = 42")
    bt.set_blackboard(tree_id2, "custom_value", 42)
    
    local custom = bt.get_blackboard(tree_id2, "custom_value")
    print("   - custom_value: " .. tostring(custom))
    
    -- Test different data types
    print("   Testing different data types:")
    bt.set_blackboard(tree_id2, "test_string", "hello from lua")
    bt.set_blackboard(tree_id2, "test_number", 3.14159)
    bt.set_blackboard(tree_id2, "test_bool", true)
    bt.set_blackboard(tree_id2, "test_int", 100)
    
    print("   - test_string: " .. tostring(bt.get_blackboard(tree_id2, "test_string")))
    print("   - test_number: " .. tostring(bt.get_blackboard(tree_id2, "test_number")))
    print("   - test_bool: " .. tostring(bt.get_blackboard(tree_id2, "test_bool")))
    print("   - test_int: " .. tostring(bt.get_blackboard(tree_id2, "test_int")))
else
    print("   ERROR: " .. bt.get_last_error())
end
print("")

-- Clean up
print("5. Clean up")
sim.remove_entity(entity_id)
sim.remove_entity(entity2_id)
print("   Entities removed")
print("")

print("========================================")
print("    Blackboard Example Complete")
print("========================================")
