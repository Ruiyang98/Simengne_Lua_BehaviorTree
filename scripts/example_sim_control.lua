-- Simulation Control Examples
-- 仿真控制示例合集
-- 包含：基础仿真控制、高级仿真控制（事件回调、速度控制）

print("========================================")
print("    Simulation Control Examples")
print("========================================")
print("")

-- ============================================================================
-- Part 1: Basic Simulation Control - 基础仿真控制
-- ============================================================================

local function run_basic_control()
    print("--- Part 1: Basic Simulation Control ---")
    print("")

    -- Check initial state
    print("1.1 Check initial state")
    print("   Current state: " .. sim.get_state())
    print("   Is stopped: " .. tostring(sim.is_stopped()))
    print("")

    -- Start simulation
    print("1.2 Start simulation")
    local success = sim.start()
    if success then
        print("   OK: Simulation started")
    else
        print("   ERROR: Failed to start simulation")
    end
    print("   Current state: " .. sim.get_state())
    print("")

    -- Simulate running for a while
    print("1.3 Simulation running...")
    print("   Waiting 0.5 seconds...")
    sleep(0.5)
    print("   Current simulation time: " .. string.format("%.2f", sim.get_time()) .. " seconds")
    print("")

    -- Pause simulation
    print("1.4 Pause simulation")
    success = sim.pause()
    if success then
        print("   OK: Simulation paused")
    else
        print("   ERROR: Failed to pause simulation")
    end
    print("   Current state: " .. sim.get_state())
    print("   Is paused: " .. tostring(sim.is_paused()))
    print("")

    -- Resume simulation
    print("1.5 Resume simulation")
    success = sim.resume()
    if success then
        print("   OK: Simulation resumed")
    else
        print("   ERROR: Failed to resume simulation")
    end
    print("   Current state: " .. sim.get_state())
    print("")

    -- Run for a bit more
    print("1.6 Continue running...")
    sleep(0.3)
    print("   Current simulation time: " .. string.format("%.2f", sim.get_time()) .. " seconds")
    print("")

    -- Stop simulation
    print("1.7 Stop simulation")
    success = sim.stop()
    if success then
        print("   OK: Simulation stopped")
    else
        print("   ERROR: Failed to stop simulation")
    end
    print("   Current state: " .. sim.get_state())
    print("   Final simulation time: " .. string.format("%.2f", sim.get_time()) .. " seconds")
    print("")

    -- Reset simulation
    print("1.8 Reset simulation")
    success = sim.reset()
    if success then
        print("   OK: Simulation reset")
    else
        print("   ERROR: Failed to reset simulation")
    end
    print("   Current state: " .. sim.get_state())
    print("   Time after reset: " .. string.format("%.2f", sim.get_time()) .. " seconds")
    print("")
end

-- ============================================================================
-- Part 2: Advanced Simulation Control - 高级仿真控制
-- ============================================================================

local function run_advanced_control()
    print("--- Part 2: Advanced Simulation Control ---")
    print("")

    -- Set simulation speed
    print("2.1 Set simulation speed")
    print("   Default speed: " .. sim.get_speed() .. "x")
    sim.set_speed(2.0)  -- 2x speed
    print("   New speed: " .. sim.get_speed() .. "x")
    print("")

    -- Register event callbacks
    print("2.2 Register event callbacks")

    sim.on_start(function()
        print("   [Callback] Simulation started!")
    end)

    sim.on_pause(function()
        print("   [Callback] Simulation paused!")
    end)

    sim.on_resume(function()
        print("   [Callback] Simulation resumed!")
    end)

    sim.on_stop(function()
        print("   [Callback] Simulation stopped!")
    end)

    sim.on_reset(function()
        print("   [Callback] Simulation reset!")
    end)

    print("   OK: All callbacks registered")
    print("")

    -- Start simulation
    print("2.3 Start simulation (triggers on_start callback)")
    sim.start()
    sleep(0.3)
    print("")

    -- Test different speeds
    print("2.4 Test different speeds")
    print("   Current time: " .. string.format("%.2f", sim.get_time()) .. "s")

    sim.set_speed(0.5)  -- Half speed
    print("   Switching to 0.5x speed...")
    sleep(0.5)
    print("   Current time: " .. string.format("%.2f", sim.get_time()) .. "s")

    sim.set_speed(3.0)  -- 3x speed
    print("   Switching to 3x speed...")
    sleep(0.5)
    print("   Current time: " .. string.format("%.2f", sim.get_time()) .. "s")
    print("")

    -- Pause and resume
    print("2.5 Pause and resume (triggers callbacks)")
    sim.pause()
    sleep(0.2)
    sim.resume()
    sleep(0.2)
    print("")

    -- Get simulation parameters
    print("2.6 Simulation parameters")
    print("   Time step: " .. sim.get_time_step() .. "s")
    print("   Current speed: " .. sim.get_speed() .. "x")
    print("   Current time: " .. string.format("%.2f", sim.get_time()) .. "s")
    print("   Is running: " .. tostring(sim.is_running()))
    print("")

    -- Conditional wait example
    print("2.7 Conditional wait example")
    print("   Waiting for simulation time to exceed 3 seconds...")

    -- Ensure simulation is running
    if not sim.is_running() then
        sim.start()
    end

    -- Use loop to wait
    local start_time = os.clock()
    while sim.get_time() < 3.0 do
        sleep(0.1)
        -- Prevent infinite wait
        if os.clock() - start_time > 10 then
            print("   Wait timeout!")
            break
        end
    end

    if sim.get_time() >= 3.0 then
        print("   OK: Condition met! Current time: " .. string.format("%.2f", sim.get_time()) .. "s")
    else
        print("   ERROR: Wait failed")
    end
    print("")

    -- Stop simulation
    print("2.8 Stop simulation (triggers on_stop callback)")
    sim.stop()
    print("")

    -- Reset
    print("2.9 Reset simulation (triggers on_reset callback)")
    sim.reset()
    print("")
end

-- ============================================================================
-- Part 3: Entity Management with Simulation Control - 实体管理与仿真控制
-- ============================================================================

local function run_entity_with_sim_control()
    print("--- Part 3: Entity Management with Simulation Control ---")
    print("")

    -- Start simulation
    print("3.1 Start simulation")
    sim.start()
    print("   Simulation started")
    print("")

    -- Add entities while simulation is running
    print("3.2 Add entities while simulation is running")
    local npc1 = sim.add_entity("npc", 0.0, 0.0, 0.0)
    local npc2 = sim.add_entity("npc", 10.0, 0.0, 0.0)
    local player = sim.add_entity("player", 5.0, 5.0, 0.0)
    print("   Added NPC 1: " .. npc1)
    print("   Added NPC 2: " .. npc2)
    print("   Added Player: " .. player)
    print("   Total entities: " .. sim.get_entity_count())
    print("")

    -- Move entities while simulation is running
    print("3.3 Move entities while simulation is running")
    sleep(0.5)
    sim.move_entity(npc1, 5.0, 5.0, 0.0)
    print("   Moved NPC 1 to (5.0, 5.0)")

    local pos = sim.get_entity_position(npc1)
    if pos then
        print("   NPC 1 position: (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")
    end
    print("")

    -- Pause and modify
    print("3.4 Pause simulation and modify entities")
    sim.pause()
    sleep(0.2)
    sim.move_entity(npc2, 15.0, 15.0, 0.0)
    print("   Moved NPC 2 to (15.0, 15.0) while paused")
    sim.resume()
    print("")

    -- Get all entities
    print("3.5 Get all entities")
    local all_entities = sim.get_all_entities()
    print("   Total entities: " .. #all_entities)
    for i, entity in ipairs(all_entities) do
        print("   Entity " .. i .. ": " .. entity.id .. " (type: " .. entity.type .. ")")
    end
    print("")

    -- Clean up
    print("3.6 Clean up entities")
    sim.remove_entity(npc1)
    sim.remove_entity(npc2)
    sim.remove_entity(player)
    print("   All entities removed")
    print("   Final entity count: " .. sim.get_entity_count())
    print("")

    -- Stop simulation
    sim.stop()
    print("   Simulation stopped")
    print("")
end

-- ============================================================================
-- Main Execution
-- ============================================================================

print("Starting Simulation Control Examples...")
print("")

run_basic_control()
run_advanced_control()
run_entity_with_sim_control()

print("========================================")
print("    Simulation Control Examples Complete")
print("========================================")
print("")
print("Summary:")
print("  - Basic control: start/pause/resume/stop/reset")
print("  - Speed control: set_speed/get_speed")
print("  - Event callbacks: on_start/on_pause/on_resume/on_stop/on_reset")
print("  - State queries: get_state/is_running/is_paused/is_stopped")
print("  - Entity management during simulation")
