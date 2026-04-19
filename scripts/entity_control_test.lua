-- Entity Control Test Script
-- Demonstrates how to use Lua API to control simulation entities

print("========================================")
print("       Entity Control Test")
print("========================================")
print("")

-- Test 1: Check initial entity count
print("1. Check initial entity count")
local count = sim.get_entity_count()
print("   Initial entity count: " .. count)
print("")

-- Test 2: Add entities
print("2. Add entities")
local npc1_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Added NPC 1: " .. npc1_id)

local npc2_id = sim.add_entity("npc", 10.0, 0.0, 0.0)
print("   Added NPC 2: " .. npc2_id)

local player_id = sim.add_entity("player", 5.0, 5.0, 0.0)
print("   Added Player: " .. player_id)

print("   Current entity count: " .. sim.get_entity_count())
print("")

-- Test 3: Get entity position
print("3. Get entity positions")
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
print("4. Move entities")
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

-- Test 5: Try to move non-existent entity
print("5. Test move non-existent entity")
success = sim.move_entity("nonexistent_entity", 0.0, 0.0, 0.0)
if success then
    print("   ERROR: Should have failed for non-existent entity")
else
    print("   OK: Correctly returned false for non-existent entity")
end
print("")

-- Test 6: Get all entities
print("6. Get all entities")
local all_entities = sim.get_all_entities()
print("   Total entities: " .. #all_entities)
for i, entity in ipairs(all_entities) do
    print("   Entity " .. i .. ": " .. entity.id .. " (type: " .. entity.type .. ") at (" 
          .. entity.x .. ", " .. entity.y .. ", " .. entity.z .. ")")
end
print("")

-- Test 7: Remove an entity
print("7. Remove entity")
success = sim.remove_entity(npc2_id)
if success then
    print("   NPC 2 removed successfully")
else
    print("   ERROR: Failed to remove NPC 2")
end
print("   Current entity count: " .. sim.get_entity_count())
print("")

-- Test 8: Verify entity is removed
print("8. Verify entity removal")
local removed_pos = sim.get_entity_position(npc2_id)
if removed_pos then
    print("   ERROR: Position found for removed entity")
else
    print("   OK: Position not found for removed entity (as expected)")
end

success = sim.remove_entity("nonexistent_entity")
if success then
    print("   ERROR: Should have failed for non-existent entity")
else
    print("   OK: Correctly returned false for non-existent entity")
end
print("")

-- Test 9: Get all entities after removal
print("9. Get all entities after removal")
all_entities = sim.get_all_entities()
print("   Total entities: " .. #all_entities)
for i, entity in ipairs(all_entities) do
    print("   Entity " .. i .. ": " .. entity.id .. " (type: " .. entity.type .. ")")
end
print("")

-- Test 10: Remove remaining entities
print("10. Remove remaining entities")
success = sim.remove_entity(npc1_id)
if success then
    print("   NPC 1 removed successfully")
else
    print("   ERROR: Failed to remove NPC 1")
end

success = sim.remove_entity(player_id)
if success then
    print("   Player removed successfully")
else
    print("   ERROR: Failed to remove Player")
end

print("   Final entity count: " .. sim.get_entity_count())
print("")

-- Test 11: Complex movement test
print("11. Complex movement test")
local test_entity = sim.add_entity("test", 0.0, 0.0, 0.0)
print("   Created test entity: " .. test_entity)

-- Move entity in a pattern
print("   Moving entity in a square pattern...")
local path = {
    {x = 10.0, y = 0.0, z = 0.0},
    {x = 10.0, y = 10.0, z = 0.0},
    {x = 0.0, y = 10.0, z = 0.0},
    {x = 0.0, y = 0.0, z = 0.0}
}

for i, point in ipairs(path) do
    sim.move_entity(test_entity, point.x, point.y, point.z)
    local pos = sim.get_entity_position(test_entity)
    print("   Step " .. i .. ": (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")
    sleep(0.1)
end

sim.remove_entity(test_entity)
print("   Test entity removed")
print("")

print("========================================")
print("       Entity Control Test Complete")
print("========================================")
