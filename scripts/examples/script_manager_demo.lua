-- Script Manager Demo
-- Demonstrates how to use EntityScriptManager with script state isolation

print("========================================")
print("    Script Manager Demo")
print("========================================")
print("")

-- Create or use an entity for testing
local entity_id = "1"

-- Check if entity exists, if not create one
if sim.get_entity_count() == 0 then
    print("Creating test entity...")
    local vid = sim.add_entity("guard", 0, 0, 0)
    entity_id = tostring(vid.vehicle)
    print(string.format("Created entity with vehicle ID: %s", entity_id))
else
    -- Use first entity
    local entities = sim.get_all_entities()
    entity_id = tostring(entities[1].id.vehicle)
    print(string.format("Using existing entity with vehicle ID: %s", entity_id))
end

-- Create an enemy entity for testing
print("Creating enemy entity...")
local enemy_vid = sim.add_entity("enemy", 5, 5, 0)
print(string.format("Created enemy with vehicle ID: %d", enemy_vid.vehicle))

print("")
print("----------------------------------------")
print("Creating script manager for entity " .. entity_id)
print("----------------------------------------")

-- Create script manager
local manager = sim.create_script_manager(entity_id)
if not manager then
    print("ERROR: Failed to create script manager")
    return
end

print("Script manager created successfully!")
print("")

-- Add multiple scripts to demonstrate state isolation
print("----------------------------------------")
print("Adding multiple scripts with state isolation...")
print("----------------------------------------")

-- Script 1: Patrol
local patrol_script = [[
function execute(state)
    -- Initialize state
    state.patrol_point = state.patrol_point or "A"
    state.patrol_count = (state.patrol_count or 0) + 1
    
    -- Patrol logic
    local points = {"A", "B", "C", "D"}
    local current_index = 1
    for i, p in ipairs(points) do
        if p == state.patrol_point then
            current_index = i
            break
        end
    end
    
    -- Move to next point
    local next_index = (current_index % #points) + 1
    state.patrol_point = points[next_index]
    
    -- Share data with other scripts
    entity.set_var("current_patrol_point", state.patrol_point)
    entity.set_var("patrol_count", state.patrol_count)
    
    print(string.format("[Patrol] Entity %s moving to point %s (count: %d)",
        entity.id, state.patrol_point, state.patrol_count))
end
]]

local success1 = manager:add_tactical_script("patrol", patrol_script)
if success1 then
    print("Added 'patrol' script")
else
    print("ERROR: Failed to add patrol script")
end

-- Script 2: Attack (demonstrates state isolation)
local attack_script = [[
function execute(state)
    -- This script has its OWN state, independent of patrol
    state.target_count = (state.target_count or 0) + 1
    state.attack_mode = state.attack_mode or "defensive"
    
    -- Read shared data from patrol script
    local patrol_point = entity.get_var("current_patrol_point")
    local patrol_count = entity.get_var("patrol_count")
    
    print(string.format("[Attack] Entity %s attack count: %d, mode: %s", 
        entity.id, state.target_count, state.attack_mode))
    print(string.format("[Attack] Patrol is at point %s (patrol count: %d)",
        tostring(patrol_point), patrol_count or 0))
    
    -- Change attack mode based on target count
    if state.target_count > 5 then
        state.attack_mode = "aggressive"
    end
end
]]

local success2 = manager:add_tactical_script("attack", attack_script)
if success2 then
    print("Added 'attack' script")
else
    print("ERROR: Failed to add attack script")
end

-- Script 3: Guard (another independent state)
local guard_script = [[
function execute(state)
    -- Another independent state
    state.alert_level = state.alert_level or 0
    state.guard_count = (state.guard_count or 0) + 1
    
    -- Increase alert level
    state.alert_level = math.min((state.alert_level + 1), 10)
    
    -- Read shared data
    local patrol_point = entity.get_var("current_patrol_point")
    
    print(string.format("[Guard] Entity %s guarding at level %d (count: %d)", 
        entity.id, state.alert_level, state.guard_count))
    print(string.format("[Guard] Current patrol point: %s", tostring(patrol_point)))
end
]]

local success3 = manager:add_tactical_script("guard", guard_script)
if success3 then
    print("Added 'guard' script")
else
    print("ERROR: Failed to add guard script")
end

print("")
print("----------------------------------------")
print("Script list:")
print("----------------------------------------")

local scripts = manager:get_scripts()
for i, name in ipairs(scripts) do
    print(string.format("  %d. %s", i, name))
end

print("")
print("----------------------------------------")
print("Summary:")
print("----------------------------------------")
print("Script count: " .. manager:get_script_count())
print("")
print("Key features demonstrated:")
print("  - Each script has its OWN state table (isolation)")
print("  - Scripts can share data via entity.vars")
print("  - Global tables (entity, sim, bt) are accessible")
print("")
print("To test:")
print("  1. Start simulation: sim.start()")
print("  2. Wait for script execution (every 500ms)")
print("  3. Observe each script's independent state")
print("")
