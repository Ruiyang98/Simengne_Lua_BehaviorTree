#include "scripting/LuaBehaviorTreeBridge.h"
#include "scripting/LuaSimBinding.h"
#include "behaviortree/BehaviorTreeExecutor.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace scripting {

// LuaBehaviorTreeBridge implementation
// Dependencies are obtained from singletons
LuaBehaviorTreeBridge::LuaBehaviorTreeBridge()
    : luaState_(&LuaSimBinding::getInstance().getState())
    , factory_(&behaviortree::BehaviorTreeExecutor::getInstance().getFactory())
    , treeIdCounter_(0)
    , initialized_(false) {}

LuaBehaviorTreeBridge::~LuaBehaviorTreeBridge() {
    // Stop all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
        }
    }
    activeTrees_.clear();
}

bool LuaBehaviorTreeBridge::initialize() {
    if (!luaState_ || !factory_) {
        lastError_ = "Lua state or BT factory is null";
        return false;
    }

    try {
        registerLuaNodeTypes();
        registerLuaAPI();

        // Auto-load nodes registry
        loadNodesRegistry("scripts/bt_nodes_registry.lua");

        // Auto-preload behavior trees from default directories
        const std::vector<std::string> defaultDirs = {
            "bt_xml/",
            "behavior_trees/"
        };

        for (const auto& dir : defaultDirs) {
            preloadBehaviorTreesFromDirectory(dir);
        }

        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to initialize bridge: ") + e.what();
        return false;
    }
}

void LuaBehaviorTreeBridge::registerLuaNodeTypes() {
    // Set Lua state for parameter collection
    LuaActionNode::setLuaState(luaState_);
    LuaConditionNode::setLuaState(luaState_);
    LuaStatefulActionNode::setLuaState(luaState_);

    // Register Lua action node type
    factory_->registerNodeType<LuaActionNode>("LuaAction");

    // Register Lua condition node type
    factory_->registerNodeType<LuaConditionNode>("LuaCondition");

    // Register Lua stateful action node type
    factory_->registerNodeType<LuaStatefulActionNode>("LuaStatefulAction");
}

void LuaBehaviorTreeBridge::registerLuaAPI() {
    // Create bt table
    sol::table btTable = luaState_->create_named_table("bt");
    
    // Load behavior tree from file
    btTable.set_function("load_file", [this](const std::string& xmlFile) -> bool {
        return loadBehaviorTreeFromFile(xmlFile);
    });
    
    // Load behavior tree from text
    btTable.set_function("load_text", [this](const std::string& xmlText) -> bool {
        return loadBehaviorTreeFromText(xmlText);
    });
    
    // Execute behavior tree
    btTable.set_function("execute", [this](const std::string& treeName,
                                            sol::optional<std::string> entityId,
                                            sol::optional<sol::table> params) -> std::string {
        std::string id = entityId.value_or("");
        return executeBehaviorTree(treeName, id, params);
    });
    
    // Get tree status
    btTable.set_function("get_status", [this](const std::string& treeId) -> std::string {
        return getTreeStatus(treeId);
    });
    
    // Stop tree
    btTable.set_function("stop", [this](const std::string& treeId) -> bool {
        return stopBehaviorTree(treeId);
    });
    
    // Set blackboard value
    btTable.set_function("set_blackboard", [this](const std::string& treeId,
                                                   const std::string& key,
                                                   sol::object value) -> bool {
        return setBlackboardValue(treeId, key, value);
    });
    
    // Get blackboard value
    btTable.set_function("get_blackboard", [this](const std::string& treeId,
                                                   const std::string& key) -> sol::object {
        return getBlackboardValue(treeId, key);
    });
    
    // Register Lua action
    btTable.set_function("register_action", [this](const std::string& name,
                                                    sol::protected_function func) -> bool {
        return registerLuaAction(name, func);
    });
    
    // Register Lua condition
    btTable.set_function("register_condition", [this](const std::string& name,
                                                       sol::protected_function func) -> bool {
        return registerLuaCondition(name, func);
    });

    // Register Lua stateful action
    btTable.set_function("register_stateful_action", [this](const std::string& name,
                                                             sol::protected_function onStart,
                                                             sol::protected_function onRunning,
                                                             sol::protected_function onHalted) -> bool {
        return registerLuaStatefulAction(name, onStart, onRunning, onHalted);
    });
    
    // Check if tree exists
    btTable.set_function("has_tree", [this](const std::string& treeId) -> bool {
        return hasTree(treeId);
    });
    
    // Get last error
    btTable.set_function("get_last_error", [this]() -> std::string {
        return getLastError();
    });

    // Note: Async execution APIs are removed
    // Behavior trees are now executed through BTScript in EntityScriptManager
}

bool LuaBehaviorTreeBridge::loadBehaviorTreeFromFile(const std::string& xmlFile) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
    }
    
    try {
        factory_->registerBehaviorTreeFromFile(xmlFile);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML file: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::loadBehaviorTreeFromText(const std::string& xmlText) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
    }
    
    try {
        factory_->registerBehaviorTreeFromText(xmlText);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load XML text: ") + e.what();
        return false;
    }
}

std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }

    // Preload mode: check if tree definition is already loaded
    if (loadedTreeDefinitions_.count(treeName) == 0) {
        lastError_ = "Tree definition not preloaded: " + treeName +
                     ". Call bt.preload_all_trees() or bt.preload_trees_from_dir() first.";
        std::cerr << "[LuaBehaviorTreeBridge] " << lastError_ << std::endl;
        return "";
    }

    try {
        // Create blackboard
        auto blackboard = BT::Blackboard::create();
        
        // Set entity ID
        blackboard->set("entity_id", entityId);
        
        // Set additional parameters if provided
        if (params) {
            sol::table paramsTable = params.value();
            for (auto& pair : paramsTable) {
                std::string key = pair.first.as<std::string>();
                sol::object value = pair.second;

                if (value.is<std::string>()) {
                    blackboard->set(key, value.as<std::string>());
                } else if (value.is<bool>()) {
                    blackboard->set(key, value.as<bool>());
                } else if (value.is<int>()) {
                    blackboard->set(key, value.as<int>());
                } else if (value.is<double>()) {
                    double d = value.as<double>();
                    if (d == static_cast<int>(d)) {
                        blackboard->set(key, static_cast<int>(d));
                    } else {
                        blackboard->set(key, d);
                    }
                }
            }
        }

        // Create tree
        auto tree = factory_->createTree(treeName, blackboard);

        // Execute tree synchronously
        BT::NodeStatus status = tree.tickRoot();
        
        // Return status
        switch (status) {
            case BT::NodeStatus::SUCCESS: return "SUCCESS";
            case BT::NodeStatus::FAILURE: return "FAILURE";
            case BT::NodeStatus::RUNNING: return "RUNNING";
            case BT::NodeStatus::IDLE: return "IDLE";
            default: return "UNKNOWN";
        }

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute behavior tree: ") + e.what();
        std::cerr << "[LuaBehaviorTreeBridge] " << lastError_ << std::endl;
        return "";
    }
}

std::string LuaBehaviorTreeBridge::getTreeStatus(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        return "NOT_FOUND";
    }
    
    switch (it->second->lastStatus) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

bool LuaBehaviorTreeBridge::stopBehaviorTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }
    
    if (it->second->isRunning) {
        it->second->tree.haltTree();
        it->second->isRunning = false;
    }
    
    activeTrees_.erase(it);
    return true;
}

bool LuaBehaviorTreeBridge::setBlackboardValue(const std::string& treeId, 
                                                const std::string& key, 
                                                sol::object value) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        lastError_ = "Tree not found: " + treeId;
        return false;
    }
    
    try {
        auto blackboard = it->second->tree.rootBlackboard();
        if (!blackboard) {
            lastError_ = "Tree has no blackboard";
            return false;
        }
        
        if (value.is<std::string>()) {
            blackboard->set(key, value.as<std::string>());
        } else if (value.is<bool>()) {
            blackboard->set(key, value.as<bool>());
        } else if (value.is<int>()) {
            blackboard->set(key, value.as<int>());
        } else if (value.is<double>()) {
            blackboard->set(key, value.as<double>());
        } else {
            blackboard->set(key, luaObjectToString(value));
        }
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to set blackboard value: ") + e.what();
        return false;
    }
}

sol::object LuaBehaviorTreeBridge::getBlackboardValue(const std::string& treeId, 
                                                       const std::string& key) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    
    auto it = activeTrees_.find(treeId);
    if (it == activeTrees_.end()) {
        return sol::nil;
    }
    
    try {
        auto blackboard = it->second->tree.rootBlackboard();
        if (!blackboard) {
            return sol::nil;
        }
        
        auto anyVal = blackboard->getAny(key);
        if (!anyVal) {
            return sol::nil;
        }
        
        return blackboardEntryToLuaObject(*anyVal);
    } catch (const std::exception& e) {
        return sol::nil;
    }
}

bool LuaBehaviorTreeBridge::registerLuaAction(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid function";
        return false;
    }
    
    try {
        LuaActionNode::setLuaFunction(name, func);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to register action: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::registerLuaCondition(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid function";
        return false;
    }
    
    try {
        LuaConditionNode::setLuaFunction(name, func);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to register condition: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::registerLuaStatefulAction(const std::string& name,
                                                       sol::protected_function onStart,
                                                       sol::protected_function onRunning,
                                                       sol::protected_function onHalted) {
    if (!onStart.valid() || !onRunning.valid() || !onHalted.valid()) {
        lastError_ = "Invalid function(s)";
        return false;
    }
    
    try {
        LuaStatefulActionNode::setLuaFunctions(name, onStart, onRunning, onHalted);
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to register stateful action: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::hasTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    return activeTrees_.find(treeId) != activeTrees_.end();
}

std::string LuaBehaviorTreeBridge::generateTreeId() {
    return "bt_" + std::to_string(++treeIdCounter_);
}

std::string LuaBehaviorTreeBridge::luaObjectToString(sol::object obj) {
    if (obj.is<std::string>()) {
        return obj.as<std::string>();
    } else if (obj.is<bool>()) {
        return obj.as<bool>() ? "true" : "false";
    } else if (obj.is<int>()) {
        return std::to_string(obj.as<int>());
    } else if (obj.is<double>()) {
        return std::to_string(obj.as<double>());
    }
    return "";
}

sol::object LuaBehaviorTreeBridge::blackboardEntryToLuaObject(const BT::Any& entry) {
    try {
        if (entry.type() == typeid(std::string)) {
            return sol::make_object(*luaState_, entry.cast<std::string>());
        } else if (entry.type() == typeid(bool)) {
            return sol::make_object(*luaState_, entry.cast<bool>());
        } else if (entry.type() == typeid(int)) {
            return sol::make_object(*luaState_, entry.cast<int>());
        } else if (entry.type() == typeid(double)) {
            return sol::make_object(*luaState_, entry.cast<double>());
        }
    } catch (...) {
        // Conversion failed
    }
    return sol::nil;
}

bool LuaBehaviorTreeBridge::loadNodesRegistry(const std::string& registryPath) {
    try {
        // Load and execute the registry script
        sol::load_result script = luaState_->load_file(registryPath);
        if (!script.valid()) {
            // Registry file doesn't exist, which is OK
            return true;
        }
        
        auto result = script();
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[LuaBehaviorTreeBridge] Error loading nodes registry: " << err.what() << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        // Registry file might not exist, which is OK
        return true;
    }
}

bool LuaBehaviorTreeBridge::preloadBehaviorTreesFromDirectory(const std::string& directory) {
    try {
        // Check if directory exists
        std::string searchPath = directory + "*.xml";
        
#ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            return true;  // Directory doesn't exist or is empty
        }
        
        do {
            std::string fileName = findData.cFileName;
            std::string filePath = directory + fileName;
            
            try {
                factory_->registerBehaviorTreeFromFile(filePath);
                
                // Extract tree name from file name (without extension)
                size_t lastDot = fileName.find_last_of('.');
                std::string treeName = (lastDot != std::string::npos) ? 
                                       fileName.substr(0, lastDot) : fileName;
                loadedTreeDefinitions_.insert(treeName);
                
                std::cout << "[LuaBehaviorTreeBridge] Preloaded behavior tree: " << treeName << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[LuaBehaviorTreeBridge] Failed to preload " << filePath << ": " << e.what() << std::endl;
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
#else
        DIR* dir = opendir(directory.c_str());
        if (!dir) {
            return true;  // Directory doesn't exist
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string fileName = entry->d_name;
            
            // Check if it's an XML file
            if (fileName.length() > 4 && 
                fileName.substr(fileName.length() - 4) == ".xml") {
                std::string filePath = directory + fileName;
                
                try {
                    factory_->registerBehaviorTreeFromFile(filePath);
                    
                    // Extract tree name from file name (without extension)
                    size_t lastDot = fileName.find_last_of('.');
                    std::string treeName = (lastDot != std::string::npos) ? 
                                           fileName.substr(0, lastDot) : fileName;
                    loadedTreeDefinitions_.insert(treeName);
                    
                    std::cout << "[LuaBehaviorTreeBridge] Preloaded behavior tree: " << treeName << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "[LuaBehaviorTreeBridge] Failed to preload " << filePath << ": " << e.what() << std::endl;
                }
            }
        }
        
        closedir(dir);
#endif
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LuaBehaviorTreeBridge] Error preloading behavior trees: " << e.what() << std::endl;
        return false;
    }
}

} // namespace scripting
