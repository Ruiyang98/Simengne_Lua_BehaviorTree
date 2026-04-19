#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "simulation/SimControlInterface.h"
#include "simulation/MockSimController.h"
#include "scripting/LuaSimBinding.h"

using namespace simulation;
using namespace scripting;

void printUsage() {
    std::cout << "Usage: my_app [script file]" << std::endl;
    std::cout << "  Example: my_app scripts/example_control.lua" << std::endl;
    std::cout << "           my_app scripts/advanced_control.lua" << std::endl;
    std::cout << "           my_app (no args for default demo)" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "    Lua Simulation Control Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Create simulation controller
    std::unique_ptr<MockSimController> simController(new MockSimController());
    simController->setVerbose(true);
    
    // Create Lua binding
    std::unique_ptr<LuaSimBinding> luaBinding(new LuaSimBinding(simController.get()));
    
    // Initialize Lua environment
    if (!luaBinding->initialize()) {
        std::cerr << "Lua initialization failed: " << luaBinding->getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "OK: Lua environment initialized" << std::endl;
    std::cout << std::endl;

    // Execute based on arguments
    if (argc > 1) {
        // Execute specified Lua script file
        std::string scriptPath = argv[1];
        std::cout << "Executing script: " << scriptPath << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        if (luaBinding->executeScript(scriptPath)) {
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "OK: Script executed successfully" << std::endl;
        } else {
            std::cout << "----------------------------------------" << std::endl;
            std::cerr << "ERROR: Script execution failed: " << luaBinding->getLastError() << std::endl;
            return 1;
        }
    } else {
        // Execute default demo
        std::cout << "Running default demo..." << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        // Embedded default Lua script
        const char* defaultScript = 
            "print('=== Lua Simulation Control Demo ===')\n"
            "print('')\n"
            "print('1. Check initial state')\n"
            "print('   State: ' .. sim.get_state())\n"
            "print('   Time: ' .. sim.get_time() .. 's')\n"
            "print('')\n"
            "sim.on_start(function()\n"
            "    print('   [Callback] Simulation started!')\n"
            "end)\n"
            "sim.on_stop(function()\n"
            "    print('   [Callback] Simulation stopped!')\n"
            "end)\n"
            "print('2. Start simulation')\n"
            "sim.start()\n"
            "sleep(0.5)\n"
            "print('   Time after 0.5s: ' .. string.format('%.2f', sim.get_time()) .. 's')\n"
            "print('')\n"
            "print('3. Pause and resume')\n"
            "sim.pause()\n"
            "print('   State after pause: ' .. sim.get_state())\n"
            "sim.resume()\n"
            "print('   State after resume: ' .. sim.get_state())\n"
            "sleep(0.3)\n"
            "print('')\n"
            "print('4. Stop simulation')\n"
            "sim.stop()\n"
            "print('   Final time: ' .. string.format('%.2f', sim.get_time()) .. 's')\n"
            "print('')\n"
            "print('=== Demo finished ===')\n";
        
        if (luaBinding->executeString(defaultScript)) {
            std::cout << "----------------------------------------" << std::endl;
            std::cout << "OK: Demo executed successfully" << std::endl;
        } else {
            std::cout << "----------------------------------------" << std::endl;
            std::cerr << "ERROR: Demo execution failed: " << luaBinding->getLastError() << std::endl;
            return 1;
        }
        
        std::cout << std::endl;
        printUsage();
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    Program finished normally" << std::endl;
    std::cout << "========================================" << std::endl;

    // Explicitly destroy Lua binding before sim controller to avoid
    // dangling callbacks
    luaBinding.reset();
    simController.reset();

    return 0;
}
