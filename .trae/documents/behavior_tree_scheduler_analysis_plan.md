# BehaviorTreeScheduler 作用分析与去留方案

## BehaviorTreeScheduler 的作用

### 1. 核心功能

**BehaviorTreeScheduler 是一个单例调度器，负责管理行为树的周期性执行（tick）：**

```cpp
// 单例模式
class BehaviorTreeScheduler {
    static BehaviorTreeScheduler& getInstance();
    
    // 注册实体行为树
    bool registerEntityWithTree(entityId, treeName, tree, blackboard);
    
    // 执行所有已注册的行为树（按时间间隔）
    void tickAll();
};
```

### 2. 主要功能

| 功能 | 说明 |
|------|------|
| **注册行为树** | `registerEntityWithTree()` - 将实体的行为树加入调度 |
| **定时 tick** | 每 500ms 自动执行 `tree.tickRoot()` |
| **暂停/恢复** | `pauseEntity()` / `resumeEntity()` |
| **自定义间隔** | 每个实体可以有不同的 tick 间隔 |
| **状态查询** | `getEntityStatus()` - 获取行为树执行状态 |

### 3. 当前使用场景

```
LuaBehaviorTreeBridge (Lua 层调用)
    └── executeAsync() → 注册到 BehaviorTreeScheduler
        └── scheduler.registerEntityWithTree(entityId, ...)
            └── 每 500ms 自动 tick

外部调用（如 main.cpp）
    └── scheduler.tickAll() → 执行所有注册的行为树
```

### 4. 与 BTScript 的关系

**当前有两个行为树执行机制：**

```
机制 1: BehaviorTreeScheduler (全局调度)
├── 用于 Lua 层通过 bt.execute_async() 调用的行为树
├── 由外部定期调用 tickAll()
└── 适合长时间运行的行为树

机制 2: BTScript::execute() (脚本内部)
├── 用于 EntityScriptManager 管理的脚本
├── 在 execute() 中直接调用 tree.tickRoot()
├── 每次脚本执行时 tick 一次
└── 适合与 Lua 逻辑混合的行为树
```

## 是否可以去掉 BehaviorTreeScheduler？

### 方案 1: 保留 BehaviorTreeScheduler（推荐）

**适用场景：**
- 需要 Lua 层通过 `bt.execute_async()` 异步执行行为树
- 需要全局统一管理多个实体的行为树
- 需要暂停/恢复/查询状态等功能

**优点：**
- 功能完整，支持异步执行
- 支持自定义 tick 间隔
- 支持状态查询和管理

**缺点：**
- 多一个组件，复杂度增加
- 需要外部定期调用 `tickAll()`

### 方案 2: 去掉 BehaviorTreeScheduler（简化）

**修改内容：**
1. 删除 `BehaviorTreeScheduler` 类
2. 修改 `LuaBehaviorTreeBridge`：
   - `executeAsync()` 改为直接 tick，不注册到调度器
   - 或删除 `executeAsync()`，只保留同步执行
3. 删除 `bt.execute_async()` Lua API

**修改后的架构：**

```
Lua 层:
- bt.load() - 加载行为树
- bt.execute() - 同步执行一次
- bt.stop() - 停止
- ❌ bt.execute_async() - 删除

C++ 层:
- LuaBehaviorTreeBridge::execute() - 同步执行
- ❌ BehaviorTreeScheduler - 删除
```

**优点：**
- 架构更简单
- 减少一个单例
- 行为树执行统一在 BTScript 中处理

**缺点：**
- 失去异步执行能力
- Lua 层无法单独运行行为树（必须通过 BTScript）

## 建议

**如果不需要 Lua 层独立运行行为树** → 可以去掉 BehaviorTreeScheduler

**如果需要 Lua 层独立运行行为树** → 保留 BehaviorTreeScheduler

## 当前代码中的使用

```cpp
// LuaBehaviorTreeBridge.cpp 中使用 scheduler 的地方：

// 1. executeAsync - 注册到调度器
bool LuaBehaviorTreeBridge::executeAsync(...) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    scheduler.registerEntityWithTree(entityId, treeName, std::move(tree), blackboard);
}

// 2. stopAsync - 从调度器注销
bool LuaBehaviorTreeBridge::stopAsync(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    scheduler.unregisterEntity(entityId);
}

// 3. getAsyncStatus - 查询调度器状态
std::string LuaBehaviorTreeBridge::getAsyncStatus(const std::string& entityId) {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    BT::NodeStatus status = scheduler.getEntityStatus(entityId);
}

// 4. getAsyncEntities - 获取调度器中的实体列表
sol::table LuaBehaviorTreeBridge::getAsyncEntities() {
    auto& scheduler = behaviortree::BehaviorTreeScheduler::getInstance();
    std::vector<std::string> entityIds = scheduler.getRegisteredEntityIds();
}
```

## 如果去掉，需要修改的文件

1. **删除文件：**
   - `include/behaviortree/BehaviorTreeScheduler.h`
   - `src/behaviortree/BehaviorTreeScheduler.cpp`

2. **修改文件：**
   - `src/behaviortree/CMakeLists.txt` - 移除 scheduler 源文件
   - `src/scripting/LuaBehaviorTreeBridge.cpp` - 移除 scheduler 相关代码
   - `src/scripting/LuaBehaviorTreeBridge.h` - 移除 async 相关方法

3. **Lua API 变更：**
   - 删除 `bt.execute_async()`
   - 删除 `bt.stop_async()`
   - 删除 `bt.get_async_status()`
   - 删除 `bt.get_async_entities()`
   - 保留 `bt.load()`, `bt.execute()`, `bt.stop()`
