#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>

#include "simulation/SimControlInterface.h"
#include "simulation/MockSimController.h"
#include "scripting/LuaSimBinding.h"
#include "behaviortree/BehaviorTreeExecutor.h"
#include "behaviortree/BlackboardKeys.h"

using namespace simulation;
using namespace scripting;
using namespace behaviortree;

// Global simulation controller and BT executor
std::unique_ptr<MockSimController> g_simController;
std::unique_ptr<BehaviorTreeExecutor> g_btExecutor;

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

// Split string by delimiter
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

void printUsage() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help                        - Show this help message" << std::endl;
    std::cout << "  quit/exit                   - Exit the program" << std::endl;
    std::cout << "  clear                       - Clear screen" << std::endl;
    std::cout << "  entity                      - List all entities" << std::endl;
    std::cout << "  entity add <type> <x> <y>   - Add a new entity" << std::endl;
    std::cout << "  bt <xml_file> [tree_name] [-e <id>]  - Execute behavior tree from XML" << std::endl;
    std::cout << "  bt list                     - List available behavior trees" << std::endl;
    std::cout << "  lua <script_path>           - Execute a Lua script file" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  > bt bt_xml/square_path_composite.xml SquarePathComposite" << std::endl;
    std::cout << "  > bt bt_xml/square_path.xml SquarePath" << std::endl;
    std::cout << "  > bt bt_xml/square_path.xml SquarePath -e npc_001" << std::endl;
    std::cout << "  > entity add npc 10 20" << std::endl;
    std::cout << "  > lua scripts/example_control.lua" << std::endl;
}

// Execute behavior tree from command
bool executeBehaviorTree(const std::string& xmlFile, const std::string& treeName, const std::string& entityId = "") {
    if (!g_btExecutor) {
        std::cerr << "ERROR: Behavior tree executor not initialized" << std::endl;
        return false;
    }
    
    std::cout << "----------------------------------------" << std::endl;
    
    // Load behavior tree XML file
    if (!g_btExecutor->loadFromFile(xmlFile)) {
        std::cerr << "ERROR: Failed to load behavior tree: " << g_btExecutor->getLastError() << std::endl;
        return false;
    }
    
    std::cout << "OK: Behavior tree loaded from: " << xmlFile << std::endl;
    
    // Create blackboard and set parameters
    auto blackboard = BT::Blackboard::create();
    
    // Use specified entity ID or find/create one
    if (!entityId.empty()) {
        // Verify the entity exists
        double x, y, z;
        if (g_simController->getEntityPosition(entityId, x, y, z)) {
            blackboard->set(BlackboardKeys::ENTITY_ID, entityId);
            std::cout << "INFO: Using specified entity: " << entityId << std::endl;
        } else {
            std::cerr << "ERROR: Entity not found: " << entityId << std::endl;
            return false;
        }
    } else {
        // Try to get an existing entity
        auto entities = g_simController->getAllEntities();
        if (!entities.empty()) {
            blackboard->set(BlackboardKeys::ENTITY_ID, entities[0].id);
            std::cout << "INFO: Using existing entity: " << entities[0].id << std::endl;
        } else {
            // Create a test entity
            std::string newId = g_simController->addEntity("npc", 0.0, 0.0, 0.0);
            blackboard->set(BlackboardKeys::ENTITY_ID, newId);
            std::cout << "INFO: Created test entity: " << newId << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Executing behavior tree: " << treeName << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // Execute behavior tree
    BT::NodeStatus status = g_btExecutor->execute(treeName, blackboard);
    
    std::cout << "----------------------------------------" << std::endl;
    
    if (status == BT::NodeStatus::SUCCESS) {
        std::cout << "OK: Behavior tree executed successfully" << std::endl;
        return true;
    } else {
        std::cerr << "ERROR: Behavior tree execution failed" << std::endl;
        return false;
    }
}

// List all entities
void listEntities() {
    auto entities = g_simController->getAllEntities();
    std::cout << "Entities (" << entities.size() << "):" << std::endl;
    for (const auto& entity : entities) {
        std::cout << "  - " << entity.id << " (" << entity.type << ") at ("
                  << entity.x << ", " << entity.y << ", " << entity.z << ")" << std::endl;
    }
}

// Add a new entity
void addEntity(const std::vector<std::string>& args) {
    if (args.size() < 5) {
        std::cerr << "Usage: entity add <type> <x> <y> [z]" << std::endl;
        return;
    }
    
    std::string type = args[2];
    double x = std::stod(args[3]);
    double y = std::stod(args[4]);
    double z = (args.size() > 5) ? std::stod(args[5]) : 0.0;
    
    std::string id = g_simController->addEntity(type, x, y, z);
    std::cout << "Created entity: " << id << " at (" << x << ", " << y << ", " << z << ")" << std::endl;
}

// List available behavior tree XML files
void listBehaviorTrees() {
    std::cout << "Available behavior tree files:" << std::endl;
    std::cout << "  - bt_xml/path_movement.xml" << std::endl;
    std::cout << "  - bt_xml/square_path.xml" << std::endl;
    std::cout << "  - bt_xml/square_path_composite.xml" << std::endl;
    std::cout << "  - bt_xml/waypoint_patrol.xml" << std::endl;
    std::cout << std::endl;
    std::cout << "Example usage:" << std::endl;
    std::cout << "  > bt bt_xml/square_path_composite.xml SquarePathComposite" << std::endl;
    std::cout << "  > bt bt_xml/square_path.xml SquarePath" << std::endl;
    std::cout << "  > bt bt_xml/square_path.xml SquarePath -e <entity_id>" << std::endl;
}

// Handle behavior tree command
// Usage: bt <xml_file> [tree_name] [-e <entity_id>]
//        bt <xml_file> [-e <entity_id>] [tree_name]
void handleBtCommand(const std::vector<std::string>& args) {
    if (args.size() == 1 || (args.size() == 2 && args[1] == "list")) {
        listBehaviorTrees();
        return;
    }
    
    if (args.size() < 2) {
        std::cerr << "Usage: bt <xml_file> [tree_name] [-e <entity_id>]" << std::endl;
        return;
    }
    
    std::string xmlFile = args[1];
    std::string treeName = "MainTree";
    std::string entityId = "";
    
    // Parse remaining arguments
    for (size_t i = 2; i < args.size(); ++i) {
        if ((args[i] == "-e" || args[i] == "--entity") && i + 1 < args.size()) {
            entityId = args[i + 1];
            ++i; // Skip the next argument as it's the entity ID
        } else if (treeName == "MainTree") {
            // First non-option argument is treated as tree name
            treeName = args[i];
        }
    }
    
    executeBehaviorTree(xmlFile, treeName, entityId);
}

void runInteractiveMode(LuaSimBinding* luaBinding) {
    std::cout << "Entering interactive mode. Type 'help' for usage, 'quit' to exit." << std::endl;
    std::cout << std::endl;

    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        // Trim input
        input = trim(input);

        // Skip empty input
        if (input.empty()) {
            continue;
        }

        // Split input into command and arguments
        std::vector<std::string> args = split(input, ' ');
        if (args.empty()) {
            continue;
        }

        // Convert command to lowercase
        std::string cmd = toLower(args[0]);

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
        else if (cmd == "bt") {
            handleBtCommand(args);
        }
        else if (cmd == "entity") {
            if (args.size() > 1 && toLower(args[1]) == "add") {
                addEntity(args);
            } else {
                listEntities();
            }
        }
        else if (cmd == "lua" && args.size() > 1) {
            // Execute Lua script
            std::string scriptPath = args[1];
            std::cout << "Executing Lua script: " << scriptPath << std::endl;
            std::cout << "----------------------------------------" << std::endl;

            if (luaBinding->executeScript(scriptPath)) {
                std::cout << "----------------------------------------" << std::endl;
                std::cout << "OK: Script executed successfully" << std::endl;
            }
            else {
                std::cout << "----------------------------------------" << std::endl;
                std::cerr << "ERROR: Script execution failed: " << luaBinding->getLastError() << std::endl;
            }
        }
        else {
            std::cout << "Error input: " << input << std::endl;
        }

        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Create simulation controller
    g_simController.reset(new MockSimController());
    g_simController->setVerbose(true);
    
    // Create behavior tree executor
    g_btExecutor.reset(new BehaviorTreeExecutor(g_simController.get()));
    if (!g_btExecutor->initialize()) {
        std::cerr << "ERROR: Failed to initialize behavior tree executor: " 
                  << g_btExecutor->getLastError() << std::endl;
        return 1;
    }
    
    // Interactive mode
    std::cout << "========================================" << std::endl;
    std::cout << "    Simulation Control System" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "OK: Simulation controller initialized" << std::endl;
    std::cout << "OK: Behavior tree executor initialized" << std::endl;
    std::cout << std::endl;

    // Create Lua binding
    std::unique_ptr<LuaSimBinding> luaBinding(new LuaSimBinding(g_simController.get()));

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

    // Cleanup
    luaBinding.reset();
    g_btExecutor.reset();
    g_simController.reset();

    return 0;
}
