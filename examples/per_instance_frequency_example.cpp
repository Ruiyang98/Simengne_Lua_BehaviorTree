// per_instance_frequency_example.cpp
// 演示实例级tick频率控制：单个Scheduler，不同实例不同频率

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include "behaviortree/BehaviorTreeExecutor.h"
#include "behaviortree/BehaviorTreeScheduler.h"
#include "simulation/MockSimController.h"

using namespace behaviortree;
using namespace simulation;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Per-Instance Frequency Control Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. 初始化
    MockSimController simController;
    simController.setVerbose(false);

    BehaviorTreeExecutor executor(&simController);
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }

    // 2. 加载行为树
    if (!executor.loadFromFile("bt_xml/async_square_path.xml")) {
        std::cerr << "Failed to load XML" << std::endl;
        return 1;
    }

    // 3. 设置为手动模式
    executor.setSchedulerManualMode(true);
    executor.startScheduler();

    std::cout << "Scheduler started in MANUAL mode" << std::endl;
    std::cout << std::endl;

    // 4. 创建3个实体，每个有不同的tick频率
    std::cout << "Creating entities with different tick frequencies..." << std::endl;
    std::cout << std::endl;

    // 实体1：高频（50ms）- 玩家
    std::string entity1 = simController.addEntity("player", 0, 0, 0);
    auto bb1 = BT::Blackboard::create();
    bb1->set("entity_id", entity1);
    std::string tree1 = executor.executeAsyncWithInterval("AsyncSquarePath", bb1, 50);
    std::cout << "Entity 1 (Player):   50ms tick interval - Tree ID: " << tree1 << std::endl;

    // 实体2：中频（200ms）- 普通NPC
    std::string entity2 = simController.addEntity("npc", 100, 0, 0);
    auto bb2 = BT::Blackboard::create();
    bb2->set("entity_id", entity2);
    std::string tree2 = executor.executeAsyncWithInterval("AsyncSquarePath", bb2, 200);
    std::cout << "Entity 2 (NPC):     200ms tick interval - Tree ID: " << tree2 << std::endl;

    // 实体3：低频（500ms）- 背景NPC
    std::string entity3 = simController.addEntity("background_npc", 200, 0, 0);
    auto bb3 = BT::Blackboard::create();
    bb3->set("entity_id", entity3);
    std::string tree3 = executor.executeAsyncWithInterval("AsyncSquarePath", bb3, 500);
    std::cout << "Entity 3 (Bg NPC):  500ms tick interval - Tree ID: " << tree3 << std::endl;

    std::cout << std::endl;
    std::cout << "Starting simulation..." << std::endl;
    std::cout << "(All trees share one scheduler, but tick at different rates)" << std::endl;
    std::cout << std::endl;

    // 5. 模拟游戏主循环
    auto startTime = std::chrono::steady_clock::now();
    int globalTickCount = 0;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        // 每10ms调用一次update（模拟游戏帧率100 FPS）
        executor.updateScheduler();
        globalTickCount++;

        // 每1000ms打印一次状态
        if (globalTickCount % 100 == 0) {
            int seconds = globalTickCount / 100;
            std::cout << "[Time: " << seconds << "s] ";
            
            auto trees = executor.listAsyncTrees();
            std::cout << "Active trees: " << trees.size();
            
            for (const auto& treeId : trees) {
                auto status = executor.getAsyncStatus(treeId);
                int interval = executor.getAsyncTreeTickInterval(treeId);
                std::cout << " | " << treeId << "(" << interval << "ms):";
                switch (status) {
                    case BT::NodeStatus::RUNNING: std::cout << "R"; break;
                    case BT::NodeStatus::SUCCESS: std::cout << "S"; break;
                    case BT::NodeStatus::FAILURE: std::cout << "F"; break;
                    default: std::cout << "?"; break;
                }
            }
            std::cout << std::endl;
        }

        // 检查是否所有树都完成
        if (executor.listAsyncTrees().empty()) {
            std::cout << std::endl;
            std::cout << "All behavior trees completed!" << std::endl;
            break;
        }

        // 10秒后退出
        if (elapsed.count() >= 10000) {
            std::cout << std::endl;
            std::cout << "Timeout! Stopping all trees..." << std::endl;
            executor.stopScheduler();
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << std::endl;
    std::cout << "Simulation ended." << std::endl;
    std::cout << "Total scheduler updates: " << globalTickCount << std::endl;

    return 0;
}
