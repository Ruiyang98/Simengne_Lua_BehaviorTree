-- Behavior Tree Custom Node Example (从 XML 文件加载版本)
-- Demonstrates how to register Lua functions as behavior tree nodes

print("========================================")
print("    Custom Lua Node Example (File)")
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

-- Test 1: 从 XML 文件加载并执行行为树
print("6. Load behavior tree from XML file and execute")
if bt.load_file("bt_xml/lua_custom_nodes_example.xml") then
    print("   OK: XML file loaded successfully")

    -- 执行完整的行为树
    local tree_id = bt.execute("LuaTestTree")
    if tree_id ~= "" then
        print("   Tree executed with ID: " .. tree_id)
        print("   Final status: " .. bt.get_status(tree_id))
    else
        print("   ERROR: " .. bt.get_last_error())
    end
else
    print("   ERROR: Failed to load XML: " .. bt.get_last_error())
end
print("")

-- Test 2: 执行简化版行为树
print("7. Execute simplified version")
local tree_id2 = bt.execute("LuaTestTreeSimple")
if tree_id2 ~= "" then
    print("   Simplified tree executed with ID: " .. tree_id2)
    print("   Final status: " .. bt.get_status(tree_id2))
end
print("")

-- Test 3: 执行混合节点行为树（需要指定实体）
print("8. Execute mixed tree (with entity)")
local tree_id3 = bt.execute("LuaMixedTree", npc_id)
if tree_id3 ~= "" then
    print("   Mixed tree executed with ID: " .. tree_id3)
    print("   Final status: " .. bt.get_status(tree_id3))
end
print("")

-- Test 4: 移除所有实体后再次执行（应该失败）
print("9. Test with no entities (should fail)")
sim.remove_entity(npc_id)
sim.remove_entity(player_id)
print("   All entities removed")

local tree_id4 = bt.execute("LuaTestTree")
if tree_id4 ~= "" then
    print("   Tree executed with ID: " .. tree_id4)
    print("   Final status (should be FAILURE): " .. bt.get_status(tree_id4))
else
    print("   ERROR: " .. bt.get_last_error())
end
print("")

print("========================================")
print("    Custom Lua Node Example Complete")
print("========================================")
print("")
print("Summary:")
print("  - Lua nodes registered from script")
print("  - XML loaded from external file")
print("  - Multiple trees defined in one XML")
print("  - Lua nodes can be mixed with C++ nodes")
