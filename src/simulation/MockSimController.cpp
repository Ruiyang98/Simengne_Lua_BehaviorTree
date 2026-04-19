#include "simulation/MockSimController.h"

namespace simulation {

// 构造函数：初始化所有成员变量
// state_: 0=STOPPED, 1=RUNNING, 2=PAUSED
// simTime_: 仿真时间（秒）
// timeScale_: 时间倍率（1.0=实时，2.0=2倍速）
// timeStep_: 时间步长（默认0.016s = 60FPS）
// autoUpdate_: 是否启用自动更新线程
// running_: 线程运行标志
// verbose_: 是否打印详细日志
MockSimController::MockSimController()
    : state_(0)
    , simTime_(0.0)
    , timeScale_(1.0)
    , timeStep_(0.016)
    , autoUpdate_(false)
    , running_(false)
    , verbose_(true) {
}

// 析构函数：停止仿真并等待线程结束
MockSimController::~MockSimController() {
    stop();
    if (updateThread_.joinable()) {
        updateThread_.join();
    }
}

// 启动仿真
// 如果已经在运行中，返回false
// 设置状态为RUNNING(1)，触发on_start回调
// 如果启用了autoUpdate，创建后台线程自动推进仿真时间
bool MockSimController::start() {
    if (state_ == 1) {
        if (verbose_) std::cout << "[MockSim] Already running" << std::endl;
        return false;
    }

    state_ = 1;
    running_ = true;
    
    if (verbose_) std::cout << "[MockSim] Simulation started" << std::endl;
    
    // 触发启动回调
    notifyStart();

    // 如果启用自动更新，创建后台线程
    if (autoUpdate_) {
        if (updateThread_.joinable()) {
            updateThread_.join();
        }
        updateThread_ = std::thread(&MockSimController::runSimulationLoop, this);
    }

    return true;
}

// 暂停仿真
// 只有在RUNNING状态才能暂停
// 设置状态为PAUSED(2)，触发on_pause回调
bool MockSimController::pause() {
    if (state_ != 1) {
        if (verbose_) std::cout << "[MockSim] Cannot pause: not running" << std::endl;
        return false;
    }

    state_ = 2;
    if (verbose_) std::cout << "[MockSim] Simulation paused, time: " << simTime_ << "s" << std::endl;
    
    // 触发暂停回调
    notifyPause();
    return true;
}

// 恢复仿真
// 只有在PAUSED状态才能恢复
// 设置状态为RUNNING(1)，触发on_resume回调
bool MockSimController::resume() {
    if (state_ != 2) {
        if (verbose_) std::cout << "[MockSim] Cannot resume: not paused" << std::endl;
        return false;
    }

    state_ = 1;
    if (verbose_) std::cout << "[MockSim] Simulation resumed" << std::endl;
    
    // 触发恢复回调
    notifyResume();
    return true;
}

// 停止仿真
// 设置状态为STOPPED(0)，停止后台线程
// 触发on_stop回调
bool MockSimController::stop() {
    if (state_ == 0) {
        return false;
    }

    state_ = 0;
    running_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation stopped, final time: " << simTime_ << "s" << std::endl;
    
    // 触发停止回调
    notifyStop();
    return true;
}

// 重置仿真
// 重置时间为0，状态为STOPPED
// 如果之前在运行，自动重新启动
// 触发on_reset回调
bool MockSimController::reset() {
    bool wasRunning = (state_ == 1);
    
    state_ = 0;
    simTime_ = 0.0;
    running_ = false;
    
    if (verbose_) std::cout << "[MockSim] Simulation reset" << std::endl;
    
    // 触发重置回调
    notifyReset();
    
    // 如果之前在运行，自动重新启动
    if (wasRunning) {
        start();
    }
    
    return true;
}

// 获取当前仿真状态
SimState MockSimController::getState() const {
    return static_cast<SimState>(state_);
}

// 检查是否正在运行
bool MockSimController::isRunning() const {
    return state_ == 1;
}

// 检查是否已暂停
bool MockSimController::isPaused() const {
    return state_ == 2;
}

// 检查是否已停止
bool MockSimController::isStopped() const {
    return state_ == 0;
}

// 获取当前仿真时间（秒）
double MockSimController::getSimTime() const {
    return simTime_;
}

// 获取仿真时间步长（秒）
double MockSimController::getTimeStep() const {
    return timeStep_;
}

// 设置时间倍率
// scale: 1.0=实时，2.0=2倍速，0.5=半速
void MockSimController::setTimeScale(double scale) {
    if (scale < 0.0) scale = 0.0;
    timeScale_ = scale;
    if (verbose_) std::cout << "[MockSim] Time scale set to: " << scale << "x" << std::endl;
}

// 获取当前时间倍率
double MockSimController::getTimeScale() const {
    return timeScale_;
}

// 设置启动事件回调函数
// 当仿真启动时，这个回调会被调用
void MockSimController::setOnStartCallback(SimEventCallback callback) {
    onStartCallback_ = callback;
}

// 设置暂停事件回调函数
// 当仿真暂停时，这个回调会被调用
void MockSimController::setOnPauseCallback(SimEventCallback callback) {
    onPauseCallback_ = callback;
}

// 设置恢复事件回调函数
// 当仿真恢复时，这个回调会被调用
void MockSimController::setOnResumeCallback(SimEventCallback callback) {
    onResumeCallback_ = callback;
}

// 设置停止事件回调函数
// 当仿真停止时，这个回调会被调用
void MockSimController::setOnStopCallback(SimEventCallback callback) {
    onStopCallback_ = callback;
}

// 设置重置事件回调函数
// 当仿真重置时，这个回调会被调用
void MockSimController::setOnResetCallback(SimEventCallback callback) {
    onResetCallback_ = callback;
}

// 更新仿真时间
// deltaTime: 经过的时间（秒）
// 只有在RUNNING状态才会推进时间
// 实际推进时间 = deltaTime * timeScale_
void MockSimController::update(double deltaTime) {
    if (state_ == 1) {
        simTime_ = simTime_ + deltaTime * timeScale_;
    }
}

// 设置是否启用自动更新线程
// enable: true=启用后台线程自动推进时间，false=需要手动调用update
void MockSimController::setAutoUpdate(bool enable) {
    autoUpdate_ = enable;
}

// 设置是否打印详细日志
void MockSimController::setVerbose(bool verbose) {
    verbose_ = verbose;
}

// 仿真更新循环线程函数
// 在后台线程中运行，自动推进仿真时间
// 循环频率：约60 FPS（每16ms更新一次）
// 工作流程：
// 1. 记录当前时间
// 2. 计算与上次的时间差（deltaTime）
// 3. 调用update推进仿真时间
// 4. 休眠16ms
// 5. 重复直到running_变为false
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

// 触发启动回调
// 如果设置了onStartCallback_，则执行它
void MockSimController::notifyStart() {
    if (onStartCallback_) {
        onStartCallback_();
    }
}

// 触发暂停回调
// 如果设置了onPauseCallback_，则执行它
void MockSimController::notifyPause() {
    if (onPauseCallback_) {
        onPauseCallback_();
    }
}

// 触发恢复回调
// 如果设置了onResumeCallback_，则执行它
void MockSimController::notifyResume() {
    if (onResumeCallback_) {
        onResumeCallback_();
    }
}

// 触发停止回调
// 如果设置了onStopCallback_，则执行它
void MockSimController::notifyStop() {
    if (onStopCallback_) {
        onStopCallback_();
    }
}

// 触发重置回调
// 如果设置了onResetCallback_，则执行它
void MockSimController::notifyReset() {
    if (onResetCallback_) {
        onResetCallback_();
    }
}

} // namespace simulation
