#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <string>

#include "simulation/SimControlInterface.h"
#include "simulation/MockSimController.h"
#include "scripting/LuaSimBinding.h"

using namespace simulation;
using namespace scripting;

// Trim whitespace from both ends of a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Convert string to lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void printUsage() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help              - Show this help message" << std::endl;
    std::cout << "  quit/exit         - Exit the program" << std::endl;
    std::cout << "  clear             - Clear screen" << std::endl;
    std::cout << "  <script_path>     - Execute a Lua script file" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  LuaSim> scripts/example_control.lua" << std::endl;
    std::cout << "  LuaSim> scripts/entity_control_test.lua" << std::endl;
}

void runInteractiveMode(LuaSimBinding* luaBinding) {
    std::cout << "Entering interactive mode. Type 'help' for usage, 'quit' to exit." << std::endl;
    std::cout << std::endl;

    std::string input;
    while (true) {
        std::cout << "LuaSim> ";
        std::getline(std::cin, input);

        // Trim input
        input = trim(input);

        // Skip empty input
        if (input.empty()) {
            continue;
        }

        // Convert to lowercase for command comparison
        std::string cmd = toLower(input);

        // Handle commands
        if (cmd == "quit" || cmd == "exit") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }
        else if (cmd == "help") {
            printUsage();
        }
        else if (cmd == "clear") {
            // Clear screen (platform independent way)
            std::cout << "\033[2J\033[1;1H";
        }
        else {
            // Treat input as script path
            std::cout << "Executing script: " << input << std::endl;
            std::cout << "----------------------------------------" << std::endl;

            if (luaBinding->executeScript(input)) {
                std::cout << "----------------------------------------" << std::endl;
                std::cout << "OK: Script executed successfully" << std::endl;
            }
            else {
                std::cout << "----------------------------------------" << std::endl;
                std::cerr << "ERROR: Script execution failed: " << luaBinding->getLastError() << std::endl;
            }
        }

        std::cout << std::endl;
    }
}

int main() {
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

    // Run interactive mode
    runInteractiveMode(luaBinding.get());

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
