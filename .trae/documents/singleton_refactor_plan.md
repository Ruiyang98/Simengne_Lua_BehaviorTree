# BehaviorTreeFactory、LuaSimBinding 改为单例模式计划

## 目标

将 `BehaviorTreeFactory` 和 `LuaSimBinding` 改为单例模式，确保全局唯一实例。

## 注意

- `BehaviorTreeFactory` 来自第三方库 BehaviorTree.CPP，不能直接修改，需要通过包装器实现单例
- `LuaSimBinding` 可以修改其实现为单例模式

## 架构设计

### 1. BehaviorTreeFactory 单例包装器

由于 `BT::BehaviorTreeFactory` 是第三方类，创建单例包装器：

```cpp
class BehaviorTreeFactorySingleton {
public:
    static BT::BehaviorTreeFactory& getInstance();
    
    // 删除拷贝和赋值
    BehaviorTreeFactorySingleton(const BehaviorTreeFactorySingleton&) = delete;
    BehaviorTreeFactorySingleton& operator=(const BehaviorTreeFactorySingleton&) = delete;

private:
    BehaviorTreeFactorySingleton();
    ~BehaviorTreeFactorySingleton();
};
```

### 2. LuaSimBinding 单例改造

将 `LuaSimBinding` 改为单例模式：

```cpp
class LuaSimBinding {
public:
    static LuaSimBinding& getInstance();
    
    // 初始化方法（替代构造函数参数）
    bool initialize(simulation::SimControlInterface* simInterface);
    
    // 删除拷贝和赋值
    LuaSimBinding(const LuaSimBinding&) = delete;
    LuaSimBinding& operator=(const LuaSimBinding&) = delete;

private:
    LuaSimBinding();
    ~LuaSimBinding();
    
    // 原有成员...
};
```

## 实现步骤

### Phase 1: 创建 BehaviorTreeFactory 单例包装器

1. 创建新文件 `include/behaviortree/BehaviorTreeFactorySingleton.h`
2. 创建新文件 `src/behaviortree/BehaviorTreeFactorySingleton.cpp`
3. 实现单例逻辑（Meyer's Singleton 或静态成员变量）

### Phase 2: 改造 LuaSimBinding 为单例

1. 修改 `include/scripting/LuaSimBinding.h`：
   - 构造函数改为 private
   - 添加 `getInstance()` 静态方法
   - 添加 `initialize()` 方法接收 simInterface
   - 删除拷贝构造函数和赋值操作符

2. 修改 `src/scripting/LuaSimBinding.cpp`：
   - 实现单例逻辑
   - 修改构造函数逻辑到 initialize 方法

### Phase 3: 更新 BehaviorTreeExecutor

1. 修改 `include/behaviortree/BehaviorTreeExecutor.h`：
   - 移除 `factory_` 成员变量
   - 修改 `getFactory()` 返回单例引用

2. 修改 `src/behaviortree/BehaviorTreeExecutor.cpp`：
   - 所有使用 `factory_` 的地方改为使用单例

### Phase 4: 更新 main.cpp

1. 移除 `g_btExecutor` 全局变量中的 factory 相关逻辑
2. 使用 `LuaSimBinding::getInstance()` 获取实例
3. 更新所有使用 LuaSimBinding 的地方

### Phase 5: 更新其他依赖代码

1. 更新 `EntityScriptManager` 获取 factory 的方式
2. 更新 `LuaBehaviorTreeBridge` 获取 factory 的方式
3. 更新所有调用 `g_btExecutor->getFactory()` 的地方

### Phase 6: 测试验证

1. 编译项目
2. 运行测试确保功能正常
3. 验证单例行为正确

## 代码示例

### BehaviorTreeFactorySingleton.h

```cpp
#ifndef BEHAVIOR_TREE_FACTORY_SINGLETON_H
#define BEHAVIOR_TREE_FACTORY_SINGLETON_H

#include <behaviortree_cpp_v3/bt_factory.h>

namespace behaviortree {

class BehaviorTreeFactorySingleton {
public:
    // 获取全局唯一实例
    static BT::BehaviorTreeFactory& getInstance();
    
    // 删除拷贝构造和赋值
    BehaviorTreeFactorySingleton(const BehaviorTreeFactorySingleton&) = delete;
    BehaviorTreeFactorySingleton& operator=(const BehaviorTreeFactorySingleton&) = delete;

private:
    BehaviorTreeFactorySingleton() = default;
    ~BehaviorTreeFactorySingleton() = default;
};

} // namespace behaviortree

#endif // BEHAVIOR_TREE_FACTORY_SINGLETON_H
```

### BehaviorTreeFactorySingleton.cpp

```cpp
#include "behaviortree/BehaviorTreeFactorySingleton.h"

namespace behaviortree {

BT::BehaviorTreeFactory& BehaviorTreeFactorySingleton::getInstance() {
    static BT::BehaviorTreeFactory instance;
    return instance;
}

} // namespace behaviortree
```

### LuaSimBinding.h (修改后)

```cpp
#ifndef LUA_SIM_BINDING_H
#define LUA_SIM_BINDING_H

#include "simulation/SimControlInterface.h"
#include <behaviortree_cpp_v3/bt_factory.h>
#include <sol.hpp>
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace scripting {

class LuaBehaviorTreeBridge;

class LuaSimBinding {
public:
    // 获取全局唯一实例
    static LuaSimBinding& getInstance();
    
    // 初始化（替代原构造函数参数）
    bool initialize(simulation::SimControlInterface* simInterface);
    
    // 删除拷贝构造和赋值
    LuaSimBinding(const LuaSimBinding&) = delete;
    LuaSimBinding& operator=(const LuaSimBinding&) = delete;

    // 原有公共接口保持不变...
    bool executeScript(const std::string& scriptPath);
    bool executeString(const std::string& scriptCode);
    sol::state& getState();
    bool isInitialized() const;
    const std::string& getLastError() const;
    
    LuaBehaviorTreeBridge* getBehaviorTreeBridge() const { return btBridge_.get(); }
    bool initializeBehaviorTree(BT::BehaviorTreeFactory* factory);
    bool isBehaviorTreeInitialized() const { return btBridge_ != nullptr; }

private:
    // 私有构造函数
    LuaSimBinding();
    ~LuaSimBinding();
    
    void registerFunctions();
    void registerSimAPI();
    void registerUtilityFunctions();
    void setupCallbacks();

    simulation::SimControlInterface* simInterface_;
    std::unique_ptr<sol::state> luaState_;
    std::unique_ptr<LuaBehaviorTreeBridge> btBridge_;
    bool initialized_;
    std::string lastError_;
    std::vector<sol::protected_function> luaCallbacks_;
};

} // namespace scripting

#endif // LUA_SIM_BINDING_H
```

## 影响范围

### 需要修改的文件

1. **新增文件**:
   - `include/behaviortree/BehaviorTreeFactorySingleton.h`
   - `src/behaviortree/BehaviorTreeFactorySingleton.cpp`

2. **修改文件**:
   - `include/scripting/LuaSimBinding.h`
   - `src/scripting/LuaSimBinding.cpp`
   - `include/behaviortree/BehaviorTreeExecutor.h`
   - `src/behaviortree/BehaviorTreeExecutor.cpp`
   - `src/main.cpp`
   - `src/scripting/LuaBehaviorTreeBridge.cpp` (可能)
   - `include/scripting/EntityScriptManager.h` (可能)
   - `src/scripting/EntityScriptManager.cpp` (可能)

3. **CMakeLists.txt**:
   - 添加新源文件到 `src/behaviortree/CMakeLists.txt`

## 注意事项

1. **线程安全**: Meyer's Singleton 在 C++11 及以上是线程安全的
2. **初始化顺序**: 确保单例在使用前已初始化
3. **生命周期**: 单例在程序结束时自动销毁
4. **测试**: 需要验证所有原有功能正常工作
