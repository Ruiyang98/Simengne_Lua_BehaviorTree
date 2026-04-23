-- Behavior Tree Basic Examples
-- 基础行为树示例合集
-- 包含：行为树基础控制、黑板操作、实体控制

print("========================================")
print("    Behavior Tree Basic Examples")
print("========================================")
print("")

-- ============================================================================
-- Part 1: Entity Control Test (实体控制测试)
-- ============================================================================

local function run_entity_control_test()
    print("--- Part 1: Entity Control Test ---")
    print("")

    -- Test 1: Check initial entity count
    print("1.1 Check initial entity count")
    local count = sim.get_entity_count()
    print("   Initial entity count: " .. count)
    print("")

    -- Test 2: Add entities
    print("1.2 Add entities")
    local npc1_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    print("   Added NPC 1: " .. npc1_id)

    local npc2_id = sim.add_entity("npc", 10.0, 0.0, 0.0)
    print("   Added NPC 2: " .. npc2_id)

    local player_id = sim.add_entity("player", 5.0, 5.0, 0.0)
    print("   Added Player: " .. player_id)

    print("   Current entity count: " .. sim.get_entity_count())
    print("")

    -- Test 3: Get entity position
    print("1.3 Get entity positions")
    local pos1 = sim.get_entity_position(npc1_id)
    if pos1 then
        print("   NPC 1 position: (" .. pos1.x .. ", " .. pos1.y .. ", " .. pos1.z .. ")")
    else
        print("   ERROR: Failed to get NPC 1 position")
    end

    local pos2 = sim.get_entity_position(npc2_id)
    if pos2 then
        print("   NPC 2 position: (" .. pos2.x .. ", " .. pos2.y .. ", " .. pos2.z .. ")")
    else
        print("   ERROR: Failed to get NPC 2 position")
    end

    local player_pos = sim.get_entity_position(player_id)
    if player_pos then
        print("   Player position: (" .. player_pos.x .. ", " .. player_pos.y .. ", " .. player_pos.z .. ")")
    else
        print("   ERROR: Failed to get Player position")
    end
    print("")

    -- Test 4: Move entities
    print("1.4 Move entities")
    local success = sim.move_entity(npc1_id, 15.0, 20.0, 0.0)
    if success then
        print("   NPC 1 moved successfully")
        local new_pos = sim.get_entity_position(npc1_id)
        print("   New position: (" .. new_pos.x .. ", " .. new_pos.y .. ", " .. new_pos.z .. ")")
    else
        print("   ERROR: Failed to move NPC 1")
    end

    success = sim.move_entity(player_id, 25.0, 30.0, 5.0)
    if success then
        print("   Player moved successfully")
        local new_pos = sim.get_entity_position(player_id)
        print("   New position: (" .. new_pos.x .. ", " .. new_pos.y .. ", " .. new_pos.z .. ")")
    else
        print("   ERROR: Failed to move Player")
    end
    print("")

    -- Test 5: Get all entities
    print("1.5 Get all entities")
    local all_entities = sim.get_all_entities()
    print("   Total entities: " .. #all_entities)
    for i, entity in ipairs(all_entities) do
        print("   Entity " .. i .. ": " .. entity.id .. " (type: " .. entity.type .. ") at ("
              .. entity.x .. ", " .. entity.y .. ", " .. entity.z .. ")")
    end
    print("")

    -- Test 6: Remove entities
    print("1.6 Remove entities")
    success = sim.remove_entity(npc2_id)
    if success then
        print("   NPC 2 removed successfully")
    else
        print("   ERROR: Failed to remove NPC 2")
    end
    print("   Current entity count: " .. sim.get_entity_count())
    print("")

    -- Clean up
    sim.remove_entity(npc1_id)
    sim.remove_entity(player_id)
    print("   All test entities removed")
    print("")
end

-- ============================================================================
-- Part 2: Behavior Tree Control (行为树基础控制)
-- ============================================================================

local function run_bt_control_example()
    print("--- Part 2: Behavior Tree Control ---")
    print("")

    -- Test 1: Load behavior tree from file
    print("2.1 Load behavior tree from XML file")
    local success = bt.load_file("bt_xml/square_path.xml")
    if success then
        print("   OK: Behavior tree loaded successfully")
    else
        print("   ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return
    end
    print("")

    -- Test 2: Create an entity for the behavior tree
    print("2.2 Create test entity")
    local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    print("   Created entity: " .. entity_id)
    print("   Entity count: " .. sim.get_entity_count())
    print("")

    -- Test 3: Execute behavior tree
    print("2.3 Execute behavior tree")
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
    print("2.4 Execute with another entity")
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
    print("2.5 Check tree existence")
    print("   Has tree '" .. tree_id .. "': " .. tostring(bt.has_tree(tree_id)))
    print("   Has tree 'nonexistent': " .. tostring(bt.has_tree("nonexistent")))
    print("")

    -- Clean up entities
    print("2.6 Clean up entities")
    sim.remove_entity(entity_id)
    sim.remove_entity(entity2_id)
    print("   Entity count after cleanup: " .. sim.get_entity_count())
    print("")
end

-- ============================================================================
-- Part 3: Blackboard Operations (黑板操作)
-- ============================================================================

local function run_blackboard_example()
    print("--- Part 3: Blackboard Operations ---")
    print("")

    -- Load behavior tree
    print("3.1 Load behavior tree")
    if not bt.load_file("bt_xml/path_movement.xml") then
        print("   ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return
    end
    print("   OK: Behavior tree loaded")
    print("")

    -- Create entity
    print("3.2 Create test entity")
    local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
    print("   Entity ID: " .. entity_id)
    print("")

    -- Test 1: Execute with parameters table
    print("3.3 Execute with parameters via table")
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
    print("3.4 Test blackboard operations")

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
    print("3.5 Clean up")
    sim.remove_entity(entity_id)
    sim.remove_entity(entity2_id)
    print("   Entities removed")
    print("")
end

-- ============================================================================
-- Main Execution
-- ============================================================================

print("Starting Basic Examples...")
print("")

run_entity_control_test()
run_bt_control_example()
run_blackboard_example()

print("========================================")
print("    Basic Examples Complete")
print("========================================")
print("")
print("Summary:")
print("  - Entity control (add/move/remove/get)")
print("  - Behavior tree loading and execution")
print("  - Tree status checking")
print("  - Blackboard read/write operations")
