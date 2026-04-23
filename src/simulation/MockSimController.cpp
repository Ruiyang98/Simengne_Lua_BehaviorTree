#include "simulation/MockSimController.h"
#include "scripting/EntityScriptManager.h"
#include "scripting/LuaSimBinding.h"
#include <cmath>



MockSimController::MockSimController()
    : state_(0)
    , simTime_(0.0)
    , timeScale_(1.0)
    , timeStep_(0.016)
    , autoUpdate_(false)
    , running_(false)
    , verbose_(true)
    , nextVehicleId_(1)
    , btFactory_(nullptr)
    , scriptUpdateAccumulator_(0.0) {
}

MockSimController::~MockSimController() {
    stop();
    if (updateThread_.joinable()) {
        updateThread_.join();
    }
}

bool MockSimController::start() {
    if (state_ == 1) {
        if (verbose_) std::cout << "[MockSim] Already running" << std::endl;
        return false;
    }

    state_ = 1;
    running_ = true;
    
    if (verbose_) std::cout << "[MockSim] Simulation started" << std::endl;
    
    notifyStart();

    if (autoUpdate_) {
        if (updateThread_.joinable()) {
            updateThread_.join();
        }
        updateThread_ = std::thread(&MockSimController::runSimulationLoop, this);
    }

    return true;
}

bool MockSimController::pause() {
    if (state_ != 1) {
        if (verbose_) std::cout << "[MockSim] Cannot pause: not running" << std::endl;
        return false;
    }

    state_ = 2;
    if (verbose_) std::cout << "[MockSim] Simulation paused, time: " << simTime_ << "s" << std::endl;
    
    notifyPause();
    return true;
}

bool MockSimController::resume() {
    if (state_ != 2) {
        if (verbose_) std::cout << "[MockSim] Cannot resume: not paused" << std::endl;
        return false;
    }

    state_ = 1;
    if (verbose_) std::cout << "[MockSim] Simulation resumed" << std::endl;
    
    notifyResume();
    return true;
}

bool MockSimController::stop() {
    if (state_ == 0) {
        return false;
    }

    state_ = 0;
    running_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation stopped, final time: " << simTime_ << "s" << std::endl;
    
    notifyStop();
    return true;
}

bool MockSimController::reset() {
    bool wasRunning = (state_ == 1);
    
    state_ = 0;
    simTime_ = 0.0;
    running_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation reset" << std::endl;
    
    notifyReset();
    
    if (wasRunning) {
        start();
    }
    
    return true;
}

SimState MockSimController::getState() const {
    return static_cast<SimState>(state_);
}

bool MockSimController::isRunning() const {
    return state_ == 1;
}

bool MockSimController::isPaused() const {
    return state_ == 2;
}

bool MockSimController::isStopped() const {
    return state_ == 0;
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

void MockSimController::setOnStartCallback(SimEventCallback callback) {
    onStartCallback_ = callback;
}

void MockSimController::setOnPauseCallback(SimEventCallback callback) {
    onPauseCallback_ = callback;
}

void MockSimController::setOnResumeCallback(SimEventCallback callback) {
    onResumeCallback_ = callback;
}

void MockSimController::setOnStopCallback(SimEventCallback callback) {
    onStopCallback_ = callback;
}

void MockSimController::setOnResetCallback(SimEventCallback callback) {
    onResetCallback_ = callback;
}

void MockSimController::update(double deltaTime) {
    if (state_ == 1) {
        simTime_ = simTime_ + deltaTime * timeScale_;
        
        // Update scripts
        updateScripts(deltaTime);
    }
}

void MockSimController::updateScripts(double deltaTime) {
    scriptUpdateAccumulator_ += deltaTime * timeScale_;
    
    if (scriptUpdateAccumulator_ >= SCRIPT_UPDATE_INTERVAL) {
        scriptUpdateAccumulator_ -= SCRIPT_UPDATE_INTERVAL;
        
        // Execute all entity script managers
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
}

void MockSimController::setBehaviorTreeFactory(BT::BehaviorTreeFactory* factory) {
    btFactory_ = factory;
}

std::shared_ptr<scripting::EntityScriptManager> MockSimController::createScriptManager(const std::string& entityId) {
    if (entityScriptManagers_.find(entityId) != entityScriptManagers_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Script manager already exists for entity: " << entityId << std::endl;
        }
        return entityScriptManagers_[entityId];
    }
    
    // Get global Lua state
    scripting::LuaSimBinding& luaBinding = scripting::LuaSimBinding::getInstance();
    if (!luaBinding.isInitialized()) {
        if (verbose_) {
            std::cout << "[MockSim] Lua binding not initialized, initializing now..." << std::endl;
        }
        if (!luaBinding.initialize(btFactory_)) {
            std::cerr << "[MockSim] Failed to initialize Lua binding" << std::endl;
            return nullptr;
        }
    }
    
    // Create EntityScriptManager using global Lua state
    auto manager = std::make_shared<scripting::EntityScriptManager>(entityId, luaBinding.getState(), btFactory_);
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

std::shared_ptr<scripting::EntityScriptManager> MockSimController::getScriptManager(const std::string& entityId) {
    auto it = entityScriptManagers_.find(entityId);
    if (it != entityScriptManagers_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MockSimController::hasScriptManager(const std::string& entityId) const {
    return entityScriptManagers_.find(entityId) != entityScriptManagers_.end();
}

std::vector<std::string> MockSimController::getManagedEntityIds() const {
    std::vector<std::string> ids;
    ids.reserve(entityScriptManagers_.size());
    
    for (const auto& pair : entityScriptManagers_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

void MockSimController::setAutoUpdate(bool enable) {
    autoUpdate_ = enable;
}

void MockSimController::setVerbose(bool verbose) {
    verbose_ = verbose;
}

void MockSimController::runSimulationLoop() {
    using namespace std::chrono;
    steady_clock::time_point lastTime = steady_clock::now();
    
    while (running_) {
        steady_clock::time_point currentTime = steady_clock::now();
        double deltaTime = duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        update(deltaTime);
        
        std::this_thread::sleep_for(milliseconds(16));
    }
}

void MockSimController::notifyStart() {
    if (onStartCallback_) {
        onStartCallback_();
    }
}

void MockSimController::notifyPause() {
    if (onPauseCallback_) {
        onPauseCallback_();
    }
}

void MockSimController::notifyResume() {
    if (onResumeCallback_) {
        onResumeCallback_();
    }
}

void MockSimController::notifyStop() {
    if (onStopCallback_) {
        onStopCallback_();
    }
}

void MockSimController::notifyReset() {
    if (onResetCallback_) {
        onResetCallback_();
    }
}

VehicleID MockSimController::generateVehicleId() {
    VehicleID id;
    id.address.site = 0;
    id.address.host = 0;
    id.vehicle = nextVehicleId_.fetch_add(1);
    return id;
}

VehicleID MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    VehicleID vehicleId = generateVehicleId();
    entities_[vehicleId] = Entity(vehicleId, type, x, y, z);

    if (verbose_) {
        std::cout << "[MockSim] Entity added: vehicle=" << vehicleId.vehicle
                  << " (type: " << type << ") at ("
                  << x << ", " << y << ", " << z << ")" << std::endl;
    }

    return vehicleId;
}

bool MockSimController::removeEntity(const VehicleID& entityId) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to remove entity: vehicle=" << entityId.vehicle << " (not found)" << std::endl;
        }
        return false;
    }

    entities_.erase(it);

    if (verbose_) {
        std::cout << "[MockSim] Entity removed: vehicle=" << entityId.vehicle << std::endl;
    }

    return true;
}

bool MockSimController::moveEntity(const VehicleID& entityId, double x, double y, double z) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to move entity: vehicle=" << entityId.vehicle << " (not found)" << std::endl;
        }
        return false;
    }

    it->second.x = x;
    it->second.y = y;
    it->second.z = z;

    if (verbose_) {
        std::cout << "[MockSim] Entity moved: vehicle=" << entityId.vehicle << " to (" << x << ", " << y << ", " << z << ")" << std::endl;
    }

    return true;
}

bool MockSimController::getEntityPosition(const VehicleID& entityId, double& x, double& y, double& z) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        return false;
    }

    x = it->second.x;
    y = it->second.y;
    z = it->second.z;

    return true;
}

std::vector<Entity> MockSimController::getAllEntities() {
    std::vector<Entity> result;
    result.reserve(entities_.size());
    
    for (const auto& pair : entities_) {
        result.push_back(pair.second);
    }
    
    return result;
}

size_t MockSimController::getEntityCount() {
	return entities_.size();
}

bool MockSimController::setEntityMoveDirection(const VehicleID& entityId, double dx, double dy, double dz) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to set move direction for entity: vehicle=" << entityId.vehicle << " (not found)" << std::endl;
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

    // Store direction in the entity (using a simple approach - in real implementation
    // this would be stored separately and applied during simulation update)
    // For now, we just move the entity immediately by a small amount in that direction
    double moveSpeed = 1.0; // units per tick
    it->second.x += dx * moveSpeed;
    it->second.y += dy * moveSpeed;
    it->second.z += dz * moveSpeed;

    if (verbose_) {
        std::cout << "[MockSim] Entity vehicle=" << entityId.vehicle << " moved in direction (" << dx << ", " << dy << ", " << dz << ")" << std::endl;
    }

    return true;
}

double MockSimController::getEntityDistance(const VehicleID& entityId, double x, double y, double z) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        return -1.0;
    }

    double dx = it->second.x - x;
    double dy = it->second.y - y;
    double dz = it->second.z - z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
