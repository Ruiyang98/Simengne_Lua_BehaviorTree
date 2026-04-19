# Lua 与行为树仿真控制系统

基于 sol2、BehaviorTree.CPP 的 C++ 项目，演示如何使用 Lua 脚本和行为树控制仿真引擎。

## 项目概述

本项目提供了一个通过 Lua 脚本和行为树控制仿真引擎的完整框架，包括：

- **SimControlInterface**: 仿真控制的抽象接口（启动、暂停、恢复、停止、重置）
- **MockSimController**: 用于测试和演示的具体实现
- **LuaSimBinding**: 使用 sol2 库的 Lua-C++ 绑定模块
- **LuaBehaviorTreeBridge**: Lua 与行为树的桥接模块，支持从 Lua 控制行为树
- **BehaviorTreeExecutor**: 行为树执行器，支持从 XML 加载和执行行为树
- **示例 Lua 脚本**: 演示基础、高级和 Lua-行为树集成用法
- **行为树 XML**: 演示路径移动、巡逻、Lua 自定义节点等行为

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
- **Lua 自定义节点**：注册 Lua 函数作为行为树节点

### Lua 与行为树集成
- 从 Lua 加载和执行行为树
- 通过黑板传递参数给行为树
- 读取和修改行为树黑板数据
- 注册 Lua 函数作为动作节点和条件节点
- Lua 节点与 C++ 节点混合使用

### Lua 脚本
- 从文件或字符串执行 Lua 脚本
- 完整的仿真控制 API
- 实体管理 API
- 行为树控制 API

## 项目结构

```
TestProject/
├── include/
│   ├── simulation/
│   │   ├── SimControlInterface.h    # 仿真控制接口
│   │   └── MockSimController.h      # 模拟实现
│   ├── scripting/
│   │   ├── LuaSimBinding.h          # Lua 绑定模块
│   │   └── LuaBehaviorTreeBridge.h  # Lua-行为树桥接器
│   └── behaviortree/
│       ├── BehaviorTreeExecutor.h   # 行为树执行器
│       ├── MoveToPoint.h            # 移动到点节点
│       ├── CheckEntityExists.h      # 检查实体存在节点
│       ├── FollowPath.h             # 跟随路径节点
│       ├── LuaActionNode.h          # Lua 动作节点
│       ├── LuaConditionNode.h       # Lua 条件节点
│       └── BlackboardKeys.h         # Blackboard 键名常量
├── src/
│   ├── main.cpp                     # 主程序入口（交互式命令行）
│   ├── simulation/
│   │   ├── SimControlInterface.cpp
│   │   └── MockSimController.cpp
│   ├── scripting/
│   │   ├── LuaSimBinding.cpp
│   │   └── LuaBehaviorTreeBridge.cpp
│   └── behaviortree/
│       ├── BehaviorTreeExecutor.cpp
│       ├── MoveToPoint.cpp
│       ├── CheckEntityExists.cpp
│       ├── FollowPath.cpp
│       └── EntityMovement.cpp
├── scripts/                         # Lua 示例脚本
│   ├── example_control.lua          # 基础仿真控制示例
│   ├── advanced_control.lua         # 高级仿真控制示例
│   ├── entity_control_test.lua      # 实体控制测试
│   ├── bt_control_example.lua       # 行为树基础控制示例
│   ├── bt_blackboard_example.lua    # 黑板操作示例
│   ├── bt_custom_node_example.lua   # Lua 自定义节点示例
│   ├── bt_advanced_example.lua      # Lua-行为树高级综合示例
│   ├── bt_custom_node_from_file.lua # 从文件加载 XML 示例
│   └── load_lua_nodes_xml.lua       # 单独加载 XML 示例
├── bt_xml/                          # 行为树 XML 文件
│   ├── path_movement.xml            # 路径移动行为树
│   ├── square_path.xml              # 正方形路径
│   ├── square_path_composite.xml    # 组合路径行为树
│   ├── waypoint_patrol.xml          # 巡逻行为树
│   └── lua_custom_nodes_example.xml # Lua 自定义节点示例
├── docs/                            # 文档
│   ├── lua_behavior_tree_examples.md    # Lua 示例脚本详解
│   └── behavior_tree_xml_reference.md   # XML 行为树参考
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
> lua-bt                      # 列出 Lua+行为树集成示例
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

### Lua 脚本执行示例

```bash
# 运行仿真控制示例
> lua scripts/example_control.lua

# 运行实体控制测试
> lua scripts/entity_control_test.lua

# 运行行为树基础示例
> lua scripts/bt_control_example.lua

# 运行黑板操作示例
> lua scripts/bt_blackboard_example.lua

# 运行 Lua 自定义节点示例
> lua scripts/bt_custom_node_example.lua

# 运行高级综合示例
> lua scripts/bt_advanced_example.lua
```

### 查看 Lua+行为树集成示例列表

```bash
> lua-bt
```

## Lua API 参考

### 仿真控制 API（sim 表）

```lua
-- 控制命令
sim.start()           -- 启动仿真
sim.pause()           -- 暂停仿真
sim.resume()          -- 恢复仿真
sim.stop()            -- 停止仿真
sim.reset()           -- 重置仿真

-- 状态查询
sim.get_state()       -- 获取状态字符串
sim.is_running()      -- 检查是否运行中
sim.is_paused()       -- 检查是否已暂停
sim.is_stopped()      -- 检查是否已停止
sim.get_time()        -- 获取仿真时间
sim.get_time_step()   -- 获取时间步长

-- 速度控制
sim.set_speed(scale)  -- 设置时间倍率
sim.get_speed()       -- 获取当前时间倍率

-- 实体管理
local entity_id = sim.add_entity(type, x, y, z)
sim.remove_entity(entity_id)
sim.move_entity(entity_id, x, y, z)
local pos = sim.get_entity_position(entity_id)  -- pos.x, pos.y, pos.z
local entities = sim.get_all_entities()         -- entities[i].id, type, x, y, z
local count = sim.get_entity_count()

-- 事件回调
sim.on_start(callback)
sim.on_pause(callback)
sim.on_resume(callback)
sim.on_stop(callback)
sim.on_reset(callback)

-- 实用函数
sleep(seconds)        -- 休眠指定秒数
```

### 行为树 API（bt 表）

```lua
-- 加载行为树
bt.load_file(xml_path)           -- 从 XML 文件加载
bt.load_text(xml_string)         -- 从 XML 字符串加载

-- 执行行为树
bt.execute(tree_name)                            -- 无参数执行
bt.execute(tree_name, entity_id)                 -- 指定实体
bt.execute(tree_name, entity_id, params)         -- 带黑板参数

-- 示例：带参数执行
local tree_id = bt.execute("MainTree", entity_id, {
    waypoints = "0,0,0;10,0,0;10,10,0;0,10,0",
    delay_ms = 200
})

-- 黑板操作
bt.get_blackboard(tree_id, key)        -- 读取黑板值
bt.set_blackboard(tree_id, key, value) -- 设置黑板值

-- 状态管理
bt.get_status(tree_id)     -- 获取执行状态（"SUCCESS"/"FAILURE"/"RUNNING"）
bt.has_tree(tree_id)       -- 检查树是否存在
bt.stop(tree_id)           -- 停止行为树

-- 自定义节点注册
bt.register_action(name, function)
bt.register_condition(name, function)

-- 示例：注册 Lua 动作节点
bt.register_action("MyAction", function()
    print("执行自定义动作")
    return "SUCCESS"  -- 或 "FAILURE"
end)

-- 示例：注册 Lua 条件节点
bt.register_condition("MyCondition", function()
    return sim.get_entity_count() > 0  -- 返回 true 或 false
end)
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
<MoveToPoint entity_id="{entity_id}" x="{target_x}" y="{target_y}" z="0" />
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

**返回值：**
- SUCCESS: 实体存在
- FAILURE: 实体不存在

#### FollowPath
使实体沿路径点移动。

```xml
<FollowPath entity_id="{entity_id}" waypoints="{waypoints}" delay_ms="{delay_ms}" />
```

**参数：**
- `entity_id` (string): 实体 ID
- `waypoints` (string): 路径点序列，格式："x1,y1,z1;x2,y2,z2;..."
- `delay_ms` (int): 每步移动的延迟（毫秒）

#### LuaAction / LuaCondition
调用 Lua 注册的自定义节点。

```xml
<LuaAction lua_node_name="MyLuaAction" />
<LuaCondition lua_node_name="MyLuaCondition" />
```

**注意：** 使用前必须先通过 `bt.register_action()` 或 `bt.register_condition()` 注册对应的 Lua 函数。

## 示例详解

### 示例 1：基础仿真控制

```lua
-- example_control.lua
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

### 示例 2：行为树基础控制

```lua
-- bt_control_example.lua
-- 加载行为树
bt.load_file("bt_xml/square_path.xml")

-- 创建实体
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 执行行为树
local tree_id = bt.execute("SquarePath", entity_id)
print("执行状态: " .. bt.get_status(tree_id))
```

### 示例 3：黑板数据传递

```lua
-- bt_blackboard_example.lua
bt.load_file("bt_xml/path_movement.xml")

local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 通过黑板传递参数
local tree_id = bt.execute("MainTree", entity_id, {
    waypoints = "0,0,0;5,0,0;5,5,0;0,5,0;0,0,0",
    delay_ms = 200
})

-- 读取黑板值
local waypoints = bt.get_blackboard(tree_id, "waypoints")
print("路径点: " .. tostring(waypoints))

-- 动态修改黑板
bt.set_blackboard(tree_id, "custom_value", 42)
```

### 示例 4：Lua 自定义节点

```lua
-- bt_custom_node_example.lua

-- 注册 Lua 条件节点
bt.register_condition("HasEntities", function()
    return sim.get_entity_count() > 0
end)

-- 注册 Lua 动作节点
bt.register_action("Patrol", function()
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        sim.move_entity(entity.id, entity.x + 1, entity.y + 1, 0)
    end
    return "SUCCESS"
end)

-- 创建使用 Lua 节点的行为树
local xml = [[
<root main_tree_to_execute="TestTree">
    <BehaviorTree ID="TestTree">
        <Sequence>
            <LuaCondition lua_node_name="HasEntities" />
            <LuaAction lua_node_name="Patrol" />
        </Sequence>
    </BehaviorTree>
</root>
]]

bt.load_text(xml)
bt.execute("TestTree")
```

### 示例 5：高级综合应用

```lua
-- bt_advanced_example.lua
-- 批量加载行为树
local bt_files = {
    "bt_xml/path_movement.xml",
    "bt_xml/square_path.xml",
    "bt_xml/waypoint_patrol.xml"
}

for _, file in ipairs(bt_files) do
    bt.load_file(file)
end

-- 注册复杂的 Lua 节点
bt.register_action("SmartPatrol", function()
    -- 查找守卫实体
    local target = nil
    for _, entity in ipairs(sim.get_all_entities()) do
        if entity.type == "guard" then
            target = entity
            break
        end
    end
    
    if not target then return "FAILURE" end
    
    -- 执行巡逻
    local moves = {{x=5,y=0}, {x=0,y=5}, {x=-5,y=0}, {x=0,y=-5}}
    for _, move in ipairs(moves) do
        sim.move_entity(target.id, target.x + move.x, target.y + move.y, 0)
        sleep(0.1)
    end
    
    return "SUCCESS"
end)

-- 多实体协调
local guard = sim.add_entity("guard", 0.0, 0.0, 0.0)
local npc1 = sim.add_entity("npc", 20.0, 20.0, 0.0)
local npc2 = sim.add_entity("npc", 30.0, 30.0, 0.0)

-- 为不同实体执行不同行为树
bt.execute("SquarePath", guard)
bt.execute("LargeSquarePath", npc1)
bt.execute("WaypointPatrol", npc2)
```

## 文档资源

- [Lua 示例脚本详解](docs/lua_behavior_tree_examples.md) - 详细解释所有 Lua 示例脚本
- [XML 行为树参考](docs/behavior_tree_xml_reference.md) - 所有行为树 XML 文件的完整参考

## 架构说明

### SimControlInterface

定义仿真控制操作的抽象接口：
- 控制命令：`start()`、`pause()`、`resume()`、`stop()`、`reset()`
- 状态查询：`getState()`、`isRunning()`、`isPaused()`、`isStopped()`
- 时间管理：`getSimTime()`、`getTimeStep()`、`setTimeScale()`、`getTimeScale()`
- 实体管理：`addEntity()`、`removeEntity()`、`moveEntity()`、`getEntityPosition()`、`getAllEntities()`
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
- 支持行为树桥接器初始化

### LuaBehaviorTreeBridge

Lua 与 BehaviorTree.CPP 的桥接模块：
- 注册 Lua 函数作为行为树节点
- 提供行为树控制 API（加载、执行、停止）
- 支持黑板数据读写
- 管理行为树生命周期

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
