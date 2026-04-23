# SyncActionNode 示例重构计划

## 目标

删除同步移动相关的所有代码（C++、XML、Lua），保留 AsyncMoveToPoint（异步实现），并添加 SelectTargetFromList 作为 SyncActionNode 的示例。

## SelectTargetFromList 节点设计

这是一个符合 SyncActionNode 设计意图的节点：
- **同步执行**：tick() 立即返回结果，不返回 RUNNING
- **无状态**：不需要在多次 tick 之间保持状态
- **原子操作**：从列表中选择一个目标是一次性完成的逻辑操作

### 头文件

```cpp
// SelectTargetFromList.h
#ifndef SELECT_TARGET_FROM_LIST_H
#define SELECT_TARGET_FROM_LIST_H

#include <behaviortree_cpp_v3/action_node.h>

namespace behaviortree {

class SelectTargetFromList : public BT::SyncActionNode {
public:
    SelectTargetFromList(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;
};

} // namespace behaviortree

#endif // SELECT_TARGET_FROM_LIST_H
```

### 实现文件

```cpp
// SelectTargetFromList.cpp
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
```

## 需要删除的文件（同步移动相关）

### C++ 头文件
- [ ] `include/behaviortree/MoveToPoint.h`
- [ ] `include/behaviortree/FollowPath.h`
- [ ] `include/behaviortree/EntityMovement.h`

### C++ 源文件
- [ ] `src/behaviortree/MoveToPoint.cpp`
- [ ] `src/behaviortree/FollowPath.cpp`
- [ ] `src/behaviortree/EntityMovement.cpp`

### XML 文件（包含同步移动节点）
- [ ] 查找并删除/修改所有包含 `MoveToPoint` 的 XML 文件
- [ ] 查找并删除/修改所有包含 `FollowPath` 的 XML 文件

### Lua 文件（包含同步移动相关代码）
- [ ] 查找并删除 Lua 中注册/使用 MoveToPoint 的代码
- [ ] 查找并删除 Lua 中注册/使用 FollowPath 的代码

## 保留的文件（异步移动）

- [x] `include/behaviortree/AsyncMoveToPoint.h`
- [x] `src/behaviortree/AsyncMoveToPoint.cpp`
- [x] 包含 `AsyncMoveToPoint` 的 XML 文件
- [x] 包含 `AsyncMoveToPoint` 的 Lua 代码

## 需要修改的文件

### 1. BehaviorTreeExecutor.cpp
删除 MoveToPoint、FollowPath 的注册代码，添加 SelectTargetFromList 的注册。保留 AsyncMoveToPoint 的注册。

### 2. LuaBehaviorTreeBridge.cpp
删除与 MoveToPoint、FollowPath 相关的 Lua 绑定代码。保留 AsyncMoveToPoint 的绑定。

### 3. CMakeLists.txt
更新源文件列表，移除已删除的文件，添加 SelectTargetFromList.cpp。

### 4. XML 示例文件
- 删除或修改包含 MoveToPoint/FollowPath 的 XML
- 添加使用 SelectTargetFromList 的示例 XML
- 保留使用 AsyncMoveToPoint 的 XML

## XML 使用示例

```xml
<root main_tree_to_execute="MainTree">
    <BehaviorTree ID="MainTree">
        <Sequence>
            <!-- 从敌人列表中随机选择一个攻击目标 -->
            <SelectTargetFromList targets="enemy1,enemy2,enemy3,enemy4"
                                  strategy="random"
                                  selected_target="{attack_target}"/>

            <!-- 记录选择的攻击目标 -->
            <SaySomething message="Selected target: {attack_target}"/>

            <!-- 异步移动到目标位置 -->
            <AsyncMoveToPoint x="100" y="200" threshold="1.0"/>
        </Sequence>
    </BehaviorTree>
</root>
```

## 实现步骤

1. 创建 `SelectTargetFromList.h` 和 `SelectTargetFromList.cpp`
2. 删除同步移动相关的 C++ 头文件和源文件（MoveToPoint、FollowPath、EntityMovement）
3. 查找并删除/修改包含同步移动节点的 XML 文件
4. 查找并删除 Lua 中同步移动相关的代码
5. 更新 `BehaviorTreeExecutor.cpp` 中的节点注册
6. 更新 `LuaBehaviorTreeBridge.cpp` 中的 Lua 绑定
7. 更新 `CMakeLists.txt`
