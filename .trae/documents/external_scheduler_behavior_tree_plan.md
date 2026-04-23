# 外部调度行为树计划文档

## Lua 实现 StatefulActionNode

### 问题：Lua 节点可以实现 StatefulActionNode 吗？

**答案：可以，但需要特殊处理。**

当前的 `LuaActionNode` 继承自 `SyncActionNode`，这意味着 Lua 函数需要一次性返回结果。要实现类似 StatefulActionNode 的行为，有两种方案：


#### 方案 : 创建 LuaStatefulActionNode C++ 类

创建一个新的 C++ 节点类型，专门支持有状态的 Lua 节点：

```cpp
class LuaStatefulActionNode : public BT::StatefulActionNode {
public:
    BT::NodeStatus onStart() override {
        // 调用 Lua onStart 函数
        auto result = callLuaFunction("onStart");
        return parseStatus(result);
    }
    
    BT::NodeStatus onRunning() override {
        // 调用 Lua onRunning 函数
        auto result = callLuaFunction("onRunning");
        return parseStatus(result);
    }
    
    void onHalted() override {
        // 调用 Lua onHalted 函数
        callLuaFunction("onHalted");
    }
};
```

**Lua 脚本：**
```lua
function onStart(params)
    -- 初始化
    return "RUNNING"
end

function onRunning(params)
    -- 持续执行
    if hasArrived() then
        return "SUCCESS"
    end
    return "RUNNING"
end

function onHalted(params)
    -- 清理
end
```

---

## 附录 B: onRunning 返回值详解

### 问题：onRunning 没有移动到指定点，是返回 FAILURE 吗？

**答案：不是！应该返回 RUNNING。**

### StatefulActionNode 返回值语义

| 返回值 | 含义 | 行为 |
|--------|------|------|
| `RUNNING` | 正在执行 | 下次 tick 继续调用 onRunning |
| `SUCCESS` | 成功完成 | 节点完成，行为树继续执行下一个节点 |
| `FAILURE` | 执行失败 | 节点失败，父节点处理失败逻辑 |

### 正确实现模式

```cpp
BT::NodeStatus AsyncMoveToPoint::onRunning() {
    // 获取当前位置
    double currentX, currentY, currentZ;
    if (!simController->getEntityPosition(entityId_, currentX, currentY, currentZ)) {
        // 获取位置失败，这是错误情况，返回 FAILURE
        std::cerr << "[AsyncMoveToPoint] Entity not found during movement" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // 检查是否到达
    if (hasArrived(currentX, currentY, currentZ)) {
        // 到达目标，返回 SUCCESS
        std::cout << "[AsyncMoveToPoint] Arrived at destination" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
    
    // 继续移动
    moveTowards(currentX, currentY, currentZ);
    simController->moveEntity(entityId_, currentX, currentY, currentZ);
    
    // 还没有到达，返回 RUNNING，下次 tick 继续
    return BT::NodeStatus::RUNNING;
}
```

### 常见错误

**错误 1：返回 FAILURE 表示还没完成**
```cpp
// 错误！
if (!hasArrived()) {
    return BT::NodeStatus::FAILURE;  // 这表示移动失败了！
}
```

**错误 2：不返回任何值**
```cpp
// 错误！
if (!hasArrived()) {
    moveTowards();
    // 忘记 return，行为未定义！
}
```

**正确做法：**
```cpp
// 正确！
if (!hasArrived()) {
    moveTowards();
    return BT::NodeStatus::RUNNING;  // 表示还在执行中
}
return BT::NodeStatus::SUCCESS;  // 表示完成
```

### 状态流转总结

```
onStart():
  ├── 初始化失败 → return FAILURE
  ├── 已经到达 → return SUCCESS
  └── 开始移动 → return RUNNING

onRunning():
  ├── 实体丢失/错误 → return FAILURE
  ├── 到达目标 → return SUCCESS
  └── 继续移动 → return RUNNING
```
