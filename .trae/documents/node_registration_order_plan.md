# 行为树节点注册顺序分析

## 问题
1. 现在的代码，是先注册所有的lua写的行为树节点，再注册所有C++行为树节点吗？
2. `bt_nodes_registry.lua` 中的节点什么时候注册的？

## 分析结果

### 1. C++ 节点注册（最先注册）

在 [BehaviorTreeExecutor::registerNodes()](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L61-L75) 中：

```cpp
void BehaviorTreeExecutor::registerNodes() {
    // Register AsyncMoveToPoint node (async)
    factory_.registerNodeType<AsyncMoveToPoint>("AsyncMoveToPoint");

    // Register CheckEntityExists node
    factory_.registerNodeType<CheckEntityExists>("CheckEntityExists");

    // Register SelectTargetFromList node (sync)
    factory_.registerNodeType<SelectTargetFromList>("SelectTargetFromList");
    ...
}
```

注册的 C++ 节点包括：
- `AsyncMoveToPoint` - 异步移动到目标点
- `CheckEntityExists` - 检查实体是否存在
- `SelectTargetFromList` - 从列表中选择目标

### 2. Lua 节点类型注册（包装器节点）

在 [LuaBehaviorTreeBridge::registerLuaNodeTypes()](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L236-L250) 中：

```cpp
void LuaBehaviorTreeBridge::registerLuaNodeTypes() {
    // Register Lua action node type
    factory_->registerNodeType<LuaActionNode>("LuaAction");

    // Register Lua condition node type
    factory_->registerNodeType<LuaConditionNode>("LuaCondition");

    // Register Lua stateful action node type
    factory_->registerNodeType<LuaStatefulActionNode>("LuaStatefulAction");
}
```

这里注册的是**Lua节点的C++包装器类型**，不是具体的Lua节点逻辑。

### 3. bt_nodes_registry.lua 中的节点注册时机

**关键结论：`bt_nodes_registry.lua` 不会自动加载，需要手动调用。**

#### 3.1 提供加载功能的API

在 [LuaBehaviorTreeBridge::registerLuaAPI()](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L364-L369) 中：

```cpp
// ==================== Preload API ====================

// Load global nodes registry script
btTable.set_function("load_registry", [this](sol::optional<std::string> registryPath) -> bool {
    return loadNodesRegistry(registryPath.value_or("scripts/bt_nodes_registry.lua"));
});
```

#### 3.2 实际加载函数

在 [LuaBehaviorTreeBridge::loadNodesRegistry()](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L766-L790) 中：

```cpp
bool LuaBehaviorTreeBridge::loadNodesRegistry(const std::string& registryPath) {
    try {
        // Load and execute the registry script
        sol::load_result script = luaState_->load_file(registryPath);
        if (!script.valid()) {
            sol::error err = script;
            lastError_ = std::string("Failed to load registry script: ") + err.what();
            return false;
        }

        sol::protected_function_result result = script();
        if (!result.valid()) {
            sol::error err = result;
            lastError_ = std::string("Failed to execute registry script: ") + err.what();
            return false;
        }

        std::cout << "[LuaBehaviorTreeBridge] Loaded nodes registry from: " << registryPath << std::endl;
        return true;

    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load nodes registry: ") + e.what();
        return false;
    }
}
```

### 4. 初始化顺序总结

在 [main.cpp](file:///d:/workspace/behaviortree/TestProject/src/main.cpp#L505-L544) 中：

```cpp
int main(int argc, char* argv[]) {
    // 1. 创建 BehaviorTreeExecutor
    g_btExecutor.reset(new BehaviorTreeExecutor(g_simController.get()));
    
    // 2. 初始化 BehaviorTreeExecutor → 注册 C++ 节点 (AsyncMoveToPoint, CheckEntityExists, SelectTargetFromList)
    if (!g_btExecutor->initialize()) { ... }
    
    // 3. 创建 LuaSimBinding
    std::unique_ptr<LuaSimBinding> luaBinding(new LuaSimBinding(g_simController.get()));
    
    // 4. 初始化 Lua 环境
    if (!luaBinding->initialize()) { ... }
    
    // 5. 初始化 Lua-BT Bridge → 注册 Lua 节点包装器类型 (LuaAction, LuaCondition, LuaStatefulAction)
    if (!luaBinding->initializeBehaviorTree(&g_btExecutor->getFactory())) { ... }
    
    // 注意：bt_nodes_registry.lua 不会在这里自动加载！
    ...
}
```

### 5. 如何加载 bt_nodes_registry.lua

#### 方式一：在 Lua 脚本中手动调用

```lua
-- 在 example_bt_xxx.lua 中
bt.load_registry("scripts/bt_nodes_registry.lua")  -- 加载全局节点注册表
```

#### 方式二：在 main.cpp 中自动加载（需要修改代码）

```cpp
// 在 Lua-BT Bridge 初始化后添加
luaBinding->executeString("bt.load_registry()");
```

#### 方式三：在每个使用Lua节点的脚本中单独注册

```lua
-- 在 example_bt_lua_nodes.lua 中
bt.register_action("LuaCheckHealth", function() ... end)
bt.register_condition("LuaHasEntities", function() ... end)
```

## 最终结论

| 阶段 | 注册内容 | 时机 |
|------|---------|------|
| 1 | C++ 节点 (AsyncMoveToPoint, CheckEntityExists, SelectTargetFromList) | BehaviorTreeExecutor::initialize() |
| 2 | Lua 节点包装器类型 (LuaAction, LuaCondition, LuaStatefulAction) | LuaBehaviorTreeBridge::initialize() |
| 3 | **bt_nodes_registry.lua 中的具体 Lua 节点** | **需要手动调用 `bt.load_registry()`** |

**`bt_nodes_registry.lua` 中的节点不会自动注册**，必须通过以下方式之一加载：
1. 在 Lua 脚本中调用 `bt.load_registry()`
2. 修改 C++ 代码在初始化时自动加载
3. 在使用时单独注册每个节点

---

## 修改计划：方式二 - 在 main.cpp 中自动加载

### 修改文件
- `d:\workspace\behaviortree\TestProject\src\main.cpp`

### 修改位置
在 `main.cpp` 第 538-544 行附近，Lua-BT Bridge 初始化成功后添加自动加载代码。

### 具体修改
在以下代码之后：
```cpp
// Initialize Lua-BehaviorTree bridge
if (!luaBinding->initializeBehaviorTree(&g_btExecutor->getFactory())) {
    std::cerr << "WARNING: Lua-BT bridge initialization failed: " << luaBinding->getLastError() << std::endl;
    std::cout << "OK: Lua environment ready (without BT integration)" << std::endl;
} else {
    std::cout << "OK: Lua-BehaviorTree bridge initialized" << std::endl;
}
```

添加自动加载 `bt_nodes_registry.lua` 的代码：
```cpp
// Auto-load bt_nodes_registry.lua
if (luaBinding->executeString("bt.load_registry()")) {
    std::cout << "OK: Lua nodes registry loaded" << std::endl;
} else {
    std::cerr << "WARNING: Failed to load Lua nodes registry: " << luaBinding->getLastError() << std::endl;
}
```

这样程序启动时会自动加载 `scripts/bt_nodes_registry.lua` 中的所有 Lua 节点。
