# BehaviorTreeScheduler 移除计划

## 1. BehaviorTreeScheduler 原本的作用

从代码分析来看，BehaviorTreeScheduler 原本设计用于：

1. **异步/后台执行行为树**：允许行为树在独立的执行循环中运行，不阻塞主线程
2. **定时 tick 行为树**：按照固定时间间隔（如每 500ms）自动 tick 行为树
3. **管理多个并发行为树**：跟踪多个实体上运行的行为树状态
4. **提供异步执行 API**：如 `executeAsync()`, `stopAsync()`, `getAsyncStatus()` 等

## 2. 当前架构分析

### 2.1 移除后的现状

BehaviorTreeScheduler 已经被移除，当前架构如下：

```
EntityScriptManager (每个实体一个)
    ├── TacticalScript (纯 Lua 脚本)
    │   └── execute() -> 执行 Lua 逻辑
    └── BTScript (Lua + 行为树混合脚本)
        ├── execute() -> 
        │   ├── 1. 执行 Lua 逻辑
        │   ├── 2. 初始化行为树 (如需要)
        │   └── 3. 直接调用 tree.tickRoot() (同步执行)
        └── 行为树状态保存在 BTScript 对象中
```

### 2.2 行为树执行流程

在 `BTScript::execute()` 中：

```cpp
void BTScript::execute() {
    // 1. 执行 Lua 逻辑
    if (executeFunc_.valid()) {
        auto result = executeFunc_(state_);
    }
    
    // 2. 初始化行为树（如果还没初始化）
    if (!btInitialized_) {
        initializeBT();
    }
    
    // 3. 直接 tick 行为树（同步执行）
    if (tree_.rootNode()) {
        BT::NodeStatus status = tree_.tickRoot();
    }
}
```

## 3. "失去异步执行能力"的含义

### 3.1 原来的异步执行

原来通过 BehaviorTreeScheduler 可以：

```cpp
// 异步启动行为树
scheduler->executeAsync("treeName", "entity1");

// 行为树在后台运行，主线程继续执行
// 可以查询状态
auto status = scheduler->getAsyncStatus("entity1");

// 可以随时停止
scheduler->stopAsync("entity1");
```

### 3.2 现在的同步执行

现在通过 BTScript 执行：

```cpp
// 在 EntityScriptManager::executeAllScripts() 中
for (auto& pair : scripts_) {
    pair.second->execute();  // 同步执行，阻塞直到完成
}
```

行为树的 `tickRoot()` 是同步调用的，会立即返回当前状态。

## 4. 用户问题的回答

### 4.1 "在 scriptmanager 里直接调用不行吗"

**可以，而且这正是当前的实现方式。**

在 `EntityScriptManager::executeAllScripts()` 中直接调用每个脚本的 `execute()`，包括 BTScript 的 `execute()`，其中会直接 tick 行为树。

### 4.2 同步 vs 异步的区别

| 特性 | 原来的 Scheduler (异步) | 现在的 ScriptManager (同步) |
|------|------------------------|---------------------------|
| 执行方式 | 后台线程/定时器 tick | 主线程直接调用 |
| 阻塞性 | 不阻塞，立即返回 | 阻塞直到 tick 完成 |
| 状态管理 | Scheduler 集中管理 | 每个 BTScript 自己管理 |
| 适用场景 | 需要持续运行的 AI | 每帧/定时执行的 AI |

### 4.3 当前架构的优势

1. **简化设计**：不需要额外的调度器组件
2. **统一执行流程**：所有脚本（Tactical 和 BT）都通过相同的 `execute()` 接口执行
3. **状态隔离**：每个脚本有自己的状态表，行为树状态保存在 BTScript 对象中
4. **易于调试**：同步执行更容易跟踪和调试

### 4.4 可能的限制

如果行为树的某个节点执行耗时操作（如复杂计算、IO），会阻塞 `executeAllScripts()` 的执行。但在典型的游戏/仿真场景中：

- 行为树节点应该是轻量级的
- 耗时操作应该异步处理或使用回调
- 每 500ms 执行一次脚本，tick 应该很快完成

## 5. 结论

**BehaviorTreeScheduler 可以被移除**，因为：

1. `EntityScriptManager` 已经通过 `BTScript` 实现了行为树的执行
2. 同步执行模式更简单，适合大多数场景
3. 行为树状态保存在 `BTScript` 对象中，不需要集中管理
4. 如果确实需要异步执行，可以在应用层实现（如使用定时器调用 `executeAllScripts`）

当前架构是合理的，不需要恢复 BehaviorTreeScheduler。
