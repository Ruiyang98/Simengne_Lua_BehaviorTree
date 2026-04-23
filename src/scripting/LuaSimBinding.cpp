#include "scripting/LuaSimBinding.h"
#include "scripting/LuaBehaviorTreeBridge.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

namespace scripting {

LuaSimBinding::LuaSimBinding(simulation::SimControlInterface* simInterface)
    : simInterface_(simInterface)
    , initialized_(false) {
}

LuaSimBinding::~LuaSimBinding() {
    // Clear callbacks to prevent dangling references
    if (simInterface_) {
        simInterface_->setOnStartCallback(nullptr);
        simInterface_->setOnPauseCallback(nullptr);
        simInterface_->setOnResumeCallback(nullptr);
        simInterface_->setOnStopCallback(nullptr);
        simInterface_->setOnResetCallback(nullptr);
    }
    // Clear Lua callbacks before destroying Lua state
    luaCallbacks_.clear();
}

bool LuaSimBinding::initialize() {
    if (!simInterface_) {
        lastError_ = "SimControlInterface pointer is null";
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
    registerUtilityFunctions();
}

void LuaSimBinding::registerSimAPI() {
    // Create sim table
    sol::table simTable = luaState_->create_named_table("sim");

    // Control commands
    simTable.set_function("start", [this]() -> bool {
        return simInterface_ ? simInterface_->start() : false;
    });

    simTable.set_function("pause", [this]() -> bool {
        return simInterface_ ? simInterface_->pause() : false;
    });

    simTable.set_function("resume", [this]() -> bool {
        return simInterface_ ? simInterface_->resume() : false;
    });

    simTable.set_function("stop", [this]() -> bool {
        return simInterface_ ? simInterface_->stop() : false;
    });

    simTable.set_function("reset", [this]() -> bool {
        return simInterface_ ? simInterface_->reset() : false;
    });

    // State queries
    simTable.set_function("get_state", [this]() -> std::string {
        if (simInterface_) {
            return simulation::SimControlInterface::stateToString(simInterface_->getState());
        }
        return "UNKNOWN";
    });

    simTable.set_function("is_running", [this]() -> bool {
        return simInterface_ ? simInterface_->isRunning() : false;
    });

    simTable.set_function("is_paused", [this]() -> bool {
        return simInterface_ ? simInterface_->isPaused() : false;
    });

    simTable.set_function("is_stopped", [this]() -> bool {
        return simInterface_ ? simInterface_->isStopped() : false;
    });

    simTable.set_function("get_time", [this]() -> double {
        return simInterface_ ? simInterface_->getSimTime() : 0.0;
    });

    simTable.set_function("get_time_step", [this]() -> double {
        return simInterface_ ? simInterface_->getTimeStep() : 0.0;
    });

    // Speed control
    simTable.set_function("set_speed", [this](double scale) {
        if (simInterface_) {
            simInterface_->setTimeScale(scale);
        }
    });

    simTable.set_function("get_speed", [this]() -> double {
        return simInterface_ ? simInterface_->getTimeScale() : 1.0;
    });

    // Entity management
    simTable.set_function("add_entity", [this](const std::string& type, double x, double y, double z) -> std::string {
        if (simInterface_) {
            return simInterface_->addEntity(type, x, y, z);
        }
        return "";
    });

    simTable.set_function("remove_entity", [this](const std::string& entityId) -> bool {
        if (simInterface_) {
            return simInterface_->removeEntity(entityId);
        }
        return false;
    });

    simTable.set_function("move_entity", [this](const std::string& entityId, double x, double y, double z) -> bool {
        if (simInterface_) {
            return simInterface_->moveEntity(entityId, x, y, z);
        }
        return false;
    });

    simTable.set_function("get_entity_position", [this](const std::string& entityId) -> sol::optional<sol::table> {
        if (!simInterface_) {
            return sol::nullopt;
        }
        
        double x, y, z;
        if (simInterface_->getEntityPosition(entityId, x, y, z)) {
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
        
        if (simInterface_) {
            auto entityList = simInterface_->getAllEntities();
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

    simTable.set_function("get_entity_count", [this]() -> int {
        if (simInterface_) {
            return static_cast<int>(simInterface_->getEntityCount());
        }
        return 0;
    });

    // Set entity move direction
    simTable.set_function("set_entity_move_direction", [this](const std::string& entityId, double dx, double dy, double dz) -> bool {
        if (simInterface_) {
            return simInterface_->setEntityMoveDirection(entityId, dx, dy, dz);
        }
        return false;
    });

    // Get entity distance to target point
    simTable.set_function("get_entity_distance", [this](const std::string& entityId, double x, double y, double z) -> double {
        if (simInterface_) {
            return simInterface_->getEntityDistance(entityId, x, y, z);
        }
        return -1.0;
    });

    // Event callbacks
    simTable.set_function("on_start", [this](sol::function callback) {
        if (simInterface_ && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            simInterface_->setOnStartCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_start callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_pause", [this](sol::function callback) {
        if (simInterface_ && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            simInterface_->setOnPauseCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_pause callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_resume", [this](sol::function callback) {
        if (simInterface_ && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            simInterface_->setOnResumeCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_resume callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_stop", [this](sol::function callback) {
        if (simInterface_ && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            simInterface_->setOnStopCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_stop callback error: " << err.what() << std::endl;
                }
            });
        }
    });

    simTable.set_function("on_reset", [this](sol::function callback) {
        if (simInterface_ && callback.valid()) {
            sol::protected_function protectedCallback = callback;
            luaCallbacks_.push_back(protectedCallback);
            
            simInterface_->setOnResetCallback([protectedCallback]() {
                auto result = protectedCallback();
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "on_reset callback error: " << err.what() << std::endl;
                }
            });
        }
    });
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
