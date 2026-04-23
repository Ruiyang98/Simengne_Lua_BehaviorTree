#include "behaviortree/SelectTargetFromList.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

namespace behaviortree {

SelectTargetFromList::SelectTargetFromList(const std::string& name, const BT::NodeConfiguration& config)
    : BT::SyncActionNode(name, config)
{
    // 初始化随机种子（仅一次）
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }
}

BT::PortsList SelectTargetFromList::providedPorts() {
    return {
        BT::InputPort<std::string>("targets", "Comma-separated list of target IDs"),
        BT::InputPort<std::string>("strategy", "first", "Selection strategy: first, last, random"),
        BT::OutputPort<std::string>("selected_target", "Selected target ID")
    };
}

BT::NodeStatus SelectTargetFromList::tick() {
    auto targets = getInput<std::string>("targets");
    auto strategy = getInput<std::string>("strategy");

    if (!targets || targets.value().empty()) {
        std::cerr << "[SelectTargetFromList] No targets provided" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // 解析目标列表
    std::vector<std::string> target_list;
    std::stringstream ss(targets.value());
    std::string target;
    while (std::getline(ss, target, ',')) {
        // 去除前后空格
        size_t start = target.find_first_not_of(" \t");
        size_t end = target.find_last_not_of(" \t");
        if (start != std::string::npos) {
            target_list.push_back(target.substr(start, end - start + 1));
        }
    }

    if (target_list.empty()) {
        std::cerr << "[SelectTargetFromList] No valid targets found" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    // 根据策略选择目标
    std::string selected;
    std::string strat = strategy.value_or("first");

    if (strat == "last") {
        selected = target_list.back();
    } else if (strat == "random") {
        selected = target_list[std::rand() % target_list.size()];
    } else { // first
        selected = target_list.front();
    }

    setOutput("selected_target", selected);
    std::cout << "[SelectTargetFromList] Selected target: " << selected
              << " (strategy: " << strat << ")" << std::endl;

    return BT::NodeStatus::SUCCESS;
}

} // namespace behaviortree
