#include "scripting/LuaBehaviorTreeBridge.h"
#include "behaviortree/BehaviorTreeScheduler.h"
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
LuaBehaviorTreeBridge::LuaBehaviorTreeBridge(sol::state* luaState, BT::BehaviorTreeFactory* factory)
    : luaState_(luaState)
    , factory_(factory)
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

    // ==================== Async Execution API ====================

    // Execute behavior tree asynchronously
    // entityId is now required (not optional)
    btTable.set_function("execute_async", [this](const std::string& treeName,
                                                  const std::string& entityId,
                                                  sol::optional<sol::table> params) -> bool {
        return executeAsync(treeName, entityId, params);
    });

    // Stop async behavior tree by entityId
    btTable.set_function("stop_async", [this](const std::string& entityId) -> bool {
        return stopAsync(entityId);
    });

    // Get async behavior tree status by entityId
    btTable.set_function("get_async_status", [this](const std::string& entityId) -> std::string {
        return getAsyncStatus(entityId);
    });

    // Get all registered async entity IDs
    btTable.set_function("get_async_entities", [this]() -> sol::table {
        return getAsyncEntities();
    });

    // Set complete callback
    btTable.set_function("set_complete_callback", [this](const std::string& entityId,
                                                          sol::protected_function callback) -> bool {
        return setCompleteCallback(entityId, callback);
    });

    // Set tick callback
    btTable.set_function("set_tick_callback", [this](const std::string& entityId,
                                                      sol::protected_function callback) -> bool {
        return setTickCallback(entityId, callback);
    });
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
        
        // Generate tree ID
        std::string treeId = generateTreeId();
        
        // Store tree info
        auto info = std::make_shared<TreeExecutionInfo>();
        info->treeId = treeId;
        info->treeName = treeName;
        info->entityId = entityId;
        info->tree = std::move(tree);
        info->isRunning = true;
        info->startTime = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(treesMutex_);
            activeTrees_[treeId] = info;
        }
        
        // Execute tree (synchronously for now)
        info->lastStatus = info->tree.tickRoot();
        info->isRunning = (info->lastStatus == BT::NodeStatus::RUNNING);
        
        return treeId;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute behavior tree: ") + e.what();
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
    
    it->second->tree.haltTree();
    it->second->isRunning = false;
    it->second->lastStatus = BT::NodeStatus::IDLE;
    
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
            lastError_ = "Blackboard is null";
            return false;
        }
        
        if (value.is<std::string>()) {
            blackboard->set(key, value.as<std::string>());
        } else if (value.is<double>()) {
            blackboard->set(key, value.as<double>());
        } else if (value.is<bool>()) {
            blackboard->set(key, value.as<bool>());
        } else if (value.is<int>()) {
            blackboard->set(key, value.as<int>());
        } else {
            lastError_ = "Unsupported value type";
            return false;
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
        lastError_ = "Tree not found: " + treeId;
        return sol::nil;
    }
    
    try {
        auto blackboard = it->second->tree.rootBlackboard();
        if (!blackboard) {
            lastError_ = "Blackboard is null";
            return sol::nil;
        }
        
        if (!blackboard->getAny(key)) {
            return sol::nil;
        }
        
        // Try to get as different types
        try {
            return sol::make_object(*luaState_, blackboard->get<std::string>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<double>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<bool>(key));
        } catch (...) {}
        
        try {
            return sol::make_object(*luaState_, blackboard->get<int>(key));
        } catch (...) {}
        
        return sol::nil;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to get blackboard value: ") + e.what();
        return sol::nil;
    }
}

bool LuaBehaviorTreeBridge::registerLuaAction(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid Lua function";
        return false;
    }
    
    LuaActionNode::setLuaFunction(name, func);
    return true;
}

bool LuaBehaviorTreeBridge::registerLuaCondition(const std::string& name, sol::protected_function func) {
    if (!func.valid()) {
        lastError_ = "Invalid Lua function";
        return false;
    }
    
    LuaConditionNode::setLuaFunction(name, func);
    return true;
}

bool LuaBehaviorTreeBridge::registerLuaStatefulAction(const std::string& name,
                                                      sol::protected_function onStart,
                                                      sol::protected_function onRunning,
                                                      sol::protected_function onHalted) {
    if (!onStart.valid() || !onRunning.valid()) {
        lastError_ = "Invalid Lua function (onStart and onRunning are required)";
        return false;
    }
    
    LuaStatefulActionNode::setLuaFunctions(name, onStart, onRunning, onHalted);
    return true;
}

bool LuaBehaviorTreeBridge::hasTree(const std::string& treeId) {
    std::lock_guard<std::mutex> lock(treesMutex_);
    return activeTrees_.find(treeId) != activeTrees_.end();
}

std::string LuaBehaviorTreeBridge::generateTreeId() {
    std::stringstream ss;
    ss << "bt_" << ++treeIdCounter_;
    return ss.str();
}

// ==================== Async Execution API ====================

bool LuaBehaviorTreeBridge::executeAsync(const std::string& treeName,
                                          const std::string& entityId,
                                          sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return false;
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

        // Get global scheduler instance and register entity
        auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
        bool success = scheduler.registerEntityWithTree(entityId, treeName, std::move(tree), blackboard);

        return success;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to execute async behavior tree: ") + e.what();
        return false;
    }
}

bool LuaBehaviorTreeBridge::stopAsync(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    scheduler.unregisterEntity(entityId);
    return true;
}

std::string LuaBehaviorTreeBridge::getAsyncStatus(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    BT::NodeStatus status = scheduler.getEntityStatus(entityId);
    
    switch (status) {
        case BT::NodeStatus::SUCCESS: return "SUCCESS";
        case BT::NodeStatus::FAILURE: return "FAILURE";
        case BT::NodeStatus::RUNNING: return "RUNNING";
        case BT::NodeStatus::IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

sol::table LuaBehaviorTreeBridge::getAsyncEntities() {
    sol::table result = luaState_->create_table();

    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    std::vector<std::string> entityIds = scheduler.getRegisteredEntityIds();

    int index = 1;
    for (const auto& entityId : entityIds) {
        result[index++] = entityId;
    }

    return result;
}

bool LuaBehaviorTreeBridge::setCompleteCallback(const std::string& entityId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[entityId].onComplete = callback;
    return true;
}

bool LuaBehaviorTreeBridge::setTickCallback(const std::string& entityId, sol::protected_function callback) {
    if (!callback.valid()) {
        lastError_ = "Invalid callback function";
        return false;
    }

    std::lock_guard<std::mutex> lock(callbacksMutex_);
    asyncCallbacks_[entityId].onTick = callback;
    return true;
}

// ==================== Preload Implementation ====================

bool LuaBehaviorTreeBridge::loadNodesRegistry(const std::string& registryPath) {
    try {
        // Load and execute the registry script
        sol::load_result script = luaState_->load_file(registryPath);
        if (!script.valid()) {
            sol::error err = script;
            lastError_ = std::string("Failed to load registry script: ") + err.what();
            return false;
        }

        sol::protected_function_result result = script();
        if (!result.valid()) {
            sol::error err = result;
            lastError_ = std::string("Failed to execute registry script: ") + err.what();
            return false;
        }

        std::cout << "[LuaBehaviorTreeBridge] Loaded nodes registry from: " << registryPath << std::endl;
        return true;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load nodes registry: ") + e.what();
        return false;
    }
}

// Helper function to scan a single XML file for BehaviorTree definitions and load them
static void scanAndLoadXmlFile(const std::string& filePath,
                                std::unordered_set<std::string>& loadedTrees,
                                BT::BehaviorTreeFactory* factory,
                                int& successCount,
                                int& failCount) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "  [FAIL] Cannot open file: " << filePath << std::endl;
        failCount++;
        return;
    }

    // Read entire file content to parse all BehaviorTree definitions
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // Find all BehaviorTree definitions in the file
    size_t pos = 0;
    bool anyLoaded = false;
    while ((pos = content.find("<BehaviorTree", pos)) != std::string::npos) {
        size_t idPos = content.find("ID=\"", pos);
        if (idPos != std::string::npos) {
            size_t start = idPos + 4;
            size_t end = content.find("\"", start);
            if (end != std::string::npos) {
                std::string treeId = content.substr(start, end - start);

                // Skip if already loaded
                if (loadedTrees.count(treeId) > 0) {
                    pos = end;
                    continue;
                }

                anyLoaded = true;
            }
        }
        pos++;
    }

    // If file contains any unloaded trees, load the entire file
    if (anyLoaded) {
        try {
            factory->registerBehaviorTreeFromFile(filePath);

            // Re-parse to record loaded tree names
            pos = 0;
            while ((pos = content.find("<BehaviorTree", pos)) != std::string::npos) {
                size_t idPos = content.find("ID=\"", pos);
                if (idPos != std::string::npos) {
                    size_t start = idPos + 4;
                    size_t end = content.find("\"", start);
                    if (end != std::string::npos) {
                        std::string treeId = content.substr(start, end - start);
                        loadedTrees.insert(treeId);
                        std::cout << "  [OK] Preloaded: " << treeId << std::endl;
                        successCount++;
                    }
                }
                pos++;
            }
        } catch (const std::exception& e) {
            std::cerr << "  [FAIL] Failed to load file: " << filePath
                      << " (" << e.what() << ")" << std::endl;
            failCount++;
        }
    }
}

// Helper function to recursively scan directory for XML files and load them
static void scanAndLoadDirectoryRecursive(const std::string& directory,
                                           std::unordered_set<std::string>& loadedTrees,
                                           BT::BehaviorTreeFactory* factory,
                                           int& successCount,
                                           int& failCount) {
#ifdef _WIN32
    std::string searchPath = directory + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        std::string name = findData.cFileName;

        // Skip . and ..
        if (name == "." || name == "..") continue;

        std::string fullPath = directory + "\\" + name;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recurse into subdirectory
            scanAndLoadDirectoryRecursive(fullPath, loadedTrees, factory, successCount, failCount);
        } else {
            // Check if it's an XML file
            if (name.size() > 4 && name.substr(name.size() - 4) == ".xml") {
                scanAndLoadXmlFile(fullPath, loadedTrees, factory, successCount, failCount);
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
#else
    // Unix/Linux implementation using dirent.h
    DIR* dir = opendir(directory.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;

        // Skip . and ..
        if (name == "." || name == "..") continue;

        std::string fullPath = directory + "/" + name;

        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // Recurse into subdirectory
                scanAndLoadDirectoryRecursive(fullPath, loadedTrees, factory, successCount, failCount);
            } else if (S_ISREG(statbuf.st_mode)) {
                // Check if it's an XML file
                if (name.size() > 4 && name.substr(name.size() - 4) == ".xml") {
                    scanAndLoadXmlFile(fullPath, loadedTrees, factory, successCount, failCount);
                }
            }
        }
    }

    closedir(dir);
#endif
}

bool LuaBehaviorTreeBridge::preloadBehaviorTreesFromDirectory(const std::string& directory) {
    try {
        // Check if directory exists using platform-specific method
#ifdef _WIN32
        DWORD attribs = GetFileAttributesA(directory.c_str());
        if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY)) {
            lastError_ = "Directory does not exist or is not accessible: " + directory;
            return false;
        }
#else
        struct stat statbuf;
        if (stat(directory.c_str(), &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
            lastError_ = "Directory does not exist or is not accessible: " + directory;
            return false;
        }
#endif

        int successCount = 0;
        int failCount = 0;

        std::cout << "[LuaBehaviorTreeBridge] Preloading behavior trees from: " << directory << std::endl;

        scanAndLoadDirectoryRecursive(directory, loadedTreeDefinitions_, factory_, successCount, failCount);

        std::cout << "[LuaBehaviorTreeBridge] Preloaded " << successCount
                  << " behavior trees";
        if (failCount > 0) {
            std::cout << " (" << failCount << " failed)";
        }
        std::cout << std::endl;

        return failCount == 0;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to preload behavior trees: ") + e.what();
        return false;
    }
}

} // namespace scripting
