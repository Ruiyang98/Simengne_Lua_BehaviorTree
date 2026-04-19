# Lua 与行为树集成示例文档

本文档详细介绍了 `scripts/` 目录下的 Lua 行为树集成示例脚本。

---

## 目录

1. [bt_control_example.lua - 基础控制示例](#bt_control_example)
2. [bt_blackboard_example.lua - 黑板操作示例](#bt_blackboard_example)
3. [bt_custom_node_example.lua - 自定义节点示例](#bt_custom_node_example)
4. [bt_advanced_example.lua - 高级综合示例](#bt_advanced_example)
5. [bt_custom_node_from_file.lua - 从文件加载示例](#bt_custom_node_from_file)
6. [load_lua_nodes_xml.lua - 单独加载XML示例](#load_lua_nodes_xml)

---

## bt_control_example

**文件**: `scripts/bt_control_example.lua`

### 功能概述

演示行为树的基础加载和执行功能，包括：
- 从 XML 文件加载行为树
- 创建实体并执行行为树
- 查询行为树执行状态

### 核心代码解析

```lua
-- 1. 加载行为树
local success = bt.load_file("bt_xml/square_path.xml")

-- 2. 创建实体
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 3. 执行行为树
local tree_id = bt.execute("SquarePath", entity_id)

-- 4. 查询状态
local status = bt.get_status(tree_id)
```

### 提供的 API

| API | 功能 | 返回值 |
|-----|------|--------|
| `bt.load_file(xml_path)` | 加载 XML 行为树 | bool |
| `bt.execute(tree_name, entity_id)` | 执行行为树 | string (tree_id) |
| `bt.get_status(tree_id)` | 获取执行状态 | "SUCCESS"/"FAILURE"/"RUNNING" |
| `bt.has_tree(tree_id)` | 检查树是否存在 | bool |

---

## bt_blackboard_example

**文件**: `scripts/bt_blackboard_example.lua`

### 什么是黑板？

黑板是行为树中用于**存储和共享数据**的键值对存储系统，允许：
- Lua 脚本向行为树传递参数
- 行为树节点之间共享数据
- 执行后读取行为树的结果

### 功能概述

演示如何使用黑板进行数据交换：
- 通过参数表向行为树传递数据
- 读取行为树自动设置的黑板值
- 动态设置和获取黑板值
- 支持多种数据类型

### 核心代码解析

#### 1. 执行时传递参数（自动写入黑板）

```lua
local params = {
    waypoints = "0,0,0;5,0,0;5,5,0;0,5,0;0,0,0",
    delay_ms = 200
}

local tree_id = bt.execute("MainTree", entity_id, params)
-- params 中的数据自动写入黑板：
-- waypoints -> 黑板的 "waypoints" 键
-- delay_ms -> 黑板的 "delay_ms" 键
```

#### 2. 读取黑板值

```lua
-- 读取执行时传入的 entity_id
local entity_id_from_bb = bt.get_blackboard(tree_id2, "entity_id")

-- 读取传入的参数
local target_x = bt.get_blackboard(tree_id2, "target_x")
```

#### 3. 动态设置黑板值

```lua
-- 随时可以向黑板写入新数据
bt.set_blackboard(tree_id2, "custom_value", 42)

-- 然后读取验证
local custom = bt.get_blackboard(tree_id2, "custom_value")
```

#### 4. 支持的数据类型

```lua
-- 字符串
bt.set_blackboard(tree_id2, "test_string", "hello from lua")

-- 浮点数
bt.set_blackboard(tree_id2, "test_number", 3.14159)

-- 布尔值
bt.set_blackboard(tree_id2, "test_bool", true)

-- 整数
bt.set_blackboard(tree_id2, "test_int", 100)
```

### 数据流向

```
┌─────────────────┐         ┌──────────────────┐
│   Lua 脚本       │         │   行为树 (XML)    │
│                 │         │                  │
│  params = {     │ ──────► │  waypoints="{waypoints}"
│    waypoints=...│  执行    │  delay_ms="{delay_ms}" │
│    delay_ms=200 │         │                  │
│  }              │         │  节点读取黑板值    │
│                 │         │  执行任务...      │
│  bt.execute()   │         │                  │
└─────────────────┘         └──────────────────┘
         │                            │
         │                            ▼
         │                   ┌──────────────────┐
         │                   │     黑板          │
         │                   │  waypoints="..." │
         │                   │  delay_ms=200    │
         │                   │  entity_id="..." │
         │                   └──────────────────┘
         ▼
┌─────────────────┐
│  bt.get_blackboard() │
│  读取结果/状态   │
└─────────────────┘
```

### 提供的 API

| API | 功能 |
|-----|------|
| `bt.execute(tree_name, entity_id, params)` | 执行时传入参数（自动写入黑板） |
| `bt.get_blackboard(tree_id, key)` | 读取黑板值 |
| `bt.set_blackboard(tree_id, key, value)` | 设置黑板值 |

---

## bt_custom_node_example

**文件**: `scripts/bt_custom_node_example.lua`

### 功能概述

演示如何将 Lua 函数注册为行为树节点，让你可以用 Lua 编写自定义的行为树逻辑，而不需要编写 C++ 代码。

### 核心概念

| 概念 | 说明 |
|------|------|
| `bt.register_action()` | 将 Lua 函数注册为**动作节点** |
| `bt.register_condition()` | 将 Lua 函数注册为**条件节点** |
| 动作节点返回值 | `"SUCCESS"` 或 `"FAILURE"` |
| 条件节点返回值 | `true` 或 `false` |

### 核心代码解析

#### 1. 注册 Lua 动作节点

```lua
bt.register_action("LuaCheckHealth", function()
    print("   [LuaCheckHealth] Executing...")
    
    local count = sim.get_entity_count()
    print("   [LuaCheckHealth] Entity count: " .. count)
    
    if count > 0 then
        print("   [LuaCheckHealth] Health OK")
        return "SUCCESS"  -- 动作节点必须返回 "SUCCESS" 或 "FAILURE"
    else
        print("   [LuaCheckHealth] No entities!")
        return "FAILURE"
    end
end)
```

#### 2. 注册 Lua 条件节点

```lua
bt.register_condition("LuaHasEntities", function()
    local count = sim.get_entity_count()
    print("   [LuaHasEntities] Checking entities: " .. count)
    return count > 0  -- 条件节点返回布尔值
end)
```

#### 3. 在 XML 中使用 Lua 节点

```lua
local xml_content = [[
<root main_tree_to_execute="LuaTestTree">
    <BehaviorTree ID="LuaTestTree">
        <Sequence name="lua_test_sequence">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <LuaAction lua_node_name="LuaCheckHealth" />
            <LuaCondition lua_node_name="LuaHasNPC" />
            <LuaAction lua_node_name="LuaPatrol" />
        </Sequence>
    </BehaviorTree>
</root>
]]
```

关键点：
- 使用 `<LuaAction lua_node_name="注册的名称" />` 调用 Lua 动作节点
- 使用 `<LuaCondition lua_node_name="注册的名称" />` 调用 Lua 条件节点
- 可以与 C++ 节点（如 `Sequence`）混合使用

### 使用场景

这种机制特别适合：
1. **快速原型开发** - 不用编译 C++ 就能测试新行为
2. **游戏逻辑脚本化** - 设计师可以用 Lua 编写 AI 行为
3. **热更新** - 修改 Lua 脚本后无需重启程序
4. **条件判断** - 复杂的业务逻辑用 Lua 表达更简洁

---

## bt_advanced_example

**文件**: `scripts/bt_advanced_example.lua`

### 功能概述

这是一个**综合演示脚本**，展示了 Lua 与行为树集成的所有核心功能，分为 5 个实际应用场景。

### 辅助函数

```lua
-- 打印章节标题
local function section(title)
    print("")
    print("--- " .. title .. " ---")
end

-- 生成巡逻路径点（正方形）
local function create_patrol_waypoints(start_x, start_y, size)
    return string.format("%f,%f,0;%f,%f,0;%f,%f,0;%f,%f,0;%f,%f,0",
        start_x, start_y,
        start_x + size, start_y,
        start_x + size, start_y + size,
        start_x, start_y + size,
        start_x, start_y
    )
end
```

### 场景 1：守卫巡逻（基础执行）

```lua
-- 创建守卫实体
local guard_id = sim.add_entity("guard", 0.0, 0.0, 0.0)

-- 执行预定义的 SquarePath 行为树
local tree1 = bt.execute("SquarePath", guard_id)
print("Guard patrol status: " .. bt.get_status(tree1))
```

**演示**：最基本的实体+行为树执行。

### 场景 2：多实体协调（批量操作）

```lua
-- 创建 3 个不同实体
local npc1 = sim.add_entity("npc", 20.0, 20.0, 0.0)
local npc2 = sim.add_entity("npc", 30.0, 30.0, 0.0)
local player = sim.add_entity("player", 25.0, 25.0, 0.0)

-- 为不同实体执行不同行为树
local tree2 = bt.execute("LargeSquarePath", npc1)

-- 使用动态生成的路径点
local tree3 = bt.execute("MainTree", npc2, {
    waypoints = create_patrol_waypoints(30.0, 30.0, 10.0),
    delay_ms = 100
})
```

**演示**：
- 多实体管理
- 不同实体使用不同行为树
- 动态参数生成

### 场景 3：动态黑板操作（数据交互）

```lua
-- 执行时传入初始参数
local tree4 = bt.execute("MoveToSinglePoint", player, {
    target_x = 50.0,
    target_y = 50.0,
    target_z = 0.0
})

-- 读取黑板值
local tx = bt.get_blackboard(tree4, "target_x")  -- 50.0

-- 动态修改黑板值
bt.set_blackboard(tree4, "target_x", 60.0)

-- 验证修改
local new_tx = bt.get_blackboard(tree4, "target_x")  -- 60.0
```

**演示**：执行前后都可以读写黑板，实现动态行为调整。

### 场景 4：复杂 Lua 驱动行为树（混合节点）

#### 注册自定义 Lua 节点

```lua
-- 智能巡逻动作（复杂的 Lua 逻辑）
bt.register_action("SmartPatrol", function()
    -- 查找需要巡逻的实体
    local target_entity = nil
    for _, entity in ipairs(sim.get_all_entities()) do
        if entity.type == "guard" or entity.type == "npc" then
            target_entity = entity
            break
        end
    end
    
    -- 执行菱形巡逻模式
    local moves = {{x=5,y=0}, {x=0,y=5}, {x=-5,y=0}, {x=0,y=-5}}
    for _, move in ipairs(moves) do
        sim.move_entity(target_entity.id, 
            target_entity.x + move.x, 
            target_entity.y + move.y, 0.0)
        sleep(0.1)  -- 模拟移动时间
    end
    
    return "SUCCESS"
end)

-- 仿真状态检查条件
bt.register_condition("SimulationActive", function()
    return sim.is_running()
end)
```

#### 创建混合行为树

```lua
local complex_xml = [[
<root main_tree_to_execute="ComplexMission">
    <BehaviorTree ID="ComplexMission">
        <Sequence name="mission_sequence">
            <!-- Lua 条件节点 -->
            <LuaCondition lua_node_name="SimulationActive" />
            
            <!-- C++ 动作节点 -->
            <CheckEntityExists entity_id="{entity_id}" />
            
            <!-- Lua 动作节点 -->
            <LuaAction lua_node_name="SmartPatrol" />
            
            <!-- C++ 动作节点 -->
            <FollowPath entity_id="{entity_id}" 
                        waypoints="{waypoints}" 
                        delay_ms="{delay_ms}" />
        </Sequence>
    </BehaviorTree>
</root>
]]
```

#### 执行

```lua
bt.load_text(complex_xml)
local mission_tree = bt.execute("ComplexMission", agent, {
    waypoints = create_patrol_waypoints(100.0, 100.0, 20.0),
    delay_ms = 50
})
```

**演示**：Lua 节点和 C++ 节点无缝协作，各取所长。

### 场景 5：行为树生命周期管理

```lua
-- 检查树是否存在
print("Tree 1 exists: " .. tostring(bt.has_tree(tree1)))

-- 停止行为树（用于异步/长时间运行的树）
local stop_result = bt.stop(tree1)
```

**演示**：管理和监控行为树的状态。

### 完整数据流

```
┌─────────────────────────────────────────────────────────────┐
│                      Lua 脚本                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  注册节点    │  │  加载 XML   │  │  执行行为树          │ │
│  │ SmartPatrol │  │  文件/文本  │  │  传入参数            │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                     │            │
│         ▼                ▼                     ▼            │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              Behavior Tree Factory                      │ │
│  │   (存储 Lua 函数引用 + 解析 XML 节点定义)                │ │
│  └────────────────────────────────────────────────────────┘ │
│                          │                                  │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              行为树执行流程                              │ │
│  │                                                         │ │
│  │  Sequence (C++ 节点)                                    │ │
│  │    ├── LuaCondition: SimulationActive (Lua)            │ │
│  │    ├── CheckEntityExists (C++)                         │ │
│  │    ├── LuaAction: SmartPatrol (Lua) ──► 操作实体       │ │
│  │    └── FollowPath (C++) ──► 读取 waypoints 参数        │ │
│  │                                                         │ │
│  └────────────────────────────────────────────────────────┘ │
│                          │                                  │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────────┐ │
│  │              Blackboard (黑板)                          │ │
│  │   entity_id="agent_001"                                 │ │
│  │   waypoints="100,100,0;120,100,0;..."                   │ │
│  │   delay_ms=50                                           │ │
│  │   target_x=60.0  (动态修改)                              │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 核心特性总结

| 特性 | 示例代码 | 用途 |
|------|---------|------|
| 批量加载 | `for _, file in ipairs(bt_files) do bt.load_file(file) end` | 初始化时加载所有行为树 |
| 辅助函数 | `create_patrol_waypoints()` | 动态生成复杂参数 |
| 智能节点 | `SmartPatrol` 查找特定类型实体 | 复杂决策逻辑 |
| 混合使用 | `<LuaAction>` + `<FollowPath>` | 灵活组合 |
| 动态修改 | `bt.set_blackboard(tree, key, value)` | 运行时调整行为 |
| 生命周期 | `bt.has_tree()`, `bt.stop()` | 管理多个行为树 |

---

## bt_custom_node_from_file

**文件**: `scripts/bt_custom_node_from_file.lua`

### 功能概述

这是 `bt_custom_node_example.lua` 的改进版本，将 XML 内容从 Lua 字符串中分离到单独的 XML 文件中，展示更好的代码组织方式。

### 对比

#### 之前（内嵌 XML）

```lua
local xml_content = [[
<root main_tree_to_execute="LuaTestTree">
    <BehaviorTree ID="LuaTestTree">
        ...
    </BehaviorTree>
</root>
]]
bt.load_text(xml_content)
```

#### 现在（外部文件）

**文件**: `bt_xml/lua_custom_nodes_example.xml`
```xml
<?xml version="1.0"?>
<root main_tree_to_execute="LuaTestTree">
    <BehaviorTree ID="LuaTestTree">
        ...
    </BehaviorTree>
</root>
```

**Lua 脚本**:
```lua
bt.load_file("bt_xml/lua_custom_nodes_example.xml")
```

### 优势

| 方面 | 内嵌 XML | 外部文件 |
|------|---------|---------|
| 可读性 | 差（字符串转义） | 好（语法高亮） |
| 维护性 | 需修改 Lua 文件 | 独立编辑 XML |
| 工具支持 | 无 | XML 编辑器/验证 |
| 复用性 | 低 | 高（多脚本共用） |
| 热更新 | 需重启 | 可重新加载 |

### XML 文件结构

```xml
<?xml version="1.0"?>
<root main_tree_to_execute="LuaTestTree">

    <!-- 使用 Lua 自定义节点的行为树示例 -->
    <BehaviorTree ID="LuaTestTree">
        <Sequence name="lua_test_sequence">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <LuaAction lua_node_name="LuaCheckHealth" />
            <LuaCondition lua_node_name="LuaHasNPC" />
            <LuaAction lua_node_name="LuaPatrol" />
        </Sequence>
    </BehaviorTree>

    <!-- 简化版：只检查实体和健康 -->
    <BehaviorTree ID="LuaTestTreeSimple">
        <Sequence name="lua_test_sequence_simple">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <LuaAction lua_node_name="LuaCheckHealth" />
        </Sequence>
    </BehaviorTree>

    <!-- 复杂示例：混合使用 C++ 节点和 Lua 节点 -->
    <BehaviorTree ID="LuaMixedTree">
        <Sequence name="mixed_sequence">
            <LuaCondition lua_node_name="LuaHasEntities" />
            <CheckEntityExists entity_id="{entity_id}" />
            <LuaAction lua_node_name="LuaCheckHealth" />
            <MoveToPoint entity_id="{entity_id}" x="10" y="10" z="0" />
            <LuaAction lua_node_name="LuaPatrol" />
        </Sequence>
    </BehaviorTree>

</root>
```

### 运行方式

```bash
lua scripts/bt_custom_node_from_file.lua
```

---

## load_lua_nodes_xml

**文件**: `scripts/load_lua_nodes_xml.lua`

### 功能概述

演示如何单独加载 `lua_custom_nodes_example.xml` 文件，并执行其中定义的多个行为树。

### 关键点

XML 文件中使用了 `<LuaAction>` 和 `<LuaCondition>` 节点，所以**必须先注册对应的 Lua 函数**，否则执行会失败。

### 核心流程

```lua
-- 第 1 步：注册 Lua 节点（必须先注册！）
bt.register_condition("LuaHasEntities", function()
    return sim.get_entity_count() > 0
end)

bt.register_action("LuaCheckHealth", function()
    local count = sim.get_entity_count()
    print("   [LuaCheckHealth] Entities: " .. count)
    return count > 0 and "SUCCESS" or "FAILURE"
end)

-- 第 2 步：加载 XML 文件
bt.load_file("bt_xml/lua_custom_nodes_example.xml")

-- 第 3 步：执行不同的行为树
local tree1 = bt.execute("LuaTestTree")
local tree2 = bt.execute("LuaTestTreeSimple")
local tree3 = bt.execute("LuaMixedTree", npc_id)
```

### 执行方式对比

| 方式 | 命令 | 适用场景 |
|------|------|---------|
| 交互式 | `bt bt_xml/lua_custom_nodes_example.xml LuaTestTree` | 手动测试 |
| Lua 脚本 | `lua scripts/load_lua_nodes_xml.lua` | 自动化流程 |
| C++ 代码 | `btExecutor->loadFromFile(...)` | 程序内置 |

---

## 总结

### 所有示例的功能对比

| 示例 | 主要功能 | 复杂度 |
|------|---------|--------|
| bt_control_example | 基础加载和执行 | ⭐ |
| bt_blackboard_example | 黑板数据交换 | ⭐⭐ |
| bt_custom_node_example | Lua 自定义节点 | ⭐⭐⭐ |
| bt_advanced_example | 综合高级功能 | ⭐⭐⭐⭐ |
| bt_custom_node_from_file | 外部 XML 文件 | ⭐⭐ |
| load_lua_nodes_xml | 单独加载 XML | ⭐⭐ |

### 学习路径建议

1. **初学者**: 从 `bt_control_example` 开始，理解基础概念
2. **进阶**: 学习 `bt_blackboard_example`，掌握数据传递
3. **高级**: 研究 `bt_custom_node_example`，编写自定义节点
4. **专家**: 参考 `bt_advanced_example`，构建复杂系统

### 常用 API 速查

```lua
-- 加载
bt.load_file(xml_path)           -- 从文件加载
bt.load_text(xml_string)         -- 从字符串加载

-- 执行
bt.execute(tree_name)                            -- 无参数执行
bt.execute(tree_name, entity_id)                 -- 指定实体
bt.execute(tree_name, entity_id, params)         -- 带参数

-- 黑板
bt.get_blackboard(tree_id, key)                  -- 读取
bt.set_blackboard(tree_id, key, value)           -- 写入

-- 状态
bt.get_status(tree_id)           -- 获取状态
bt.has_tree(tree_id)             -- 检查存在
bt.stop(tree_id)                 -- 停止执行

-- 自定义节点
bt.register_action(name, function)      -- 注册动作节点
bt.register_condition(name, function)   -- 注册条件节点
```
