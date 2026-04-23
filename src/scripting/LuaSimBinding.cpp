#include "scripting/LuaSimBinding.h"
#include "scripting/LuaBehaviorTreeBridge.h"
#include "scripting/EntityScriptManager.h"
#include "simulation/MockSimController.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

namespace scripting {

// Singleton instance getter
LuaSimBinding& LuaSimBinding::getInstance() {
    static LuaSimBinding instance;
    return instance;
}

LuaSimBinding::LuaSimBinding()
    : initialized_(false) {
}

LuaSimBinding::~LuaSimBinding() {
    // Clear callbacks to prevent dangling references
    SimControlInterface* simInterface = SimControlInterface::getInstance();
    if (simInterface) {
        simInterface->setOnStartCallback(nullptr);
        simInterface->setOnPauseCallback(nullptr);
        simInterface->setOnResumeCallback(nullptr);
        simInterface->setOnStopCallback(nullptr);
        simInterface->setOnResetCallback(nullptr);
    }
    // Clear Lua callbacks before destroying Lua state
    luaCallbacks_.clear();
}

bool LuaSimBinding::initialize(BT::BehaviorTreeFactory* factory) {
    if (initialized_) {
        return true;  // Already initialized, return success
    }
    
    if (!SimControlInterface::getInstance()) {
        lastError_ = "SimControlInterface instance is null";
        return false;
    }

    try {
        luaState_ = std::make_unique<sol::state>();
        
        luaState_->open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::os,
            sol::lib::math,
            sol::lib::table,
            sol::lib::debug,
            sol::lib::bit32,
            sol::lib::io,
            sol::lib::ffi
        );

        registerFunctions();
        setupCallbacks();

        initialized_ = true;
        
        // If factory provided, auto-initialize behavior tree bridge
        if (factory) {
            return initializeBehaviorTree(factory);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Initialization failed: ") + e.what();
        return false;
    }
}

bool LuaSimBinding::executeScript(const std::string& scriptPath) {
    if (!initialized_) {
        lastError_ = "Lua environment not initialized";
        return false;
    }

    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        lastError_ = "Cannot open script file: " + scriptPath;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return executeString(buffer.str());
}

bool LuaSimBinding::executeString(const std::string& scriptCode) {
    if (!initialized_) {
        lastError_ = "Lua environment not initialized";
        return false;
    }

    try {
        // Use script() with exception handling for older sol2 versions
        luaState_->script(scriptCode);
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Execution exception: ") + e.what();
        return false;
    }
}

sol::state& LuaSimBinding::getState() {
    return *luaState_;
}

bool LuaSimBinding::isInitialized() const {
    return initialized_;
}

const std::string& LuaSimBinding::getLastError() const {
    return lastError_;
}

void LuaSimBinding::registerFunctions() {
    registerSimAPI();
    registerBehaviorTreeAPI();
    registerUtilityFunctions();
}

void LuaSimBinding::registerSimAPI() {
    SimControlInterface* simInterface = SimControlInterface::getInstance();

    // Register SimAddress type
    luaState_->new_usertype<SimAddress>("SimAddress",
        "site", &SimAddress::site,
        "host", &SimAddress::host
    );

    // Register VehicleID type
    luaState_->new_usertype<VehicleID>("VehicleID",
        "address", &VehicleID::address,
        "vehicle", &VehicleID::vehicle
    );

    // Create sim table
    sol::table simTable = luaState_->create_named_table("sim");

    // Control commands
    simTable.set_function("start", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->start() : false;
    });

    simTable.set_function("pause", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->pause() : false;
    });

    simTable.set_function("resume", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->resume() : false;
    });

    simTable.set_function("stop", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->stop() : false;
    });

    simTable.set_function("reset", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->reset() : false;
    });

    // State queries
    simTable.set_function("get_state", []() -> std::string {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return SimControlInterface::stateToString(sim->getState());
        }
        return "UNKNOWN";
    });

    simTable.set_function("is_running", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->isRunning() : false;
    });

    simTable.set_function("is_paused", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->isPaused() : false;
    });

    simTable.set_function("is_stopped", []() -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->isStopped() : false;
    });

    simTable.set_function("get_time", []() -> double {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->getSimTime() : 0.0;
    });

    simTable.set_function("get_time_step", []() -> double {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->getTimeStep() : 0.0;
    });

    // Speed control
    simTable.set_function("set_speed", [](double scale) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            sim->setTimeScale(scale);
        }
    });

    simTable.set_function("get_speed", []() -> double {
        SimControlInterface* sim = SimControlInterface::getInstance();
        return sim ? sim->getTimeScale() : 1.0;
    });

    // Entity management
    simTable.set_function("add_entity", [](const std::string& type, double x, double y, double z) -> VehicleID {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return sim->addEntity(type, x, y, z);
        }
        return VehicleID{};
    });

    simTable.set_function("remove_entity", [](const VehicleID& vehicleId) -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return sim->removeEntity(vehicleId);
        }
        return false;
    });

    simTable.set_function("move_entity", [](const VehicleID& vehicleId, double x, double y, double z) -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return sim->moveEntity(vehicleId, x, y, z);
        }
        return false;
    });

    simTable.set_function("get_entity_position", [this](const VehicleID& vehicleId) -> sol::optional<sol::table> {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (!sim) {
            return sol::nullopt;
        }

        double x, y, z;
        if (sim->getEntityPosition(vehicleId, x, y, z)) {
            sol::table pos = luaState_->create_table();
            pos["x"] = x;
            pos["y"] = y;
            pos["z"] = z;
            return pos;
        }
        return sol::nullopt;
    });

    simTable.set_function("get_all_entities", [this]() -> sol::table {
        sol::table entities = luaState_->create_table();

        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            auto entityList = sim->getAllEntities();
            for (size_t i = 0; i < entityList.size(); ++i) {
                sol::table entity = luaState_->create_table();
                entity["id"] = entityList[i].id;
                entity["type"] = entityList[i].type;
                entity["x"] = entityList[i].x;
                entity["y"] = entityList[i].y;
                entity["z"] = entityList[i].z;
                entities[i + 1] = entity;  // Lua arrays are 1-indexed
            }
        }

        return entities;
    });

    simTable.set_function("get_entity_count", []() -> int {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return static_cast<int>(sim->getEntityCount());
        }
        return 0;
    });

    // Set entity move direction
    simTable.set_function("set_entity_move_direction", [](const VehicleID& vehicleId, double dx, double dy, double dz) -> bool {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return sim->setEntityMoveDirection(vehicleId, dx, dy, dz);
        }
        return false;
    });

    // Get entity distance to target point
    simTable.set_function("get_entity_distance", [](const VehicleID& vehicleId, double x, double y, double z) -> double {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim) {
            return sim->getEntityDistance(vehicleId, x, y, z);
        }
        return -1.0;
    });

    // Event callbacks
    simTable.set_function("on_start", [this](sol::function callback) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            sim->setOnStartCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_start callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_pause", [this](sol::function callback) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            sim->setOnPauseCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_pause callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_resume", [this](sol::function callback) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            sim->setOnResumeCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_resume callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_stop", [this](sol::function callback) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            sim->setOnStopCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_stop callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_reset", [this](sol::function callback) {
        SimControlInterface* sim = SimControlInterface::getInstance();
        if (sim && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            sim->setOnResetCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_reset callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    // Note: Script manager APIs are removed from Lua layer
    // Script management is now handled entirely in C++ layer
    // Entities automatically get script managers when created via add_entity
}

void LuaSimBinding::registerBehaviorTreeAPI() {
    // Behavior tree API will be registered in initializeBehaviorTree via btBridge_
    // Reserved interface, actual registration done in LuaBehaviorTreeBridge
}

void LuaSimBinding::registerUtilityFunctions() {
    // Sleep function
    luaState_->set_function("sleep", [](double seconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
    });
}

void LuaSimBinding::setupCallbacks() {
    // Callbacks are set up in registerSimAPI
}

bool LuaSimBinding::initializeBehaviorTree(BT::BehaviorTreeFactory* factory) {
    if (!initialized_) {
        lastError_ = "Lua environment not initialized";
        return false;
    }
    
    if (!factory) {
        lastError_ = "BT factory is null";
        return false;
    }
    
    try {
        btBridge_ = std::make_unique<LuaBehaviorTreeBridge>(luaState_.get(), factory);
        return btBridge_->initialize();
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to initialize BT bridge: ") + e.what();
        return false;
    }
}

} // namespace scripting
