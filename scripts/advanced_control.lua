-- Example: Advanced Simulation Control Script
-- Demonstrates event callbacks and speed control

print("========================================")
print("       Advanced Control Example")
print("========================================")
print("")

-- Set simulation speed
print("1. Set simulation speed")
print("   Default speed: " .. sim.get_speed() .. "x")
sim.set_speed(2.0)  -- 2x speed
print("   New speed: " .. sim.get_speed() .. "x")
print("")

-- Register event callbacks
print("2. Register event callbacks")

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
print("3. Start simulation (triggers on_start callback)")
sim.start()
sleep(0.3)
print("")

-- Test different speeds
print("4. Test different speeds")
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
print("5. Pause and resume (triggers callbacks)")
sim.pause()
sleep(0.2)
sim.resume()
sleep(0.2)
print("")

-- Get simulation parameters
print("6. Simulation parameters")
print("   Time step: " .. sim.get_time_step() .. "s")
print("   Current speed: " .. sim.get_speed() .. "x")
print("   Current time: " .. string.format("%.2f", sim.get_time()) .. "s")
print("   Is running: " .. tostring(sim.is_running()))
print("")

-- Conditional wait example
print("7. Conditional wait example")
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
print("8. Stop simulation (triggers on_stop callback)")
sim.stop()
print("")

-- Reset
print("9. Reset simulation (triggers on_reset callback)")
sim.reset()
print("")

print("========================================")
print("       Advanced example finished")
print("========================================")
