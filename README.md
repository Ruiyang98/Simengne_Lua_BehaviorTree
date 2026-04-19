# Lua Simulation Control

A C++ project demonstrating Lua script control for simulation engine using sol2 and BehaviorTree.CPP.

## Overview

This project provides a framework for controlling simulation engines through Lua scripts. It includes:

- **SimControlInterface**: Abstract interface for simulation control (start, pause, resume, stop, reset)
- **MockSimController**: A concrete implementation for testing and demonstration
- **LuaSimBinding**: Lua-C++ binding using sol2 library
- **Example Lua Scripts**: Demonstrating basic and advanced usage

## Features

- Control simulation state (start, pause, resume, stop, reset)
- Query simulation status and time
- Adjust simulation speed (time scale)
- Event callbacks (on_start, on_pause, on_resume, on_stop, on_reset)
- Execute Lua scripts from files or strings

## Project Structure

```
TestProject/
├── include/
│   ├── simulation/
│   │   ├── SimControlInterface.h    # Simulation control interface
│   │   └── MockSimController.h      # Mock implementation
│   └── scripting/
│       └── LuaSimBinding.h          # Lua binding module
├── src/
│   ├── main.cpp                     # Main entry point
│   ├── simulation/
│   │   ├── SimControlInterface.cpp
│   │   ├── MockSimController.cpp
│   │   └── CMakeLists.txt
│   └── scripting/
│       ├── LuaSimBinding.cpp
│       └── CMakeLists.txt
├── scripts/
│   ├── example_control.lua          # Basic example
│   └── advanced_control.lua         # Advanced example with callbacks
├── 3rdparty/
│   └── lua/                         # Lua 5.1.5 and sol2
├── cmake/
│   └── lua.cmake                    # Lua CMake configuration
└── CMakeLists.txt                   # Root CMake configuration
```

## Dependencies

- CMake 3.10+
- C++11 compatible compiler
- Lua 5.1.5 (included in 3rdparty)
- sol2 v2.17.5 (included in 3rdparty)

## Building

### Windows

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Linux/macOS

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Usage

### Running the Demo

```bash
# Run default demo
./Release/my_app

# Run example script
./Release/my_app scripts/example_control.lua

# Run advanced script
./Release/my_app scripts/advanced_control.lua
```

### Lua API

The following Lua API is available through the `sim` table:

#### Control Commands
```lua
sim.start()           -- Start simulation
sim.pause()           -- Pause simulation
sim.resume()          -- Resume simulation
sim.stop()            -- Stop simulation
sim.reset()           -- Reset simulation
```

#### State Queries
```lua
sim.get_state()       -- Get state as string ("STOPPED", "RUNNING", "PAUSED")
sim.is_running()      -- Check if running
sim.is_paused()       -- Check if paused
sim.is_stopped()      -- Check if stopped
sim.get_time()        -- Get simulation time
sim.get_time_step()   -- Get time step
```

#### Speed Control
```lua
sim.set_speed(scale)  -- Set time scale (1.0 = real-time)
sim.get_speed()       -- Get current time scale
```

#### Event Callbacks
```lua
sim.on_start(function()
    print("Simulation started!")
end)

sim.on_pause(function()
    print("Simulation paused!")
end)

sim.on_resume(function()
    print("Simulation resumed!")
end)

sim.on_stop(function()
    print("Simulation stopped!")
end)

sim.on_reset(function()
    print("Simulation reset!")
end)
```

#### Utility Functions
```lua
sleep(seconds)        -- Sleep for specified seconds
```

## Example Script

```lua
-- Example: Basic simulation control
print("=== Simulation Control Demo ===")

-- Register callbacks
sim.on_start(function()
    print("[Callback] Simulation started!")
end)

-- Start simulation
sim.start()
sleep(0.5)

-- Check status
print("State: " .. sim.get_state())
print("Time: " .. sim.get_time() .. "s")

-- Pause and resume
sim.pause()
sim.resume()

-- Stop simulation
sim.stop()

print("=== Demo finished ===")
```

## Architecture

### SimControlInterface

The abstract interface that defines simulation control operations:

- `start()`, `pause()`, `resume()`, `stop()`, `reset()` - Control commands
- `getState()`, `isRunning()`, `isPaused()`, `isStopped()` - State queries
- `getSimTime()`, `getTimeStep()` - Time queries
- `setTimeScale()`, `getTimeScale()` - Speed control
- Event callback setters

### MockSimController

A concrete implementation of `SimControlInterface` for testing:
- Simulates time progression
- Supports auto-update thread
- Provides verbose logging

### LuaSimBinding

Uses sol2 to bind C++ simulation interface to Lua:
- Creates `sim` table in Lua global namespace
- Wraps C++ methods as Lua functions
- Handles Lua callbacks with error protection

## License

This project is provided as an example implementation. Please check the licenses of included third-party libraries:
- Lua: MIT License
- sol2: MIT License

## Contributing

Feel free to fork and modify this project for your own simulation control needs.
