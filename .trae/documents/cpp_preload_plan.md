# C++ 中自动预加载 Lua 和 XML 的计划（简化版）

## 目标
让用户在 Lua 中直接调用 `bt.execute()` 执行行为树，不需要手动调用 `bt.load_registry()` 和 `bt.preload_trees_from_dir()`。

## 方案：C++ 初始化时自动预加载（无配置参数）

在 C++ 初始化阶段自动完成所有预加载工作，使用硬编码的默认路径。

### 默认路径
- Lua 注册表: `scripts/bt_nodes_registry.lua`
- XML 目录: `bt_xml/`

## 具体修改步骤

### 步骤 1: 修改 LuaBehaviorTreeBridge.h

1. 移除 `loadNodesRegistry()` 和 `preloadBehaviorTreesFromDirectory()` 的 public 声明
2. 将这些方法改为 private，或直接在 initialize 中内联实现

### 步骤 2: 修改 LuaBehaviorTreeBridge.cpp

1. 修改 `initialize()` 方法，在初始化完成后自动：
   - 调用 `loadNodesRegistry("scripts/bt_nodes_registry.lua")`
   - 调用 `preloadBehaviorTreesFromDirectory("bt_xml/")`

2. 修改 `registerLuaAPI()`，移除以下 Lua API 绑定：
   - `bt.load_registry`
   - `bt.preload_trees_from_dir`

3. 保留的 Lua API：
   - `bt.execute()` - 执行行为树
   - `bt.execute_async()` - 异步执行
   - `bt.stop()` / `bt.stop_async()` - 停止
   - `bt.get_status()` / `bt.get_async_status()` - 获取状态
   - `bt.register_action()` / `bt.register_condition()` / `bt.register_stateful_action()` - 注册节点
   - `bt.set_blackboard()` / `bt.get_blackboard()` - 黑板操作
   - `bt.has_tree()` - 检查树是否存在
   - `bt.get_last_error()` - 获取错误信息

### 步骤 3: 修改 main.cpp

1. 移除手动调用 `bt.load_registry()` 的代码（约第546-550行）
2. BT Bridge 初始化时会自动完成预加载

## 修改后的使用流程

### C++ 端（main.cpp）
```cpp
// 初始化 BT Bridge 时自动完成所有预加载
if (!luaBinding->initializeBehaviorTree(&g_btExecutor->getFactory())) {
    std::cerr << "BT bridge initialization failed" << std::endl;
    return 1;
}
// 不需要再手动调用 bt.load_registry()
```

### Lua 端
```lua
-- 直接执行行为树，不需要手动预加载
local treeId = bt.execute("SquarePath", "npc_001", {speed = 5})

-- 或者异步执行
bt.execute_async("PatrolTree", "npc_001")

-- 注册自定义节点
bt.register_action("custom_action", function()
    print("Custom action executed")
    return "SUCCESS"
end)
```

## 备选：支持多个 XML 目录

如果需要在多个目录中加载 XML，可以在 `initialize()` 中硬编码多个目录：

```cpp
bool LuaBehaviorTreeBridge::initialize() {
    // ... 原有初始化代码 ...
    
    // 自动预加载默认目录
    const std::vector<std::string> defaultDirs = {
        "bt_xml/",
        "behavior_trees/",
        "scripts/bt_xml/"
    };
    
    for (const auto& dir : defaultDirs) {
        preloadBehaviorTreesFromDirectory(dir);
    }
    
    return true;
}
```

## 预期结果

修改后：
1. C++ 启动时自动加载 Lua 注册表和 XML 行为树
2. Lua 代码更简洁，直接调用 `bt.execute()` 即可
3. main.cpp 中移除手动预加载代码
4. Lua API 更精简，只保留执行相关功能
