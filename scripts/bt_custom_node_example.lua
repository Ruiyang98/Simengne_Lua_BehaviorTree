-- Behavior Tree Custom Node Example
-- Demonstrates how to register Lua functions as behavior tree nodes

print("========================================")
print("    Custom Lua Node Example")
print("========================================")
print("")

-- Register a Lua action node
print("1. Register Lua action node")
bt.register_action("LuaCheckHealth", function()
    print("   [LuaCheckHealth] Executing...")
    
    -- Get entity count as a proxy for "health check"
    local count = sim.get_entity_count()
    print("   [LuaCheckHealth] Entity count: " .. count)
    
    if count > 0 then
        print("   [LuaCheckHealth] Health OK")
        return "SUCCESS"
    else
        print("   [LuaCheckHealth] No entities!")
        return "FAILURE"
    end
end)
print("   OK: LuaCheckHealth registered")
print("")

-- Register another Lua action node
print("2. Register Lua patrol action")
bt.register_action("LuaPatrol", function()
    print("   [LuaPatrol] Starting patrol...")
    
    -- Get all entities
    local entities = sim.get_all_entities()
    if #entities == 0 then
        print("   [LuaPatrol] No entities to patrol")
        return "FAILURE"
    end
    
    -- Move each entity in a small pattern
    for i, entity in ipairs(entities) do
        local new_x = entity.x + 1.0
        local new_y = entity.y + 1.0
        sim.move_entity(entity.id, new_x, new_y, entity.z)
        print("   [LuaPatrol] Moved " .. entity.id .. " to (" .. new_x .. ", " .. new_y .. ")")
    end
    
    print("   [LuaPatrol] Patrol complete")
    return "SUCCESS"
end)
print("   OK: LuaPatrol registered")
print("")

-- Register a Lua condition node
print("3. Register Lua condition node")
bt.register_condition("LuaHasEntities", function()
    local count = sim.get_entity_count()
    print("   [LuaHasEntities] Checking entities: " .. count)
    return count > 0
end)
print("   OK: LuaHasEntities registered")
print("")

-- Register another condition
print("4. Register Lua condition for specific entity type")
bt.register_condition("LuaHasNPC", function()
    local entities = sim.get_all_entities()
    for i, entity in ipairs(entities) do
        if entity.type == "npc" then
            print("   [LuaHasNPC] Found NPC: " .. entity.id)
            return true
        end
    end
    print("   [LuaHasNPC] No NPC found")
    return false
end)
print("   OK: LuaHasNPC registered")
print("")

-- Create test entities
print("5. Create test entities")
local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
local player_id = sim.add_entity("player", 10.0, 10.0, 0.0)
print("   Created NPC: " .. npc_id)
print("   Created Player: " .. player_id)
print("   Total entities: " .. sim.get_entity_count())
print("")

-- Test the Lua nodes by creating a behavior tree XML that uses them
print("6. Create and execute behavior tree with Lua nodes")
local xml_content = [[
<root main_tree_to_execute="LuaTestTree">
    <BehaviorTree ID="LuaTestTree">
        <Sequence name="lua_test_sequence">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <LuaAction lua_node_name="LuaCheckHealth" />
            <LuaCondition lua_node_name="LuaHasNPC" />
            <LuaAction lua_node_name="LuaPatrol" />
        </Sequence>
    </BehaviorTree>
</root>
]]

if bt.load_text(xml_content) then
    print("   OK: Lua test tree loaded")
    
    local tree_id = bt.execute("LuaTestTree")
    if tree_id ~= "" then
        print("   Tree executed with ID: " .. tree_id)
        print("   Final status: " .. bt.get_status(tree_id))
    else
        print("   ERROR: " .. bt.get_last_error())
    end
else
    print("   ERROR: " .. bt.get_last_error())
end
print("")

-- Test with no entities (should fail LuaHasEntities)
print("7. Test with no entities (should fail)")
sim.remove_entity(npc_id)
sim.remove_entity(player_id)
print("   All entities removed")

local xml_content2 = [[
<root main_tree_to_execute="LuaTestTree2">
    <BehaviorTree ID="LuaTestTree2">
        <Sequence name="lua_test_sequence2">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <LuaAction lua_node_name="LuaCheckHealth" />
        </Sequence>
    </BehaviorTree>
</root>
]]

if bt.load_text(xml_content2) then
    local tree_id2 = bt.execute("LuaTestTree2")
    if tree_id2 ~= "" then
        print("   Tree executed with ID: " .. tree_id2)
        print("   Final status (should be FAILURE): " .. bt.get_status(tree_id2))
    end
end
print("")

print("========================================")
print("    Custom Lua Node Example Complete")
print("========================================")
