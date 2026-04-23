# 构造函数依赖注入改为单例获取重构计划

## 1. 重构目标

将以下类的构造函数中传入的 `globalLuaState` 和 `factory` 改为从单例获取：
- `EntityScriptManager`
- `TacticalScript`
- `BTScript`
- `LuaBehaviorTreeBridge`

## 2. 单例来源

- `sol::state`: 从 `LuaSimBinding::getInstance()` 获取
- `BT::BehaviorTreeFactory`: 从 `BehaviorTreeExecutor::getInstance()` 获取（需要将 BehaviorTreeExecutor 改为单例）

## 3. 具体修改内容

### 3.1 修改 BehaviorTreeExecutor 为单例

**文件**: `include/behaviortree/BehaviorTreeExecutor.h`

```cpp
class BehaviorTreeExecutor {
public:
    // Get singleton instance
    static BehaviorTreeExecutor& getInstance();
    
    // Disable copy and assignment
    BehaviorTreeExecutor(const BehaviorTreeExecutor&) = delete;
    BehaviorTreeExecutor& operator=(const BehaviorTreeExecutor&) = delete;
    
    // Get factory
    BT::BehaviorTreeFactory& getFactory() { return factory_; }
    
    // ... other public methods
    
private:
    BehaviorTreeExecutor();  // Private constructor
    ~BehaviorTreeExecutor();
    
    // ... existing members
};
```

**文件**: `src/behaviortree/BehaviorTreeExecutor.cpp`

```cpp
// Meyers' singleton implementation
BehaviorTreeExecutor& BehaviorTreeExecutor::getInstance() {
    static BehaviorTreeExecutor instance;
    return instance;
}

BehaviorTreeExecutor::BehaviorTreeExecutor()
    : initialized_(false)
    , treeIdCounter_(0)
{
}

BehaviorTreeExecutor::~BehaviorTreeExecutor() {
    // Halt all running trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    for (auto& pair : activeTrees_) {
        if (pair.second->isRunning) {
            pair.second->tree.haltTree();
        }
    }
    activeTrees_.clear();
}
```

### 3.2 修改 TacticalScript

**文件**: `include/scripting/TacticalScript.h`

```cpp
// 修改前
TacticalScript(const std::string& name, 
               const std::string& scriptCode,
               sol::state& luaState, 
               const std::string& entityId,
               sol::table state);

// 修改后
TacticalScript(const std::string& name, 
               const std::string& scriptCode,
               const std::string& entityId,
               sol::table state);
```

**文件**: `src/scripting/TacticalScript.cpp`

- 移除构造函数中的 `luaState` 参数
- 在构造函数内部通过 `LuaSimBinding::getInstance().getState()` 获取
- 修改 `luaState_` 成员为指针类型

### 3.3 修改 BTScript

**文件**: `include/scripting/BTScript.h`

```cpp
// 修改前
BTScript(const std::string& name, 
         const std::string& scriptCode,
         const std::string& xmlFile, 
         const std::string& treeName,
         sol::state& luaState, 
         const std::string& entityId,
         BT::BehaviorTreeFactory* factory,
         sol::table state);

// 修改后
BTScript(const std::string& name, 
         const std::string& scriptCode,
         const std::string& xmlFile, 
         const std::string& treeName,
         const std::string& entityId,
         sol::table state);
```

**文件**: `src/scripting/BTScript.cpp`

- 移除构造函数中的 `luaState` 和 `factory` 参数
- 在构造函数内部通过单例获取
- 修改 `luaState_` 为指针，`factory_` 保持指针

### 3.4 修改 EntityScriptManager

**文件**: `include/scripting/EntityScriptManager.h`

```cpp
// 修改前
EntityScriptManager(const std::string& entityId, 
                    sol::state& globalLuaState,
                    BT::BehaviorTreeFactory* factory);

// 修改后
EntityScriptManager(const std::string& entityId);
```

**文件**: `src/scripting/EntityScriptManager.cpp`

- 移除构造函数中的 `globalLuaState` 和 `factory` 参数
- 在构造函数内部通过单例获取
- 修改 `addTacticalScript` 和 `addBTScript` 方法，内部创建脚本时不再传递这些参数

### 3.5 修改 LuaBehaviorTreeBridge

**文件**: `include/scripting/LuaBehaviorTreeBridge.h`

```cpp
// 修改前
explicit LuaBehaviorTreeBridge(sol::state* luaState, BT::BehaviorTreeFactory* factory);

// 修改后
LuaBehaviorTreeBridge();
```

**文件**: `src/scripting/LuaBehaviorTreeBridge.cpp`

- 移除构造函数中的参数
- 在构造函数内部通过单例获取

### 3.6 更新 MockSimController

**文件**: `include/simulation/MockSimController.h` 和 `src/simulation/MockSimController.cpp`

- 修改 `createScriptManager` 方法：
  ```cpp
  // 修改前
  std::shared_ptr<scripting::EntityScriptManager> createScriptManager(const std::string& entityId);
  // 内部: new EntityScriptManager(entityId, luaState, factory)
  
  // 修改后
  std::shared_ptr<scripting::EntityScriptManager> createScriptManager(const std::string& entityId);
  // 内部: new EntityScriptManager(entityId)
  ```
- 移除 `btFactory_` 成员变量
- 移除 `setBehaviorTreeFactory` 方法

### 3.7 更新 main.cpp

**文件**: `src/main.cpp`

- 修改 BehaviorTreeExecutor 的使用：
  ```cpp
  // 修改前
  g_btExecutor.reset(new BehaviorTreeExecutor());
  if (!g_btExecutor->initialize()) { ... }
  
  // 修改后
  BehaviorTreeExecutor& btExecutor = BehaviorTreeExecutor::getInstance();
  if (!btExecutor.initialize()) { ... }
  ```
- 修改 LuaSimBinding 的初始化（移除 factory 参数）
- 修改 LuaBehaviorTreeBridge 的创建
- 移除 `setBehaviorTreeFactory` 调用

## 4. 修改后的调用流程

```
main()
  ├── LuaSimBinding::getInstance().initialize()
  ├── BehaviorTreeExecutor::getInstance().initialize()
  └── runInteractiveMode()
       └── (用户命令)
            └── simController->createScriptManager(entityId)
                 └── new EntityScriptManager(entityId)  <-- 无参数
                      └── addBTScript()
                           └── new BTScript(...)  <-- 无luaState/factory参数
```

## 5. 实施步骤

1. **修改 BehaviorTreeExecutor 为单例** (h + cpp)
2. **修改 TacticalScript** (h + cpp)
3. **修改 BTScript** (h + cpp)
4. **修改 EntityScriptManager** (h + cpp)
5. **修改 LuaBehaviorTreeBridge** (h + cpp)
6. **更新 MockSimController** (h + cpp)
7. **更新 main.cpp**
8. **编译验证**

## 6. 注意事项

1. **初始化顺序**: 确保 LuaSimBinding 和 BehaviorTreeExecutor 在使用前已初始化
2. **线程安全**: Meyers' singleton 是线程安全的
3. **循环依赖**: 检查是否有循环依赖问题
