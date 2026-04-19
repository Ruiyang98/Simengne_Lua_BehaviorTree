#include "scripting/LuaSimBinding.h"
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

} // namespace scripting
