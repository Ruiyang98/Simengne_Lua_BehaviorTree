#include "simulation/MockSimController.h"
#include "scripting/EntityScriptManager.h"
#include "scripting/LuaSimBinding.h"
#include <cmath>
#include <fstream>
#include <sstream>


MockSimController::MockSimController()
    : state_(SimState::STOPPED)
    , simTime_(0.0)
    , timeScale_(1.0)
    , timeStep_(0.016)
    , running_(false)
    , paused_(false)
    , verbose_(true)
    , nextVehicleId_(1) {
}

MockSimController::~MockSimController() {
    stop();
}

bool MockSimController::start() {
    if (state_ == SimState::RUNNING) {
        if (verbose_) std::cout << "[MockSim] Already running" << std::endl;
        return false;
    }

    state_ = SimState::RUNNING;
    running_ = true;
    paused_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation started" << std::endl;
    
    if (onStartCallback_) {
        onStartCallback_();
    }

    return true;
}

bool MockSimController::pause() {
    if (state_ != SimState::RUNNING) {
        if (verbose_) std::cout << "[MockSim] Cannot pause: not running" << std::endl;
        return false;
    }

    state_ = SimState::PAUSED;
    paused_ = true;
    if (verbose_) std::cout << "[MockSim] Simulation paused, time: " << simTime_ << "s" << std::endl;
    
    if (onPauseCallback_) {
        onPauseCallback_();
    }
    return true;
}

bool MockSimController::resume() {
    if (state_ != SimState::PAUSED) {
        if (verbose_) std::cout << "[MockSim] Cannot resume: not paused" << std::endl;
        return false;
    }

    state_ = SimState::RUNNING;
    paused_ = false;
    if (verbose_) std::cout << "[MockSim] Simulation resumed" << std::endl;
    
    if (onResumeCallback_) {
        onResumeCallback_();
    }
    return true;
}

bool MockSimController::stop() {
    if (state_ == SimState::STOPPED) {
        return false;
    }

    state_ = SimState::STOPPED;
    running_ = false;
    paused_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation stopped, final time: " << simTime_ << "s" << std::endl;
    
    if (onStopCallback_) {
        onStopCallback_();
    }
    return true;
}

bool MockSimController::reset() {
    bool wasRunning = (state_ == SimState::RUNNING);
    
    state_ = SimState::STOPPED;
    simTime_ = 0.0;
    running_ = false;
    paused_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation reset" << std::endl;
    
    if (onResetCallback_) {
        onResetCallback_();
    }
    
    if (wasRunning) {
        start();
    }
    
    return true;
}

SimState MockSimController::getState() const {
    return state_;
}

bool MockSimController::isRunning() const {
    return state_ == SimState::RUNNING;
}

bool MockSimController::isPaused() const {
    return state_ == SimState::PAUSED;
}

bool MockSimController::isStopped() const {
    return state_ == SimState::STOPPED;
}

double MockSimController::getSimTime() const {
    return simTime_;
}

double MockSimController::getTimeStep() const {
    return timeStep_;
}

void MockSimController::setTimeScale(double scale) {
    if (scale < 0.0) scale = 0.0;
    timeScale_ = scale;
    if (verbose_) std::cout << "[MockSim] Time scale set to: " << scale << "x" << std::endl;
}

double MockSimController::getTimeScale() const {
    return timeScale_;
}

void MockSimController::setOnStartCallback(std::function<void()> callback) {
    onStartCallback_ = callback;
}

void MockSimController::setOnPauseCallback(std::function<void()> callback) {
    onPauseCallback_ = callback;
}

void MockSimController::setOnResumeCallback(std::function<void()> callback) {
    onResumeCallback_ = callback;
}

void MockSimController::setOnStopCallback(std::function<void()> callback) {
    onStopCallback_ = callback;
}

void MockSimController::setOnResetCallback(std::function<void()> callback) {
    onResetCallback_ = callback;
}

void MockSimController::setVerbose(bool verbose) {
    verbose_ = verbose;
}

bool MockSimController::isVerbose() const {
    return verbose_;
}

// setBehaviorTreeFactory removed - factory is now obtained from BehaviorTreeExecutor singleton

VehicleID MockSimController::generateVehicleId() {
    VehicleID id;
    id.address.site = 0;
    id.address.host = 0;
    id.vehicle = nextVehicleId_++;
    return id;
}

VehicleID MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    VehicleID vehicleId = generateVehicleId();
    entities_[vehicleId] = EntityExt(vehicleId, type, x, y, z);
    
    // Auto-create script manager for this entity
    std::string entityId = std::to_string(vehicleId.vehicle);
    createScriptManager(entityId);
    
    if (verbose_) {
        std::cout << "[MockSim] Entity added: vehicle=" << vehicleId.vehicle
                  << " (type: " << type << ") at ("
                  << x << ", " << y << ", " << z << ")" << std::endl;
    }

    return vehicleId;
}

bool MockSimController::removeEntity(const VehicleID& vehicleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entities_.find(vehicleId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to remove entity: vehicle=" << vehicleId.vehicle << " (not found)" << std::endl;
        }
        return false;
    }

    entities_.erase(it);
    
    // Also remove script manager
    std::string entityId = std::to_string(vehicleId.vehicle);
    removeScriptManager(entityId);

    if (verbose_) {
        std::cout << "[MockSim] Entity removed: vehicle=" << vehicleId.vehicle << std::endl;
    }

    return true;
}

bool MockSimController::moveEntity(const VehicleID& vehicleId, double x, double y, double z) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entities_.find(vehicleId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to move entity: vehicle=" << vehicleId.vehicle << " (not found)" << std::endl;
        }
        return false;
    }

    it->second.x = x;
    it->second.y = y;
    it->second.z = z;

    if (verbose_) {
        std::cout << "[MockSim] Entity moved: vehicle=" << vehicleId.vehicle << " to (" << x << ", " << y << ", " << z << ")" << std::endl;
    }

    return true;
}

bool MockSimController::getEntityPosition(const VehicleID& vehicleId, double& x, double& y, double& z) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entities_.find(vehicleId);
    if (it == entities_.end()) {
        return false;
    }

    x = it->second.x;
    y = it->second.y;
    z = it->second.z;

    return true;
}

std::vector<Entity> MockSimController::getAllEntities() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Entity> result;
    result.reserve(entities_.size());
    
    for (const auto& pair : entities_) {
        result.push_back(pair.second);
    }
    
    return result;
}

size_t MockSimController::getEntityCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return entities_.size();
}

bool MockSimController::setEntityMoveDirection(const VehicleID& vehicleId, double dx, double dy, double dz) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entities_.find(vehicleId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to set move direction for entity: vehicle=" << vehicleId.vehicle << " (not found)" << std::endl;
        }
        return false;
    }

    // Normalize direction vector
    double length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length > 0) {
        dx /= length;
        dy /= length;
        dz /= length;
    }

    it->second.dx = dx;
    it->second.dy = dy;
    it->second.dz = dz;

    if (verbose_) {
        std::cout << "[MockSim] Entity vehicle=" << vehicleId.vehicle << " direction set to (" << dx << ", " << dy << ", " << dz << ")" << std::endl;
    }

    return true;
}

double MockSimController::getEntityDistance(const VehicleID& vehicleId, double x, double y, double z) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entities_.find(vehicleId);
    if (it == entities_.end()) {
        return -1.0;
    }

    double dx = it->second.x - x;
    double dy = it->second.y - y;
    double dz = it->second.z - z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::shared_ptr<scripting::EntityScriptManager> MockSimController::createScriptManager(const std::string& entityId) {
    if (entityScriptManagers_.find(entityId) != entityScriptManagers_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Script manager already exists for entity: " << entityId << std::endl;
        }
        return entityScriptManagers_[entityId];
    }
    
    // Get global Lua state from singleton
    scripting::LuaSimBinding& luaBinding = scripting::LuaSimBinding::getInstance();
    if (!luaBinding.isInitialized()) {
        if (verbose_) {
            std::cout << "[MockSim] Lua binding not initialized, initializing now..." << std::endl;
        }
        // Initialize without factory parameter - factory is obtained from BehaviorTreeExecutor singleton
        if (!luaBinding.initialize()) {
            std::cerr << "[MockSim] Failed to initialize Lua binding" << std::endl;
            return nullptr;
        }
    }
    
    // Create EntityScriptManager - dependencies are obtained from singletons internally
    auto manager = std::make_shared<scripting::EntityScriptManager>(entityId);
    entityScriptManagers_[entityId] = manager;
    
    if (verbose_) {
        std::cout << "[MockSim] Created script manager for entity: " << entityId << std::endl;
    }
    
    return manager;
}

bool MockSimController::removeScriptManager(const std::string& entityId) {
    auto it = entityScriptManagers_.find(entityId);
    if (it == entityScriptManagers_.end()) {
        return false;
    }
    
    entityScriptManagers_.erase(it);
    
    if (verbose_) {
        std::cout << "[MockSim] Removed script manager for entity: " << entityId << std::endl;
    }
    
    return true;
}

std::shared_ptr<scripting::EntityScriptManager> MockSimController::getScriptManager(const std::string& entityId) const {
    auto it = entityScriptManagers_.find(entityId);
    if (it != entityScriptManagers_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MockSimController::hasScriptManager(const std::string& entityId) const {
    return entityScriptManagers_.find(entityId) != entityScriptManagers_.end();
}

bool MockSimController::addScriptToEntity(const std::string& entityId, 
                                           const std::string& scriptName,
                                           const std::string& scriptCode) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        std::cerr << "[MockSim] No script manager for entity: " << entityId << std::endl;
        return false;
    }
    
    return manager->addTacticalScript(scriptName, scriptCode);
}

bool MockSimController::addScriptToEntityFromFile(const std::string& entityId,
                                                   const std::string& scriptName,
                                                   const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MockSim] Cannot open script file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return addScriptToEntity(entityId, scriptName, buffer.str());
}

bool MockSimController::removeScriptFromEntity(const std::string& entityId,
                                                const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->removeScript(scriptName);
}

bool MockSimController::enableEntityScript(const std::string& entityId,
                                            const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->enableScript(scriptName);
}

bool MockSimController::disableEntityScript(const std::string& entityId,
                                             const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->disableScript(scriptName);
}

std::vector<std::string> MockSimController::getEntityScriptNames(const std::string& entityId) const {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return {};
    }
    
    return manager->getScriptNames();
}

void MockSimController::executeAllEntityScripts() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : entityScriptManagers_) {
        try {
            pair.second->executeAllScripts();
        } catch (const std::exception& e) {
            if (verbose_) {
                std::cout << "[MockSim] Error executing scripts for entity " << pair.first 
                          << ": " << e.what() << std::endl;
            }
        }
    }
}
