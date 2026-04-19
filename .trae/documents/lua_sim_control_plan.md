# Lua 仿真控制接口实现计划

## 概述

本项目旨在实现一个基于 Lua 脚本的仿真引擎控制系统。通过 Lua 脚本解析和控制仿真引擎的启动、暂停、结束等操作。

## 项目现状

- 项目已集成 Lua 5.1.5 和 sol2 (v2.17.5) 作为脚本引擎
- 已配置 CMake 构建系统，包含 lua.cmake 用于 Lua 库链接
- 项目依赖 BehaviorTree.CPP 库
- 当前只有一个空的 main.cpp

## 目标

1. 创建仿真控制接口 (SimControlInterface)
2. 实现 Lua 脚本引擎绑定
3. 提供 Lua API 用于控制仿真引擎
4. 创建示例 Lua 脚本

## 详细实现步骤

### 阶段 1: 创建核心接口文件

#### 1.1 创建 SimControlInterface.h
**路径**: `include/simulation/SimControlInterface.h`

**功能**:
- 定义仿真引擎控制接口
- 提供纯虚函数接口：start(), pause(), resume(), stop(), reset()
- 提供状态查询接口：isRunning(), isPaused(), getSimTime()
- 提供事件回调机制

**关键设计**:
```cpp
class SimControlInterface {
public:
    virtual ~SimControlInterface() = default;
    
    // 控制命令
    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop() = 0;
    virtual bool reset() = 0;
    
    // 状态查询
    virtual bool isRunning() const = 0;
    virtual bool isPaused() const = 0;
    virtual double getSimTime() const = 0;
    
    // 设置仿真速度倍率
    virtual void setTimeScale(double scale) = 0;
    virtual double getTimeScale() const = 0;
};
```

#### 1.2 创建 SimControlInterface.cpp
**路径**: `src/simulation/SimControlInterface.cpp`

**功能**:
- 实现接口的基础功能
- 提供默认实现或抽象基类实现

---

### 阶段 2: 创建 Lua 绑定模块

#### 2.1 创建 LuaSimBinding.h
**路径**: `include/scripting/LuaSimBinding.h`

**功能**:
- 提供 Lua 与 C++ 的绑定功能
- 使用 sol2 库进行绑定
- 注册仿真控制函数到 Lua 环境

**关键设计**:
```cpp
class LuaSimBinding {
public:
    explicit LuaSimBinding(SimControlInterface* simInterface);
    ~LuaSimBinding();
    
    // 初始化 Lua 环境并注册函数
    bool initialize();
    
    // 执行 Lua 脚本文件
    bool executeScript(const std::string& scriptPath);
    
    // 执行 Lua 脚本字符串
    bool executeString(const std::string& scriptCode);
    
    // 获取 Lua 状态
    sol::state& getState();
    
private:
    void registerFunctions();
    
    SimControlInterface* simInterface_;
    std::unique_ptr<sol::state> luaState_;
};
```

#### 2.2 创建 LuaSimBinding.cpp
**路径**: `src/scripting/LuaSimBinding.cpp`

**功能**:
- 实现 sol2 绑定逻辑
- 将 SimControlInterface 的方法绑定到 Lua
- 提供错误处理和日志输出

**Lua API 设计**:
```lua
-- 仿真控制 API
sim.start()           -- 启动仿真
sim.pause()           -- 暂停仿真
sim.resume()          -- 恢复仿真
sim.stop()            -- 停止仿真
sim.reset()           -- 重置仿真

-- 状态查询 API
sim.is_running()      -- 检查是否运行中
sim.is_paused()       -- 检查是否暂停
sim.get_time()        -- 获取仿真时间

-- 速度控制 API
sim.set_speed(scale)  -- 设置仿真速度倍率
sim.get_speed()       -- 获取当前速度倍率

-- 事件回调
sim.on_start(function() ... end)
sim.on_pause(function() ... end)
sim.on_resume(function() ... end)
sim.on_stop(function() ... end)
```

---

### 阶段 3: 创建示例实现

#### 3.1 创建 MockSimController.h/.cpp
**路径**: 
- `include/simulation/MockSimController.h`
- `src/simulation/MockSimController.cpp`

**功能**:
- 提供 SimControlInterface 的具体实现
- 用于测试和演示
- 模拟仿真引擎的行为

---

### 阶段 4: 创建示例脚本

#### 4.1 创建示例 Lua 脚本
**路径**: `scripts/example_control.lua`

**功能**:
- 演示如何使用 Lua API 控制仿真
- 包含基本的控制流程示例

**示例内容**:
```lua
-- 示例：仿真控制脚本
print("=== 仿真控制示例 ===")

-- 启动仿真
print("启动仿真...")
sim.start()

-- 等待一段时间（模拟）
print("仿真运行中...")

-- 检查状态
if sim.is_running() then
    print("仿真正在运行，当前时间: " .. sim.get_time())
end

-- 暂停仿真
print("暂停仿真...")
sim.pause()

-- 恢复仿真
print("恢复仿真...")
sim.resume()

-- 停止仿真
print("停止仿真...")
sim.stop()

print("=== 示例结束 ===")
```

#### 4.2 创建高级示例脚本
**路径**: `scripts/advanced_control.lua`

**功能**:
- 演示事件回调机制
- 演示速度控制
- 演示条件控制流程

---

### 阶段 5: 更新 CMake 配置

#### 5.1 更新根 CMakeLists.txt
**修改内容**:
- 添加新的子目录
- 创建 script_system 库
- 创建 simulation 库
- 链接 Lua 库到相应目标

**结构**:
```cmake
# 添加子目录
add_subdirectory(src/script_system)
add_subdirectory(src/simulation)

# 主可执行文件
target_link_libraries(my_app PRIVATE
    bt_adapters
    script_system
    simulation
)
```

#### 5.2 创建 src/script_system/CMakeLists.txt
**功能**:
- 定义 script_system 库
- 包含 Lua 绑定相关源文件
- 链接 Lua 和 sol2

#### 5.3 创建 src/simulation/CMakeLists.txt
**功能**:
- 定义 simulation 库
- 包含仿真控制接口实现

---

### 阶段 6: 更新 main.cpp

**功能**:
- 创建 MockSimController 实例
- 创建 LuaSimBinding 实例
- 加载并执行示例 Lua 脚本
- 展示完整的调用流程

---

## 文件结构

```
TestProject/
├── include/
│   ├── simulation/
│   │   └── SimControlInterface.h
│   │   └── MockSimController.h
│   └── scripting/
│       └── LuaSimBinding.h
├── src/
│   ├── main.cpp
│   ├── simulation/
│   │   ├── SimControlInterface.cpp
│   │   ├── MockSimController.cpp
│   │   └── CMakeLists.txt
│   └── scripting/
│       ├── LuaSimBinding.cpp
│       └── CMakeLists.txt
├── scripts/
│   ├── example_control.lua
│   └── advanced_control.lua
├── CMakeLists.txt
└── cmake/
    └── lua.cmake
```

## 技术要点

### 1. sol2 绑定技术
- 使用 `sol::state` 创建 Lua 环境
- 使用 `new_usertype` 注册 C++ 类到 Lua
- 使用 `set_function` 注册全局函数
- 使用 `protected_function` 进行安全调用

### 2. 错误处理
- 使用 sol2 的异常处理机制
- 提供详细的错误日志
- 确保 Lua 错误不会导致程序崩溃

### 3. 线程安全
- 考虑仿真引擎可能在单独线程运行
- 使用适当的同步机制
- Lua 状态机不是线程安全的，需要保护

### 4. 内存管理
- 使用智能指针管理资源
- 正确处理 Lua  userdata 生命周期
- 避免循环引用

## 验证步骤

1. 编译项目无错误
2. 运行程序，Lua 脚本正确加载
3. 仿真控制命令按预期执行
4. 状态查询返回正确结果
5. 事件回调正常工作

## 扩展建议

1. **行为树集成**: 将 Lua 脚本控制与 BehaviorTree.CPP 结合
2. **更多 API**: 添加获取/设置仿真参数、查询实体状态等功能
3. **热重载**: 支持 Lua 脚本热重载，无需重启程序
4. **调试支持**: 添加 Lua 调试接口
