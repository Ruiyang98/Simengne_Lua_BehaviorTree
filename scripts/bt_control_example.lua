-- Behavior Tree Control Example
-- Demonstrates basic behavior tree loading and execution from Lua

print("========================================")
print("    Behavior Tree Control Example")
print("========================================")
print("")

-- Test 1: Load behavior tree from file
print("1. Load behavior tree from XML file")
local success = bt.load_file("bt_xml/square_path.xml")
if success then
    print("   OK: Behavior tree loaded successfully")
else
    print("   ERROR: Failed to load behavior tree: " .. bt.get_last_error())
    return
end
print("")

-- Test 2: Create an entity for the behavior tree
print("2. Create test entity")
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Created entity: " .. entity_id)
print("   Entity count: " .. sim.get_entity_count())
print("")

-- Test 3: Execute behavior tree
print("3. Execute behavior tree")
local tree_id = bt.execute("SquarePath", entity_id)
if tree_id ~= "" then
    print("   OK: Behavior tree executed, tree ID: " .. tree_id)
    
    -- Check status
    local status = bt.get_status(tree_id)
    print("   Tree status: " .. status)
else
    print("   ERROR: Failed to execute behavior tree: " .. bt.get_last_error())
end
print("")

-- Test 4: Execute another behavior tree with different entity
print("4. Execute with another entity")
local entity2_id = sim.add_entity("player", 5.0, 5.0, 0.0)
print("   Created entity: " .. entity2_id)

local tree_id2 = bt.execute("LargeSquarePath", entity2_id)
if tree_id2 ~= "" then
    print("   OK: Behavior tree executed, tree ID: " .. tree_id2)
    local status = bt.get_status(tree_id2)
    print("   Tree status: " .. status)
else
    print("   ERROR: Failed to execute behavior tree: " .. bt.get_last_error())
end
print("")

-- Test 5: Check tree existence
print("5. Check tree existence")
print("   Has tree '" .. tree_id .. "': " .. tostring(bt.has_tree(tree_id)))
print("   Has tree 'nonexistent': " .. tostring(bt.has_tree("nonexistent")))
print("")

-- Test 6: Clean up entities
print("6. Clean up entities")
sim.remove_entity(entity_id)
sim.remove_entity(entity2_id)
print("   Entity count after cleanup: " .. sim.get_entity_count())
print("")

print("========================================")
print("    Behavior Tree Control Example Complete")
print("========================================")
