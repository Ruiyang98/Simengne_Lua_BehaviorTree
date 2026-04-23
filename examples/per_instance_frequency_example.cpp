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

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Per-Instance Frequency Control Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 1. 初始化
    MockSimController* simController = MockSimController::createInstance();
    simController->setVerbose(false);

    BehaviorTreeExecutor executor;
    if (!executor.initialize()) {
        std::cerr << "Failed to initialize" << std::endl;
        return 1;
    }

    // 2. 加载行为树
    if (!executor.loadFromFile("bt_xml/async_square_path.xml")) {
        std::cerr << "Failed to load XML" << std::endl;
        return 1;
    }

    std::cout << "Scheduler started in MANUAL mode" << std::endl;
    std::cout << std::endl;

    // 3. 创建3个实体，每个有不同的tick频率
    std::cout << "Creating entities with different tick frequencies..." << std::endl;
    std::cout << std::endl;

    // 实体1：高频（50ms）- 玩家
    VehicleID entity1 = simController->addEntity("player", 0, 0, 0);
    auto bb1 = BT::Blackboard::create();
    bb1->set("vehicle_id", entity1);
    std::string tree1 = std::to_string(entity1.vehicle);
    
    BT::Tree btTree1 = executor.getFactory().createTree("AsyncSquarePath", bb1);
    BehaviorTreeScheduler::getInstance().registerEntityWithTreeAndInterval(tree1, "AsyncSquarePath", std::move(btTree1), 50, bb1);
    std::cout << "Entity 1 (Player):   50ms tick interval - Tree ID: " << tree1 << std::endl;

    // 实体2：中频（200ms）- 普通NPC
    VehicleID entity2 = simController->addEntity("npc", 100, 0, 0);
    auto bb2 = BT::Blackboard::create();
    bb2->set("vehicle_id", entity2);
    std::string tree2 = std::to_string(entity2.vehicle);
    
    BT::Tree btTree2 = executor.getFactory().createTree("AsyncSquarePath", bb2);
    BehaviorTreeScheduler::getInstance().registerEntityWithTreeAndInterval(tree2, "AsyncSquarePath", std::move(btTree2), 200, bb2);
    std::cout << "Entity 2 (NPC):     200ms tick interval - Tree ID: " << tree2 << std::endl;

    // 实体3：低频（500ms）- 背景NPC
    VehicleID entity3 = simController->addEntity("background_npc", 200, 0, 0);
    auto bb3 = BT::Blackboard::create();
    bb3->set("vehicle_id", entity3);
    std::string tree3 = std::to_string(entity3.vehicle);
    
    BT::Tree btTree3 = executor.getFactory().createTree("AsyncSquarePath", bb3);
    BehaviorTreeScheduler::getInstance().registerEntityWithTreeAndInterval(tree3, "AsyncSquarePath", std::move(btTree3), 500, bb3);
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

        // 每10ms调用一次tickAll（模拟游戏帧率100 FPS）
        BehaviorTreeScheduler::getInstance().tickAll();
        globalTickCount++;

        // 每1000ms打印一次状态
        if (globalTickCount % 100 == 0) {
            int seconds = globalTickCount / 100;
            std::cout << "[Time: " << seconds << "s] ";
            
            auto entityIds = BehaviorTreeScheduler::getInstance().getRegisteredEntityIds();
            std::cout << "Active trees: " << entityIds.size();
            
            for (const auto& treeId : entityIds) {
                auto status = BehaviorTreeScheduler::getInstance().getEntityStatus(treeId);
                auto info = BehaviorTreeScheduler::getInstance().getEntityInfo(treeId);
                std::cout << " | " << treeId << "(" << info->tickIntervalMs << "ms):";
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
        if (BehaviorTreeScheduler::getInstance().getRegisteredEntityCount() == 0) {
            std::cout << std::endl;
            std::cout << "All behavior trees completed!" << std::endl;
            break;
        }

        // 10秒后退出
        if (elapsed.count() >= 10000) {
            std::cout << std::endl;
            std::cout << "Timeout! Stopping all trees..." << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << std::endl;
    std::cout << "Simulation ended." << std::endl;
    std::cout << "Total scheduler updates: " << globalTickCount << std::endl;

    return 0;
}
