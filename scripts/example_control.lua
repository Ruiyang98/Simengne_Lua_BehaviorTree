-- Example: Basic Simulation Control Script
-- Demonstrates how to use Lua API to control simulation engine

print("========================================")
print("       Simulation Control Example")
print("========================================")
print("")

-- Check initial state
print("1. Check initial state")
print("   Current state: " .. sim.get_state())
print("   Is stopped: " .. tostring(sim.is_stopped()))
print("")

-- Start simulation
print("2. Start simulation")
local success = sim.start()
if success then
    print("   OK: Simulation started")
else
    print("   ERROR: Failed to start simulation")
end
print("   Current state: " .. sim.get_state())
print("")

-- Simulate running for a while
print("3. Simulation running...")
print("   Waiting 0.5 seconds...")
sleep(0.5)
print("   Current simulation time: " .. string.format("%.2f", sim.get_time()) .. " seconds")
print("")

-- Pause simulation
print("4. Pause simulation")
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
print("5. Resume simulation")
success = sim.resume()
if success then
    print("   OK: Simulation resumed")
else
    print("   ERROR: Failed to resume simulation")
end
print("   Current state: " .. sim.get_state())
print("")

-- Run for a bit more
print("6. Continue running...")
sleep(0.3)
print("   Current simulation time: " .. string.format("%.2f", sim.get_time()) .. " seconds")
print("")

-- Stop simulation
print("7. Stop simulation")
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
print("8. Reset simulation")
success = sim.reset()
if success then
    print("   OK: Simulation reset")
else
    print("   ERROR: Failed to reset simulation")
end
print("   Current state: " .. sim.get_state())
print("   Time after reset: " .. string.format("%.2f", sim.get_time()) .. " seconds")
print("")

print("========================================")
print("       Example finished")
print("========================================")
