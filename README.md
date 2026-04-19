# Lua 仿真控制系统

基于 sol2、BehaviorTree.CPP 的 C++ 项目，演示如何使用 Lua 脚本和行为树控制仿真引擎。

## 项目概述

本项目提供了一个通过 Lua 脚本和行为树控制仿真引擎的框架，包括：

- **SimControlInterface**: 仿真控制的抽象接口（启动、暂停、恢复、停止、重置）
- **MockSimController**: 用于测试和演示的具体实现
- **LuaSimBinding**: 使用 sol2 库的 Lua-C++ 绑定模块
- **BehaviorTreeExecutor**: 行为树执行器，支持从 XML 加载和执行行为树
- **示例 Lua 脚本**: 演示基础和高级用法
- **行为树 XML**: 演示路径移动、巡逻等行为

## 功能特性

### 仿真控制
- 控制仿真状态（启动、暂停、恢复、停止、重置）
- 查询仿真状态和时间
- 调整仿真速度（时间倍率）
- 事件回调（on_start、on_pause、on_resume、on_stop、on_reset）

### 实体管理
- 添加/删除实体
- 移动实体到指定位置
- 查询实体位置和状态
- 获取所有实体列表

### 行为树
- 从 XML 文件加载行为树
- 支持 Blackboard 数据共享
- 内置行为树节点：MoveToPoint、CheckEntityExists、FollowPath
- 支持指定实体 ID 执行行为树

### Lua 脚本
- 从文件或字符串执行 Lua 脚本
- 完整的仿真控制 API
- 实体管理 API

## 项目结构

```
TestProject/
├── include/
│   ├── simulation/
│   │   ├── SimControlInterface.h    # 仿真控制接口
│   │   └── MockSimController.h      # 模拟实现
│   ├── scripting/
│   │   └── LuaSimBinding.h          # Lua 绑定模块
│   └── behaviortree/
│       ├── BehaviorTreeExecutor.h   # 行为树执行器
│       ├── MoveToPoint.h            # 移动到点节点
│       ├── CheckEntityExists.h      # 检查实体存在节点
│       ├── FollowPath.h             # 跟随路径节点
│       └── BlackboardKeys.h         # Blackboard 键名常量
├── src/
│   ├── main.cpp                     # 主程序入口（交互式命令行）
│   ├── simulation/
│   │   ├── SimControlInterface.cpp
│   │   └── MockSimController.cpp
│   ├── scripting/
│   │   └── LuaSimBinding.cpp
│   └── behaviortree/
│       ├── BehaviorTreeExecutor.cpp
│       ├── MoveToPoint.cpp
│       ├── CheckEntityExists.cpp
│       ├── FollowPath.cpp
│       └── EntityMovement.cpp
├── scripts/
│   ├── example_control.lua          # 基础示例
│   ├── advanced_control.lua         # 高级示例（含回调）
│   └── entity_control_test.lua      # 实体控制测试
├── bt_xml/
│   ├── path_movement.xml            # 路径移动行为树
│   ├── square_path.xml              # 正方形路径
│   ├── square_path_composite.xml    # 组合路径行为树
│   └── waypoint_patrol.xml          # 巡逻行为树
├── 3rdparty/
│   ├── lua/                         # Lua 5.1.5 和 sol2
│   └── BehaviorTree.CPP/            # BehaviorTree.CPP 库
└── CMakeLists.txt                   # 根 CMake 配置
```

## 依赖项

- CMake 3.10+
- 支持 C++11 的编译器
- Lua 5.1.5（已包含在 3rdparty 中）
- sol2 v2.17.5（已包含在 3rdparty 中）
- BehaviorTree.CPP v3.8（已包含在 3rdparty 中）

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

### 运行交互式命令行

```bash
./Release/my_app
```

进入交互式模式后，可以使用以下命令：

```
> help                        # 显示帮助信息
> entity                      # 列出所有实体
> entity add npc 10 20        # 添加实体（类型 x y [z]）
> bt <xml_file> [tree_name] [-e <entity_id>]  # 执行行为树
> bt list                     # 列出可用行为树
> lua <script_path>           # 执行 Lua 脚本
> quit/exit                   # 退出程序
```

### 行为树执行示例

```bash
# 列出所有可用行为树
> bt list

# 执行行为树（自动使用第一个实体或创建新实体）
> bt bt_xml/square_path_composite.xml SquarePathComposite

# 执行行为树并指定实体 ID
> bt bt_xml/square_path_composite.xml SquarePathComposite -e npc_001

# 先添加实体，再执行行为树
> entity add npc 0 0
> bt bt_xml/square_path.xml SquarePath -e npc_0
```

### 运行 Lua 脚本

```bash
# 运行示例脚本
./Release/my_app scripts/example_control.lua

# 运行高级脚本
./Release/my_app scripts/advanced_control.lua

# 运行实体控制测试
./Release/my_app scripts/entity_control_test.lua
```

## Lua API

通过 `sim` 表提供以下 Lua API：

### 仿真控制命令
```lua
sim.start()           -- 启动仿真
sim.pause()           -- 暂停仿真
sim.resume()          -- 恢复仿真
sim.stop()            -- 停止仿真
sim.reset()           -- 重置仿真
```

### 仿真状态查询
```lua
sim.get_state()       -- 获取状态字符串（"STOPPED"、"RUNNING"、"PAUSED"）
sim.is_running()      -- 检查是否运行中
sim.is_paused()       -- 检查是否已暂停
sim.is_stopped()      -- 检查是否已停止
sim.get_time()        -- 获取仿真时间
sim.get_time_step()   -- 获取时间步长
```

### 仿真速度控制
```lua
sim.set_speed(scale)  -- 设置时间倍率（1.0 = 实时）
sim.get_speed()       -- 获取当前时间倍率
```

### 实体管理
```lua
-- 添加实体
local entity_id = sim.add_entity(type, x, y, z)

-- 删除实体
local success = sim.remove_entity(entity_id)

-- 移动实体
local success = sim.move_entity(entity_id, x, y, z)

-- 获取实体位置
local pos = sim.get_entity_position(entity_id)
-- pos.x, pos.y, pos.z

-- 获取所有实体
local entities = sim.get_all_entities()
-- entities[i].id, entities[i].type, entities[i].x, entities[i].y, entities[i].z

-- 获取实体数量
local count = sim.get_entity_count()
```

### 事件回调
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

### 实用函数
```lua
sleep(seconds)        -- 休眠指定秒数
```

## 行为树 XML 格式

### 基本结构

```xml
<?xml version="1.0"?>
<root main_tree_to_execute="MainTree">
    <BehaviorTree ID="MainTree">
        <Sequence>
            <CheckEntityExists entity_id="{entity_id}" />
            <MoveToPoint entity_id="{entity_id}" x="10" y="0" z="0" />
        </Sequence>
    </BehaviorTree>
</root>
```

### Blackboard 引用

使用 `{key}` 语法引用 Blackboard 中的变量：

```xml
<MoveToPoint entity_id="{entity_id}" x="10" y="0" z="0" />
```

### 内置行为树节点

#### MoveToPoint
移动实体到指定坐标。

```xml
<MoveToPoint entity_id="{entity_id}" x="10.0" y="20.0" z="0.0" />
```

**参数：**
- `entity_id` (string): 实体 ID
- `x` (double): 目标 X 坐标
- `y` (double): 目标 Y 坐标
- `z` (double, 可选): 目标 Z 坐标，默认 0.0

#### CheckEntityExists
检查实体是否存在。

```xml
<CheckEntityExists entity_id="{entity_id}" />
```

**参数：**
- `entity_id` (string): 实体 ID

**返回值：**
- SUCCESS: 实体存在
- FAILURE: 实体不存在

#### FollowPath
使实体沿路径点移动。

```xml
<FollowPath entity_id="{entity_id}" />
```

**参数：**
- `entity_id` (string): 实体 ID

**Blackboard 输入：**
- `waypoints`: 路径点列表（在 Blackboard 中设置）

## 示例

### Lua 脚本示例

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

### 实体控制示例

```lua
-- 添加实体
local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("添加实体: " .. npc_id)

-- 移动实体
sim.move_entity(npc_id, 10.0, 20.0, 0.0)

-- 获取位置
local pos = sim.get_entity_position(npc_id)
print("位置: (" .. pos.x .. ", " .. pos.y .. ", " .. pos.z .. ")")

-- 删除实体
sim.remove_entity(npc_id)
```

## 架构说明

### SimControlInterface

定义仿真控制操作的抽象接口：

- `start()`、`pause()`、`resume()`、`stop()`、`reset()` - 控制命令
- `getState()`、`isRunning()`、`isPaused()`、`isStopped()` - 状态查询
- `getSimTime()`、`getTimeStep()` - 时间查询
- `setTimeScale()`、`getTimeScale()` - 速度控制
- `addEntity()`、`removeEntity()`、`moveEntity()`、`getEntityPosition()`、`getAllEntities()` - 实体管理
- 事件回调设置器

### MockSimController

`SimControlInterface` 的具体实现，用于测试：
- 模拟时间推进
- 支持自动更新线程
- 提供详细日志输出
- 管理实体状态

### LuaSimBinding

使用 sol2 将 C++ 仿真接口绑定到 Lua：
- 在 Lua 全局命名空间创建 `sim` 表
- 将 C++ 方法包装为 Lua 函数
- 处理带错误保护的 Lua 回调

### BehaviorTreeExecutor

行为树执行器：
- 从 XML 文件加载行为树
- 注册自定义行为树节点
- 执行行为树并管理 Blackboard
- 与仿真控制器集成

## 许可证

本项目作为示例实现提供。请查看包含的第三方库的许可证：
- Lua: MIT 许可证
- sol2: MIT 许可证
- BehaviorTree.CPP: MIT 许可证

## 贡献

欢迎 fork 并修改本项目以满足你自己的仿真控制需求。
