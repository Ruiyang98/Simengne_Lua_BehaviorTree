-- Behavior Tree Lua Nodes Examples
-- Lua 节点示例合集
-- 包含：自定义节点（内联XML）、自定义节点（文件加载）、参数化节点

print("========================================")
print("    Behavior Tree Lua Nodes Examples")
print("========================================")
print("")

-- ============================================================================
-- Part 1: Custom Lua Nodes (Inline XML) - 内联XML方式
-- ============================================================================

local function run_custom_nodes_inline()
    print("--- Part 1: Custom Lua Nodes (Inline XML) ---")
    print("")

    -- Register a Lua action node
    print("1.1 Register Lua action node")
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
    print("1.2 Register Lua patrol action")
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
    print("1.3 Register Lua condition node")
    bt.register_condition("LuaHasEntities", function()
        local count = sim.get_entity_count()
        print("   [LuaHasEntities] Checking entities: " .. count)
        return count > 0
    end)
    print("   OK: LuaHasEntities registered")
    print("")

    -- Register another condition
    print("1.4 Register Lua condition for specific entity type")
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
    print("1.5 Create test entities")
    local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    local player_id = sim.add_entity("player", 10.0, 10.0, 0.0)
    print("   Created NPC: " .. npc_id)
    print("   Created Player: " .. player_id)
    print("   Total entities: " .. sim.get_entity_count())
    print("")

    -- Test the Lua nodes by creating a behavior tree XML that uses them
    print("1.6 Create and execute behavior tree with Lua nodes")
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
    print("1.7 Test with no entities (should fail)")
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
end

-- ============================================================================
-- Part 2: Custom Lua Nodes (From XML File) - 从文件加载XML
-- ============================================================================

local function run_custom_nodes_from_file()
    print("--- Part 2: Custom Lua Nodes (From XML File) ---")
    print("")

    -- Re-register nodes (they were already registered in Part 1, but let's be explicit)
    print("2.1 Register Lua nodes")

    bt.register_action("LuaCheckHealth", function()
        local count = sim.get_entity_count()
        print("   [LuaCheckHealth] Entities: " .. count)
        return count > 0 and "SUCCESS" or "FAILURE"
    end)

    bt.register_action("LuaPatrol", function()
        print("   [LuaPatrol] Patrolling...")
        local entities = sim.get_all_entities()
        if #entities == 0 then
            return "FAILURE"
        end
        for i, entity in ipairs(entities) do
            local new_x = entity.x + 1.0
            local new_y = entity.y + 1.0
            sim.move_entity(entity.id, new_x, new_y, entity.z)
        end
        return "SUCCESS"
    end)

    bt.register_condition("LuaHasEntities", function()
        return sim.get_entity_count() > 0
    end)

    bt.register_condition("LuaHasNPC", function()
        local entities = sim.get_all_entities()
        for i, entity in ipairs(entities) do
            if entity.type == "npc" then return true end
        end
        return false
    end)

    print("   OK: Lua nodes registered")
    print("")

    -- Create test entities
    print("2.2 Create test entities")
    local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    local player_id = sim.add_entity("player", 10.0, 10.0, 0.0)
    print("   Created NPC: " .. npc_id)
    print("   Created Player: " .. player_id)
    print("   Total entities: " .. sim.get_entity_count())
    print("")

    -- Test 1: Load from XML file and execute
    print("2.3 Load behavior tree from XML file and execute")
    if bt.load_file("bt_xml/lua_custom_nodes_example.xml") then
        print("   OK: XML file loaded successfully")

        -- Execute full behavior tree
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

    -- Test 2: Execute simplified version
    print("2.4 Execute simplified version")
    local tree_id2 = bt.execute("LuaTestTreeSimple")
    if tree_id2 ~= "" then
        print("   Simplified tree executed with ID: " .. tree_id2)
        print("   Final status: " .. bt.get_status(tree_id2))
    end
    print("")

    -- Test 3: Execute mixed tree (with entity)
    print("2.5 Execute mixed tree (with entity)")
    local tree_id3 = bt.execute("LuaMixedTree", npc_id)
    if tree_id3 ~= "" then
        print("   Mixed tree executed with ID: " .. tree_id3)
        print("   Final status: " .. bt.get_status(tree_id3))
    end
    print("")

    -- Clean up
    sim.remove_entity(npc_id)
    sim.remove_entity(player_id)
    print("   Entities removed")
    print("")
end

-- ============================================================================
-- Part 3: Lua Nodes with Parameters - 带参数的Lua节点
-- ============================================================================

local function run_lua_nodes_with_params()
    print("--- Part 3: Lua Nodes with Parameters ---")
    print("")

    -- Register a Lua action node with parameters
    print("3.1 Register LuaMoveTo action with parameters")
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
    print("3.2 Register LuaCheckDistance condition with parameters")
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
    print("3.3 Register LuaWait action with duration parameter")
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
    print("3.4 Register LuaPatrol action with patrol parameters")
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
    print("3.5 Create test entity")
    local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    print("   Created NPC: " .. npc_id)
    print("")

    -- Test with inline XML containing parameters
    print("3.6 Execute behavior tree with parameters")
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
    print("3.7 Test with XML file containing parameters")
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
    print("   Entity removed")
    print("")
end

-- ============================================================================
-- Main Execution
-- ============================================================================

print("Starting Lua Nodes Examples...")
print("")

run_custom_nodes_inline()
run_custom_nodes_from_file()
run_lua_nodes_with_params()

print("========================================")
print("    Lua Nodes Examples Complete")
print("========================================")
print("")
print("Summary:")
print("  - Lua nodes registered from script")
print("  - XML loaded from inline text and external file")
print("  - Multiple trees defined in one XML")
print("  - Lua nodes can be mixed with C++ nodes")
print("  - Parameters passed from XML to Lua functions")
