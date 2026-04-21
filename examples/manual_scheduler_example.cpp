// manual_scheduler_example.cpp
// 演示手动模式下如何以固定周期(500ms)tick行为树

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "behaviortree/BehaviorTreeScheduler.h"
#include "behaviortree/BehaviorTreeExecutor.h"
#include "simulation/MockSimController.h"

using namespace behaviortree;
using namespace simulation;

// 游戏引擎主循环模拟
class GameEngineSimulator {
public:
    GameEngineSimulator() : running_(false), tickIntervalMs_(500) {}

    // 初始化
    bool initialize() {
        // 创建仿真控制器
        simController_ = std::make_unique<MockSimController>();
        simController_->setVerbose(true);

        // 创建行为树执行器
        btExecutor_ = std::make_unique<BehaviorTreeExecutor>(simController_.get());
        if (!btExecutor_->initialize()) {
            std::cerr << "Failed to initialize BT executor" << std::endl;
            return false;
        }

        // 加载行为树XML
        if (!btExecutor_->loadFromFile("bt_xml/async_square_path.xml")) {
            std::cerr << "Failed to load behavior tree" << std::endl;
            return false;
        }

        // 获取调度器并设置为手动模式
        BehaviorTreeScheduler& scheduler = btExecutor_->getScheduler();
        scheduler.setManualMode(true);  // 关键：设置为手动模式
        scheduler.start();               // 启动（不会创建后台线程）

        std::cout << "[GameEngine] Initialized in MANUAL mode" << std::endl;
        std::cout << "[GameEngine] Tick interval: " << tickIntervalMs_ << "ms" << std::endl;

        return true;
    }

    // 启动异步行为树
    std::string startBehaviorTree(const std::string& treeName, const std::string& entityId) {
        // 创建黑板并设置实体ID
        auto blackboard = BT::Blackboard::create();
        blackboard->set("entity_id", entityId);

        // 启动异步行为树
        std::string treeId = btExecutor_->executeAsync(treeName, blackboard, tickIntervalMs_);

        if (!treeId.empty()) {
            std::cout << "[GameEngine] Started behavior tree: " << treeId << std::endl;
        }

        return treeId;
    }

    // 游戏主循环 - 手动控制tick
    void run() {
        running_ = true;

        std::cout << "[GameEngine] Main loop started" << std::endl;
        std::cout << "[GameEngine] Press Ctrl+C to stop" << std::endl;
        std::cout << std::endl;

        int frameCount = 0;
        auto lastTickTime = std::chrono::steady_clock::now();

        while (running_) {
            auto frameStart = std::chrono::steady_clock::now();

            // ========== 游戏引擎的其他更新 ==========
            // 更新物理
            // 更新渲染
            // 更新AI...
            // ======================================

            // ========== 手动Tick行为树 ==========
            // 检查是否到了tick时间（500ms）
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                frameStart - lastTickTime);

            if (elapsed.count() >= tickIntervalMs_) {
                frameCount++;
                std::cout << "[GameEngine] Frame " << frameCount 
                          << " - Ticking behavior trees..." << std::endl;

                // 关键：手动调用update()来tick所有行为树
                btExecutor_->updateScheduler();

                lastTickTime = frameStart;

                // 打印当前所有行为树状态
                printTreeStatus();
            }
            // ==================================

            // 检查是否所有行为树都已完成
            if (frameCount > 0 && btExecutor_->listAsyncTrees().empty()) {
                std::cout << "[GameEngine] All behavior trees completed" << std::endl;
                break;
            }

            // 小睡一下，避免CPU占用过高（实际游戏引擎会有更精确的帧率控制）
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[GameEngine] Main loop ended" << std::endl;
    }

    // 停止游戏循环
    void stop() {
        running_ = false;
    }

    // 停止指定的行为树
    void stopBehaviorTree(const std::string& treeId) {
        btExecutor_->stopAsync(treeId);
    }

private:
    void printTreeStatus() {
        auto treeIds = btExecutor_->listAsyncTrees();
        if (treeIds.empty()) {
            std::cout << "  No active behavior trees" << std::endl;
            return;
        }

        for (const auto& treeId : treeIds) {
            auto status = btExecutor_->getAsyncStatus(treeId);
            std::string statusStr;
            switch (status) {
                case BT::NodeStatus::SUCCESS: statusStr = "SUCCESS"; break;
                case BT::NodeStatus::FAILURE: statusStr = "FAILURE"; break;
                case BT::NodeStatus::RUNNING: statusStr = "RUNNING"; break;
                case BT::NodeStatus::IDLE: statusStr = "IDLE"; break;
                default: statusStr = "UNKNOWN"; break;
            }
            std::cout << "  Tree " << treeId << " - Status: " << statusStr << std::endl;
        }
    }

    std::unique_ptr<MockSimController> simController_;
    std::unique_ptr<BehaviorTreeExecutor> btExecutor_;
    std::atomic<bool> running_;
    int tickIntervalMs_;
};

// 另一个更简单的示例：使用std::thread的定时器方式
void simpleManualModeExample() {
    std::cout << "========================================" << std::endl;
    std::cout << "    Simple Manual Mode Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 创建组件
    MockSimController simController;
    simController.setVerbose(false);

    BehaviorTreeExecutor executor(&simController);
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize" << std::endl;
        return;
    }

    // 加载XML
    if (!executor.loadFromFile("bt_xml/async_square_path.xml")) {
        std::cerr << "Failed to load XML" << std::endl;
        return;
    }

    // 创建测试实体
    std::string entityId = simController.addEntity("npc", 0, 0, 0);
    std::cout << "Created entity: " << entityId << std::endl;

    // 获取调度器并设置为手动模式
    BehaviorTreeScheduler& scheduler = executor.getScheduler();
    scheduler.setManualMode(true);  // 设置为手动模式
    scheduler.start();

    std::cout << "Scheduler started in MANUAL mode" << std::endl;
    std::cout << std::endl;

    // 启动异步行为树
    auto blackboard = BT::Blackboard::create();
    blackboard->set("entity_id", entityId);

    std::string treeId = executor.executeAsync("AsyncSquarePath", blackboard, 500);
    if (treeId.empty()) {
        std::cerr << "Failed to start behavior tree" << std::endl;
        return;
    }

    std::cout << "Started behavior tree: " << treeId << std::endl;
    std::cout << "Ticking every 500ms..." << std::endl;
    std::cout << std::endl;

    // 方式1: 使用while循环 + sleep（简单但不够精确）
    std::cout << "--- Method 1: Simple while loop ---" << std::endl;
    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::steady_clock::now();

        // 手动tick
        executor.updateScheduler();

        // 检查状态
        auto status = executor.getAsyncStatus(treeId);
        std::cout << "Tick " << (i + 1) << " - Status: ";
        switch (status) {
            case BT::NodeStatus::RUNNING: std::cout << "RUNNING"; break;
            case BT::NodeStatus::SUCCESS: std::cout << "SUCCESS"; break;
            case BT::NodeStatus::FAILURE: std::cout << "FAILURE"; break;
            default: std::cout << "OTHER"; break;
        }
        std::cout << std::endl;

        if (status != BT::NodeStatus::RUNNING) {
            std::cout << "Behavior tree completed!" << std::endl;
            break;
        }

        // 精确睡眠500ms
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto sleepTime = std::chrono::milliseconds(500) - elapsed;
        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    std::cout << std::endl;
    std::cout << "Example completed!" << std::endl;
}

int main(int argc, char* argv[]) {
    // 运行简单示例
    simpleManualModeExample();

    std::cout << std::endl;
    std::cout << "Press Enter to run advanced example..." << std::endl;
    std::cin.get();

    // 运行高级示例
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "    Advanced Manual Mode Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    GameEngineSimulator engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }

    // 创建测试实体并启动行为树
    // 注意：实际应用中实体应该在initialize中创建
    engine.startBehaviorTree("AsyncSquarePath", "entity_001");

    // 运行游戏主循环
    engine.run();

    return 0;
}
