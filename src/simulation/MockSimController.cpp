#include "simulation/MockSimController.h"

namespace simulation {

MockSimController::MockSimController()
    : state_(0)
    , simTime_(0.0)
    , timeScale_(1.0)
    , timeStep_(0.016)
    , autoUpdate_(false)
    , running_(false)
    , verbose_(true)
    , nextEntityId_(1) {
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
    }
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

std::string MockSimController::generateEntityId() {
    uint64_t id = nextEntityId_.fetch_add(1);
    return "entity_" + std::to_string(id);
}

std::string MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    std::string entityId = generateEntityId();
    entities_[entityId] = Entity(entityId, type, x, y, z);
    
    if (verbose_) {
        std::cout << "[MockSim] Entity added: " << entityId << " (type: " << type << ") at (" 
                  << x << ", " << y << ", " << z << ")" << std::endl;
    }
    
    return entityId;
}

bool MockSimController::removeEntity(const std::string& entityId) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to remove entity: " << entityId << " (not found)" << std::endl;
        }
        return false;
    }
    
    entities_.erase(it);
    
    if (verbose_) {
        std::cout << "[MockSim] Entity removed: " << entityId << std::endl;
    }
    
    return true;
}

bool MockSimController::moveEntity(const std::string& entityId, double x, double y, double z) {
    auto it = entities_.find(entityId);
    if (it == entities_.end()) {
        if (verbose_) {
            std::cout << "[MockSim] Failed to move entity: " << entityId << " (not found)" << std::endl;
        }
        return false;
    }
    
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    
    if (verbose_) {
        std::cout << "[MockSim] Entity moved: " << entityId << " to (" << x << ", " << y << ", " << z << ")" << std::endl;
    }
    
    return true;
}

bool MockSimController::getEntityPosition(const std::string& entityId, double& x, double& y, double& z) {
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

} // namespace simulation
