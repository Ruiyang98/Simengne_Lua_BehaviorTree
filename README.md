# Lua 仿真控制系统

基于 sol2 和 BehaviorTree.CPP 的 C++ 项目，演示如何使用 Lua 脚本控制仿真引擎。

## 项目概述

本项目提供了一个通过 Lua 脚本控制仿真引擎的框架，包括：

- **SimControlInterface**: 仿真控制的抽象接口（启动、暂停、恢复、停止、重置）
- **MockSimController**: 用于测试和演示的具体实现
- **LuaSimBinding**: 使用 sol2 库的 Lua-C++ 绑定模块
- **示例 Lua 脚本**: 演示基础和高级用法

## 功能特性

- 控制仿真状态（启动、暂停、恢复、停止、重置）
- 查询仿真状态和时间
- 调整仿真速度（时间倍率）
- 事件回调（on_start、on_pause、on_resume、on_stop、on_reset）
- 从文件或字符串执行 Lua 脚本

## 项目结构

```
TestProject/
├── include/
│   ├── simulation/
│   │   ├── SimControlInterface.h    # 仿真控制接口
│   │   └── MockSimController.h      # 模拟实现
│   └── scripting/
│       └── LuaSimBinding.h          # Lua 绑定模块
├── src/
│   ├── main.cpp                     # 主程序入口
│   ├── simulation/
│   │   ├── SimControlInterface.cpp
│   │   ├── MockSimController.cpp
│   │   └── CMakeLists.txt
│   └── scripting/
│       ├── LuaSimBinding.cpp
│       └── CMakeLists.txt
├── scripts/
│   ├── example_control.lua          # 基础示例
│   └── advanced_control.lua         # 高级示例（含回调）
├── 3rdparty/
│   └── lua/                         # Lua 5.1.5 和 sol2
├── cmake/
│   └── lua.cmake                    # Lua CMake 配置
└── CMakeLists.txt                   # 根 CMake 配置
```

## 依赖项

- CMake 3.10+
- 支持 C++11 的编译器
- Lua 5.1.5（已包含在 3rdparty 中）
- sol2 v2.17.5（已包含在 3rdparty 中）

## 构建方法

### Windows

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Linux/macOS

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 使用方法

### 运行演示

```bash
# 运行默认演示
./Release/my_app

# 运行示例脚本
./Release/my_app scripts/example_control.lua

# 运行高级脚本
./Release/my_app scripts/advanced_control.lua
```

### Lua API

通过 `sim` 表提供以下 Lua API：

#### 控制命令
```lua
sim.start()           -- 启动仿真
sim.pause()           -- 暂停仿真
sim.resume()          -- 恢复仿真
sim.stop()            -- 停止仿真
sim.reset()           -- 重置仿真
```

#### 状态查询
```lua
sim.get_state()       -- 获取状态字符串（"STOPPED"、"RUNNING"、"PAUSED"）
sim.is_running()      -- 检查是否运行中
sim.is_paused()       -- 检查是否已暂停
sim.is_stopped()      -- 检查是否已停止
sim.get_time()        -- 获取仿真时间
sim.get_time_step()   -- 获取时间步长
```

#### 速度控制
```lua
sim.set_speed(scale)  -- 设置时间倍率（1.0 = 实时）
sim.get_speed()       -- 获取当前时间倍率
```

#### 事件回调
```lua
sim.on_start(function()
    print("仿真已启动！")
end)

sim.on_pause(function()
    print("仿真已暂停！")
end)

sim.on_resume(function()
    print("仿真已恢复！")
end)

sim.on_stop(function()
    print("仿真已停止！")
end)

sim.on_reset(function()
    print("仿真已重置！")
end)
```

#### 实用函数
```lua
sleep(seconds)        -- 休眠指定秒数
```

## 示例脚本

```lua
-- 示例：基础仿真控制
print("=== 仿真控制演示 ===")

-- 注册回调
sim.on_start(function()
    print("[回调] 仿真已启动！")
end)

-- 启动仿真
sim.start()
sleep(0.5)

-- 检查状态
print("状态: " .. sim.get_state())
print("时间: " .. sim.get_time() .. "秒")

-- 暂停和恢复
sim.pause()
sim.resume()

-- 停止仿真
sim.stop()

print("=== 演示结束 ===")
```

## 架构说明

### SimControlInterface

定义仿真控制操作的抽象接口：

- `start()`、`pause()`、`resume()`、`stop()`、`reset()` - 控制命令
- `getState()`、`isRunning()`、`isPaused()`、`isStopped()` - 状态查询
- `getSimTime()`、`getTimeStep()` - 时间查询
- `setTimeScale()`、`getTimeScale()` - 速度控制
- 事件回调设置器

### MockSimController

`SimControlInterface` 的具体实现，用于测试：
- 模拟时间推进
- 支持自动更新线程
- 提供详细日志输出

### LuaSimBinding

使用 sol2 将 C++ 仿真接口绑定到 Lua：
- 在 Lua 全局命名空间创建 `sim` 表
- 将 C++ 方法包装为 Lua 函数
- 处理带错误保护的 Lua 回调

## 许可证

本项目作为示例实现提供。请查看包含的第三方库的许可证：
- Lua: MIT 许可证
- sol2: MIT 许可证

## 贡献

欢迎 fork 并修改本项目以满足你自己的仿真控制需求。
