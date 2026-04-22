-- Behavior Tree Lua Node with Parameters Example
-- Demonstrates how to use Lua nodes with custom parameters

print("========================================")
print("    Lua Node with Parameters Example")
print("========================================")
print("")

-- Register a Lua action node with parameters
print("1. Register LuaMoveTo action with parameters")
bt.register_action("LuaMoveTo", function(params)
    -- params is a table containing all XML attributes (except lua_node_name)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local speed = tonumber(params.speed) or 1.0

    print(string.format("   [LuaMoveTo] Moving entity '%s' to (%.1f, %.1f) at speed %.1f",
                        entity_id, target_x, target_y, speed))

    -- Execute the move
    if entity_id ~= "" then
        sim.move_entity(entity_id, target_x, target_y, 0)
        print("   [LuaMoveTo] Move executed successfully")
        return "SUCCESS"
    else
        print("   [LuaMoveTo] ERROR: No entity_id provided")
        return "FAILURE"
    end
end)
print("   OK: LuaMoveTo registered")
print("")

-- Register a Lua condition node with parameters
print("2. Register LuaCheckDistance condition with parameters")
bt.register_condition("LuaCheckDistance", function(params)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local max_distance = tonumber(params.max_distance) or 10.0

    print(string.format("   [LuaCheckDistance] Checking if '%s' is within %.1f units of (%.1f, %.1f)",
                        entity_id, max_distance, target_x, target_y))

    if entity_id == "" then
        print("   [LuaCheckDistance] ERROR: No entity_id provided")
        return false
    end

    local pos = sim.get_entity_position(entity_id)
    if not pos then
        print("   [LuaCheckDistance] Entity not found: " .. entity_id)
        return false
    end

    local dx = pos.x - target_x
    local dy = pos.y - target_y
    local distance = math.sqrt(dx * dx + dy * dy)

    local is_within_range = distance <= max_distance
    print(string.format("   [LuaCheckDistance] Distance: %.2f, Within range: %s",
                        distance, tostring(is_within_range)))

    return is_within_range
end)
print("   OK: LuaCheckDistance registered")
print("")

-- Register another action with different parameters
print("3. Register LuaWait action with duration parameter")
bt.register_action("LuaWait", function(params)
    local duration = tonumber(params.duration) or 1.0

    print(string.format("   [LuaWait] Waiting for %.1f seconds...", duration))

    -- In a real implementation, you might use a timer or coroutine
    -- For this example, we'll just print and return success
    print("   [LuaWait] Wait complete")
    return "SUCCESS"
end)
print("   OK: LuaWait registered")
print("")

-- Register a parameterized patrol action
print("4. Register LuaPatrol action with patrol parameters")
bt.register_action("LuaPatrol", function(params)
    local entity_id = params.entity_id or ""
    local patrol_radius = tonumber(params.patrol_radius) or 5.0
    local num_points = tonumber(params.num_points) or 4

    print(string.format("   [LuaPatrol] Entity '%s' patrolling with radius %.1f, %d points",
                        entity_id, patrol_radius, num_points))

    if entity_id == "" then
        return "FAILURE"
    end

    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return "FAILURE"
    end

    -- Simple patrol logic: move in a circle
    for i = 1, num_points do
        local angle = (2 * math.pi * (i - 1)) / num_points
        local new_x = pos.x + patrol_radius * math.cos(angle)
        local new_y = pos.y + patrol_radius * math.sin(angle)
        sim.move_entity(entity_id, new_x, new_y, pos.z)
        print(string.format("   [LuaPatrol] Moved to point %d: (%.1f, %.1f)", i, new_x, new_y))
    end

    return "SUCCESS"
end)
print("   OK: LuaPatrol registered")
print("")

-- Create test entity
print("5. Create test entity")
local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Created NPC: " .. npc_id)
print("")

-- Test with inline XML containing parameters
print("6. Execute behavior tree with parameters")
local xml_content = [[
<root main_tree_to_execute="ParamTestTree">
    <BehaviorTree ID="ParamTestTree">
        <Sequence name="param_sequence">
            <!-- Move to position (10, 10) with speed 2.0 -->
            <LuaAction lua_node_name="LuaMoveTo"
                       entity_id="]] .. npc_id .. [["
                       target_x="10"
                       target_y="10"
                       speed="2.0"/>

            <!-- Check if we're close to target -->
            <LuaCondition lua_node_name="LuaCheckDistance"
                          entity_id="]] .. npc_id .. [["
                          target_x="10"
                          target_y="10"
                          max_distance="1.0"/>

            <!-- Wait for 0.5 seconds -->
            <LuaAction lua_node_name="LuaWait"
                       duration="0.5"/>

            <!-- Patrol around current position -->
            <LuaAction lua_node_name="LuaPatrol"
                       entity_id="]] .. npc_id .. [["
                       patrol_radius="3.0"
                       num_points="4"/>
        </Sequence>
    </BehaviorTree>
</root>
]]

if bt.load_text(xml_content) then
    print("   OK: Behavior tree loaded")

    local tree_id = bt.execute("ParamTestTree")
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

-- Test with separate XML file
print("7. Test with XML file containing parameters")
if bt.load_file("bt_xml/lua_nodes_with_params.xml") then
    print("   OK: XML file loaded")

    local tree_id2 = bt.execute("LuaParamsTree", npc_id)
    if tree_id2 ~= "" then
        print("   Tree executed with ID: " .. tree_id2)
        print("   Final status: " .. bt.get_status(tree_id2))
    else
        print("   ERROR: " .. bt.get_last_error())
    end
else
    print("   ERROR: " .. bt.get_last_error())
end
print("")

-- Cleanup
sim.remove_entity(npc_id)

print("========================================")
print("    Lua Node with Parameters Complete")
print("========================================")
