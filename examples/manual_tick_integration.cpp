// manual_tick_integration.cpp
// 演示如何将手动模式tick集成到现有的主程序中

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>

// 假设这些头文件已经存在
#include "behaviortree/BehaviorTreeExecutor.h"
#include "behaviortree/BehaviorTreeScheduler.h"
#include "simulation/MockSimController.h"

using namespace behaviortree;
using namespace simulation;

// ============================================================================
// 方案1: 最简单的集成方式 - 在现有代码中添加定时tick
// ============================================================================

class SimpleManualTick {
public:
    // 初始化
    bool init(BehaviorTreeExecutor* executor, int tickIntervalMs = 500) {
        executor_ = executor;
        tickIntervalMs_ = tickIntervalMs;

        // 设置为手动模式
        executor_->setSchedulerManualMode(true);
        executor_->startScheduler();

        std::cout << "[SimpleManualTick] Initialized with " << tickIntervalMs_ << "ms interval" << std::endl;
        return true;
    }

    // 需要在主循环中定期调用
    void update() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTickTime_);

        // 检查是否到了tick时间
        if (elapsed.count() >= tickIntervalMs_) {
            // 执行一次行为树tick
            executor_->updateScheduler();

            lastTickTime_ = now;

            // 可选：打印状态
            printStatus();
        }
    }

    // 设置tick间隔
    void setTickInterval(int ms) {
        tickIntervalMs_ = ms;
    }

private:
    void printStatus() {
        auto treeIds = executor_->listAsyncTrees();
        if (!treeIds.empty()) {
            std::cout << "[ManualTick] Active trees: " << treeIds.size() << std::endl;
        }
    }

    BehaviorTreeExecutor* executor_;
    int tickIntervalMs_;
    std::chrono::steady_clock::time_point lastTickTime_;
};

// ============================================================================
// 方案2: 使用独立线程的定时器（但仍然手动控制tick）
// ============================================================================

class ThreadedManualTick {
public:
    ThreadedManualTick() : running_(false), tickIntervalMs_(500) {}

    ~ThreadedManualTick() {
        stop();
    }

    bool init(BehaviorTreeExecutor* executor, int tickIntervalMs = 500) {
        executor_ = executor;
        tickIntervalMs_ = tickIntervalMs;

        // 设置为手动模式
        executor_->setSchedulerManualMode(true);
        executor_->startScheduler();

        return true;
    }

    // 启动定时tick线程
    void start() {
        if (running_) return;

        running_ = true;
        tickThread_ = std::thread(&ThreadedManualTick::tickLoop, this);

        std::cout << "[ThreadedManualTick] Started with " << tickIntervalMs_ << "ms interval" << std::endl;
    }

    // 停止定时tick
    void stop() {
        running_ = false;
        if (tickThread_.joinable()) {
            tickThread_.join();
        }
    }

private:
    void tickLoop() {
        while (running_) {
            auto start = std::chrono::steady_clock::now();

            // 执行tick
            if (executor_) {
                executor_->updateScheduler();
            }

            // 精确睡眠
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto sleepTime = std::chrono::milliseconds(tickIntervalMs_) - elapsed;
            if (sleepTime > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(sleepTime);
            }
        }
    }

    BehaviorTreeExecutor* executor_;
    int tickIntervalMs_;
    std::atomic<bool> running_;
    std::thread tickThread_;
};

// ============================================================================
// 方案3: 完整的游戏循环集成示例
// ============================================================================

void gameLoopExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Game Loop Integration Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. 初始化组件
    MockSimController simController;
    simController.setVerbose(false);

    BehaviorTreeExecutor executor(&simController);
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize" << std::endl;
        return;
    }

    // 2. 加载行为树
    if (!executor.loadFromFile("bt_xml/async_square_path.xml")) {
        std::cerr << "Failed to load XML" << std::endl;
        return;
    }

    // 3. 创建实体
    std::string entityId = simController.addEntity("npc", 0, 0, 0);
    std::cout << "Created entity: " << entityId << std::endl;

    // 4. 启动异步行为树
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);
    std::string treeId = executor.executeAsync("AsyncSquarePath", blackboard, 500);

    if (treeId.empty()) {
        std::cerr << "Failed to start behavior tree" << std::endl;
        return;
    }

    std::cout << "Started behavior tree: " << treeId << std::endl;
    std::cout << std::endl;

    // 5. 游戏主循环（手动tick，500ms间隔）
    std::cout << "Starting game loop (500ms tick interval)..." << std::endl;
    std::cout << std::endl;

    const int targetFPS = 60;  // 游戏渲染帧率
    const int tickIntervalMs = 500;  // 行为树tick间隔

    auto lastRenderTime = std::chrono::steady_clock::now();
    auto lastTickTime = std::chrono::steady_clock::now();

    int frameCount = 0;
    int tickCount = 0;
    bool running = true;

    while (running) {
        auto frameStart = std::chrono::steady_clock::now();

        // ===== 渲染帧控制（60 FPS）=====
        auto renderElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            frameStart - lastRenderTime);

        if (renderElapsed.count() >= (1000 / targetFPS)) {
            frameCount++;
            // 这里执行渲染、物理更新等...
            // render();
            // updatePhysics();

            lastRenderTime = frameStart;
        }

        // ===== 行为树Tick控制（500ms）=====
        auto tickElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            frameStart - lastTickTime);

        if (tickElapsed.count() >= tickIntervalMs) {
            tickCount++;
            std::cout << "[Tick " << tickCount << "] Updating behavior trees..." << std::endl;

            // 关键：手动调用updateScheduler()
            executor.updateScheduler();

            lastTickTime = frameStart;

            // 检查行为树是否完成
            auto status = executor.getAsyncStatus(treeId);
            if (status != BT::NodeStatus::RUNNING) {
                std::cout << "Behavior tree completed!" << std::endl;
                running = false;
            }
        }

        // 小睡避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << std::endl;
    std::cout << "Game loop ended. Total frames: " << frameCount 
              << ", Total ticks: " << tickCount << std::endl;
}

// ============================================================================
// 方案4: 使用SimpleManualTick类的简化集成
// ============================================================================

void simpleIntegrationExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Simple Integration Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 初始化
    MockSimController simController;
    BehaviorTreeExecutor executor(&simController);
    executor.initialize();
    executor.loadFromFile("bt_xml/async_square_path.xml");

    std::string entityId = simController.addEntity("npc", 0, 0, 0);

    // 启动行为树
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);
    std::string treeId = executor.executeAsync("AsyncSquarePath", blackboard, 500);

    // 创建手动tick管理器
    SimpleManualTick manualTick;
    manualTick.init(&executor, 500);  // 500ms间隔

    // 模拟游戏主循环
    std::cout << "Running for 20 ticks..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        // 每帧调用update()
        manualTick.update();

        // 检查是否完成
        if (executor.getAsyncStatus(treeId) != BT::NodeStatus::RUNNING) {
            std::cout << "Behavior tree completed!" << std::endl;
            break;
        }

        // 帧率控制
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    // 运行游戏循环集成示例
    gameLoopExample();

    std::cout << std::endl;
    std::cout << "Press Enter to run simple integration example..." << std::endl;
    std::cin.get();

    // 运行简化集成示例
    simpleIntegrationExample();

    return 0;
}
