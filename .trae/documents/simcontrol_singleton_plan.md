# MockSimController 单例化改造计划

## 目标
1. 将 `MockSimController` 改为单例模式
2. `SimControlInterface::getInstance()` 获取对应的实现单例（MockSimController 单例）
3. 去掉多余的 `getSimController()` 和 `setSimController()` 函数
4. 去掉 `simulation` 命名空间

## 现状分析

### 当前架构
- `SimControlInterface` 是一个抽象基类，定义在 `simulation` 命名空间中
- `MockSimController` 是其实现类
- 通过全局指针 `g_simController` 和辅助函数 `getSimController()` / `setSimController()` 来访问
- 多处代码通过构造函数参数传递 `SimControlInterface*`

### 涉及文件

#### 核心文件（需要修改）
1. `include/simulation/SimControlInterface.h` - 接口定义，添加单例虚函数，去除 namespace
2. `src/simulation/SimControlInterface.cpp` - 实现单例，去除 namespace
3. `include/simulation/MockSimController.h` - 实现类改为单例，去除 namespace
4. `src/simulation/MockSimController.cpp` - 实现单例，去除 namespace
5. `include/behaviortree/SimControllerPtr.h` - 删除文件（不再需要）

#### 使用方文件（需要修改引用）
6. `include/behaviortree/AsyncMoveToPoint.h` - 使用 simulation::VehicleID
7. `include/behaviortree/CheckEntityExists.h` - 使用 SimControlInterface
8. `include/behaviortree/BehaviorTreeExecutor.h` - 构造函数参数
9. `include/scripting/EntityScriptManager.h` - 构造函数参数
10. `include/scripting/LuaSimBinding.h` - 构造函数参数
11. `include/scripting/BTScript.h` - 可能使用
12. `include/scripting/TacticalScript.h` - 可能使用

#### 实现文件（需要修改）
13. `src/behaviortree/AsyncMoveToPoint.cpp`
14. `src/behaviortree/CheckEntityExists.cpp`
15. `src/behaviortree/BehaviorTreeExecutor.cpp`
16. `src/scripting/EntityScriptManager.cpp`
17. `src/scripting/LuaSimBinding.cpp`
18. `src/scripting/BTScript.cpp`
19. `src/scripting/TacticalScript.cpp`

## 改造方案

### 1. SimControlInterface 添加单例接口

在 `SimControlInterface.h` 中添加纯虚函数：
```cpp
// 单例访问接口（由子类实现）
static SimControlInterface* getInstance();
static void setInstance(SimControlInterface* instance);
```

### 2. MockSimController 实现单例

在 `MockSimController.h` 中：
```cpp
class MockSimController : public SimControlInterface {
public:
    // 单例访问
    static MockSimController* getInstance();
    static void setInstance(MockSimController* instance);
    
    // 获取 SimControlInterface 单例（供基类调用）
    static SimControlInterface* getInterfaceInstance();
    
private:
    static MockSimController* instance_;
};
```

### 3. 去除 simulation 命名空间

将所有 `namespace simulation { ... }` 中的内容移到全局命名空间

### 4. 删除 SimControllerPtr.h

该文件中的 `g_simController` 全局指针和辅助函数不再需要

### 5. 修改所有使用方

- 删除构造函数中的 `SimControlInterface*` 参数
- 改为通过 `SimControlInterface::getInstance()` 或 `MockSimController::getInstance()` 获取实例
- 移除 `simulation::` 命名空间前缀

## 实施步骤

### 阶段1：修改核心接口
1. 修改 `SimControlInterface.h` - 添加单例静态方法声明，去除 namespace
2. 修改 `SimControlInterface.cpp` - 实现单例，去除 namespace
3. 修改 `MockSimController.h` - 添加单例支持，去除 namespace
4. 修改 `MockSimController.cpp` - 实现单例，去除 namespace

### 阶段2：删除废弃文件
5. 删除 `SimControllerPtr.h`

### 阶段3：修改使用方头文件
6. 修改 `AsyncMoveToPoint.h`
7. 修改 `CheckEntityExists.h`
8. 修改 `BehaviorTreeExecutor.h`
9. 修改 `EntityScriptManager.h`
10. 修改 `LuaSimBinding.h`
11. 修改 `BTScript.h`（如需要）
12. 修改 `TacticalScript.h`（如需要）

### 阶段4：修改使用方实现文件
13. 修改 `AsyncMoveToPoint.cpp`
14. 修改 `CheckEntityExists.cpp`
15. 修改 `BehaviorTreeExecutor.cpp`
16. 修改 `EntityScriptManager.cpp`
17. 修改 `LuaSimBinding.cpp`
18. 修改 `BTScript.cpp`（如需要）
19. 修改 `TacticalScript.cpp`（如需要）

### 阶段5：修改入口文件
20. 修改 `main.cpp` - 使用单例方式初始化和访问

### 阶段6：验证和测试
21. 检查 examples 目录下的文件（如需要）
22. 编译验证
