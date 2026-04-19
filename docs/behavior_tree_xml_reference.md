# 行为树 XML 文件参考文档

本文档详细介绍了 `bt_xml/` 目录下的所有行为树 XML 文件，包括每个行为树的结构、用途和使用方法。

---

## 目录

1. [path_movement.xml - 基础路径移动](#path_movement)
2. [square_path.xml - 正方形路径](#square_path)
3. [square_path_composite.xml - 组合路径](#square_path_composite)
4. [waypoint_patrol.xml - 巡逻路径](#waypoint_patrol)
5. [lua_custom_nodes_example.xml - Lua 自定义节点](#lua_custom_nodes_example)

---

## path_movement

**文件**: `bt_xml/path_movement.xml`

### 概述

这是一个**基础路径移动**的行为树集合，展示了如何使用参数化的路径点进行移动。适合学习行为树的基本结构和参数传递机制。

### 行为树列表

| 行为树 ID | 用途 | 特点 |
|-----------|------|------|
| `MainTree` | 沿指定路径移动 | 从黑板读取 waypoints 和 delay_ms |
| `MoveToSinglePoint` | 移动到单个目标点 | 从黑板读取 target_x/y/z |
| `PathMovementWithRetry` | 带重试机制的路径移动 | 使用 RetryUntilSuccessful 装饰器 |

### XML 结构详解

#### MainTree

```xml
<BehaviorTree ID="MainTree">
    <Sequence name="path_movement_sequence">
        <!-- 检查实体是否存在 -->
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 沿路径移动 -->
        <FollowPath entity_id="{entity_id}" 
                    waypoints="{waypoints}" 
                    delay_ms="{delay_ms}" />
    </Sequence>
</BehaviorTree>
```

**节点说明**:
- `Sequence`: 顺序执行子节点，任一失败则整体失败
- `CheckEntityExists`: 检查指定实体是否存在
- `FollowPath`: 沿路径点序列移动

**黑板参数**:
| 参数名 | 类型 | 说明 |
|--------|------|------|
| `entity_id` | string | 实体标识符（自动设置） |
| `waypoints` | string | 路径点序列，格式："x1,y1,z1;x2,y2,z2;..." |
| `delay_ms` | int | 每步移动的延迟（毫秒） |

#### MoveToSinglePoint

```xml
<BehaviorTree ID="MoveToSinglePoint">
    <Sequence name="move_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        <MoveToPoint entity_id="{entity_id}" 
                     x="{target_x}" 
                     y="{target_y}" 
                     z="{target_z}" />
    </Sequence>
</BehaviorTree>
```

**黑板参数**:
| 参数名 | 类型 | 说明 |
|--------|------|------|
| `target_x` | double | 目标 X 坐标 |
| `target_y` | double | 目标 Y 坐标 |
| `target_z` | double | 目标 Z 坐标 |

#### PathMovementWithRetry

```xml
<BehaviorTree ID="PathMovementWithRetry">
    <Sequence name="retry_sequence">
        <RetryUntilSuccessful num_attempts="3">
            <CheckEntityExists entity_id="{entity_id}" />
        </RetryUntilSuccessful>
        
        <FollowPath entity_id="{entity_id}" 
                    waypoints="{waypoints}" 
                    delay_ms="{delay_ms}" />
    </Sequence>
</BehaviorTree>
```

**特点**: 使用 `RetryUntilSuccessful` 装饰器，如果实体检查失败会重试 3 次。

### Lua 调用示例

```lua
-- 创建实体
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 执行 MainTree，传入路径参数
local tree_id = bt.execute("MainTree", entity_id, {
    waypoints = "0,0,0;5,0,0;5,5,0;0,5,0;0,0,0",
    delay_ms = 200
})

-- 执行 MoveToSinglePoint，传入目标坐标
local tree_id2 = bt.execute("MoveToSinglePoint", entity_id, {
    target_x = 20.0,
    target_y = 20.0,
    target_z = 0.0
})
```

---

## square_path

**文件**: `bt_xml/square_path.xml`

### 概述

这是一个**固定路径**的行为树集合，预定义了正方形移动路径。适合快速测试和演示，无需传入复杂参数。

### 行为树列表

| 行为树 ID | 用途 | 路径范围 |
|-----------|------|----------|
| `SquarePath` | 小正方形路径 | 10x10 单位 |
| `LargeSquarePath` | 大正方形路径 | 50x50 单位 |

### XML 结构详解

#### SquarePath

```xml
<BehaviorTree ID="SquarePath">
    <Sequence name="square_path_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 正方形路径：中心(0,0) -> (10,0) -> (10,10) -> (0,10) -> (0,0) -->
        <FollowPath entity_id="{entity_id}" 
                    waypoints="0,0,0;10,0,0;10,10,0;0,10,0;0,0,0" 
                    delay_ms="500" />
    </Sequence>
</BehaviorTree>
```

**路径可视化**:
```
(0,10) ------ (10,10)
   |              |
   |              |
(0,0)  ------ (10,0)
```

#### LargeSquarePath

```xml
<BehaviorTree ID="LargeSquarePath">
    <Sequence name="large_square_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 大正方形：中心(0,0) -> (50,0) -> (50,50) -> (0,50) -> (0,0) -->
        <FollowPath entity_id="{entity_id}" 
                    waypoints="0,0,0;50,0,0;50,50,0;0,50,0;0,0,0" 
                    delay_ms="300" />
    </Sequence>
</BehaviorTree>
```

**特点**:
- 路径点硬编码在 XML 中
- `delay_ms` 固定，不可配置
- 适合快速测试和简单场景

### Lua 调用示例

```lua
-- 创建实体
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 执行小正方形路径
local tree_id = bt.execute("SquarePath", entity_id)

-- 执行大正方形路径（更快的移动速度）
local tree_id2 = bt.execute("LargeSquarePath", entity_id)
```

---

## square_path_composite

**文件**: `bt_xml/square_path_composite.xml`

### 概述

这是一个**组合式路径**的行为树集合，使用多个 `MoveToPoint` 节点组合成完整路径。展示了如何在 XML 层面复用原子节点。

### 行为树列表

| 行为树 ID | 用途 | 节点数 |
|-----------|------|--------|
| `SquarePathComposite` | 正方形路径 | 5 个 MoveToPoint |
| `TrianglePathComposite` | 三角形路径 | 4 个 MoveToPoint |
| `PatrolPathComposite` | 复杂巡逻路径 | 6 个 MoveToPoint |

### XML 结构详解

#### SquarePathComposite

```xml
<BehaviorTree ID="SquarePathComposite">
    <Sequence name="square_composite_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 正方形路径：使用多个 MoveToPoint 节点组合 -->
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="10" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="10" y="10" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="0" y="10" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
    </Sequence>
</BehaviorTree>
```

**与 SquarePath 的区别**:
- `SquarePath`: 使用单个 `FollowPath` 节点，路径作为参数
- `SquarePathComposite`: 使用多个 `MoveToPoint` 节点，路径在 XML 中定义

**优缺点对比**:

| 特性 | SquarePath (FollowPath) | SquarePathComposite (MoveToPoint) |
|------|------------------------|-----------------------------------|
| 灵活性 | 高（动态传入路径） | 低（固定路径） |
| 可读性 | 中等 | 高（每个点清晰可见） |
| 可维护性 | 高（修改路径无需改 XML） | 中等（修改 XML） |
| 性能 | 稍好（单个节点） | 稍差（多个节点） |

#### TrianglePathComposite

```xml
<BehaviorTree ID="TrianglePathComposite">
    <Sequence name="triangle_composite_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="30" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="15" y="26" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
    </Sequence>
</BehaviorTree>
```

**路径可视化**:
```
       (15, 26)
         /\
        /  \
       /    \
(0,0) /______\ (30, 0)
```

#### PatrolPathComposite

```xml
<BehaviorTree ID="PatrolPathComposite">
    <Sequence name="patrol_composite_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="20" y="0" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="20" y="20" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="10" y="30" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="0" y="20" z="0" />
        <MoveToPoint entity_id="{entity_id}" x="0" y="0" z="0" />
    </Sequence>
</BehaviorTree>
```

**路径可视化**:
```
        D(10,30)
       /\
      /  \
     /    \
E(0,20)  C(20,20)
    |      |
    |      |
A(0,0)---B(20,0)
```

### Lua 调用示例

```lua
-- 创建实体
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 执行正方形路径（组合方式）
local tree_id = bt.execute("SquarePathComposite", entity_id)

-- 执行三角形路径
local tree_id2 = bt.execute("TrianglePathComposite", entity_id)

-- 执行复杂巡逻路径
local tree_id3 = bt.execute("PatrolPathComposite", entity_id)
```

---

## waypoint_patrol

**文件**: `bt_xml/waypoint_patrol.xml`

### 概述

这是一个**巡逻路径**的行为树集合，预定义了多种巡逻模式（多边形、8字形等）。适合 NPC 巡逻、守卫巡逻等场景。

### 行为树列表

| 行为树 ID | 用途 | 路径特点 |
|-----------|------|----------|
| `WaypointPatrol` | 五边形巡逻 | 5 个巡逻点 |
| `TrianglePatrol` | 三角形巡逻 | 3 个巡逻点 |
| `FigureEightPatrol` | 8字形巡逻 | 交叉路径 |

### XML 结构详解

#### WaypointPatrol

```xml
<BehaviorTree ID="WaypointPatrol">
    <Sequence name="patrol_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 巡逻点序列：A -> B -> C -> D -> E -> A -->
        <FollowPath entity_id="{entity_id}" 
                    waypoints="0,0,0;20,0,0;20,20,0;10,30,0;0,20,0;0,0,0" 
                    delay_ms="400" />
    </Sequence>
</BehaviorTree>
```

**巡逻点**:
- A: (0, 0, 0) - 起点
- B: (20, 0, 0) - 右
- C: (20, 20, 0) - 右上
- D: (10, 30, 0) - 顶部
- E: (0, 20, 0) - 左上
- 回到 A

**路径可视化**:
```
        D(10,30)
       /\
      /  \
     /    \
E(0,20)  C(20,20)
    \      /
     \    /
      \  /
       \/
      A(0,0)---B(20,0)
```

#### TrianglePatrol

```xml
<BehaviorTree ID="TrianglePatrol">
    <Sequence name="triangle_patrol_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <FollowPath entity_id="{entity_id}" 
                    waypoints="0,0,0;30,0,0;15,26,0;0,0,0" 
                    delay_ms="500" />
    </Sequence>
</BehaviorTree>
```

#### FigureEightPatrol

```xml
<BehaviorTree ID="FigureEightPatrol">
    <Sequence name="figure_eight_sequence">
        <CheckEntityExists entity_id="{entity_id}" />
        
        <!-- 8字形路径 -->
        <FollowPath entity_id="{entity_id}" 
                    waypoints="0,0,0;10,10,0;20,0,0;10,-10,0;0,0,0;10,10,0;20,0,0;10,-10,0;0,0,0" 
                    delay_ms="300" />
    </Sequence>
</BehaviorTree>
```

**路径可视化**:
```
        (10,10)
         /\
        /  \
(0,0) /    \ (20,0)
      \    /
       \  /
        \/
      (10,-10)
```

### 使用场景

| 场景 | 推荐行为树 | 说明 |
|------|-----------|------|
| 守卫巡逻 | `WaypointPatrol` | 覆盖区域大，路径复杂 |
| 边界巡逻 | `TrianglePatrol` | 简单高效 |
| 展示/演示 | `FigureEightPatrol` | 路径有趣，视觉效果佳 |

### Lua 调用示例

```lua
-- 创建守卫实体
local guard_id = sim.add_entity("guard", 0.0, 0.0, 0.0)

-- 执行五边形巡逻
local tree_id = bt.execute("WaypointPatrol", guard_id)

-- 执行三角形巡逻
local tree_id2 = bt.execute("TrianglePatrol", guard_id)

-- 执行8字形巡逻
local tree_id3 = bt.execute("FigureEightPatrol", guard_id)
```

---

## lua_custom_nodes_example

**文件**: `bt_xml/lua_custom_nodes_example.xml`

### 概述

这是一个**Lua 自定义节点**的示例行为树集合，展示了如何在 XML 中使用 Lua 注册的自定义节点。必须与 Lua 脚本配合使用。

### 行为树列表

| 行为树 ID | 用途 | 节点组成 |
|-----------|------|----------|
| `LuaTestTree` | 完整 Lua 节点测试 | 2 个 LuaCondition + 2 个 LuaAction |
| `LuaTestTreeSimple` | 简化版测试 | 1 个 LuaCondition + 1 个 LuaAction |
| `LuaMixedTree` | 混合节点测试 | Lua 节点 + C++ 节点混合 |

### XML 结构详解

#### LuaTestTree

```xml
<BehaviorTree ID="LuaTestTree">
    <Sequence name="lua_test_sequence">
        <!-- 条件检查：是否有实体存在 -->
        <LuaCondition lua_node_name="LuaHasEntities" />

        <!-- 动作：检查健康状态 -->
        <LuaAction lua_node_name="LuaCheckHealth" />

        <!-- 条件检查：是否有 NPC -->
        <LuaCondition lua_node_name="LuaHasNPC" />

        <!-- 动作：执行巡逻 -->
        <LuaAction lua_node_name="LuaPatrol" />
    </Sequence>
</BehaviorTree>
```

**节点说明**:
- `LuaCondition`: 调用 Lua 注册的条件判断函数
- `LuaAction`: 调用 Lua 注册的动作执行函数
- `lua_node_name`: 对应 Lua 中注册的节点名称

#### LuaMixedTree

```xml
<BehaviorTree ID="LuaMixedTree">
    <Sequence name="mixed_sequence">
        <!-- Lua 条件节点 -->
        <LuaCondition lua_node_name="LuaHasEntities" />

        <!-- C++ 动作节点 -->
        <CheckEntityExists entity_id="{entity_id}" />

        <!-- Lua 动作节点 -->
        <LuaAction lua_node_name="LuaCheckHealth" />

        <!-- C++ 动作节点 -->
        <MoveToPoint entity_id="{entity_id}" x="10" y="10" z="0" />

        <!-- Lua 动作节点 -->
        <LuaAction lua_node_name="LuaPatrol" />
    </Sequence>
</BehaviorTree>
```

**特点**: Lua 节点和 C++ 节点可以无缝混合使用。

### 前置条件

在使用此 XML 之前，必须先注册对应的 Lua 节点：

```lua
-- 注册 Lua 条件节点
bt.register_condition("LuaHasEntities", function()
    return sim.get_entity_count() > 0
end)

bt.register_condition("LuaHasNPC", function()
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.type == "npc" then return true end
    end
    return false
end)

-- 注册 Lua 动作节点
bt.register_action("LuaCheckHealth", function()
    local count = sim.get_entity_count()
    print("[LuaCheckHealth] Entities: " .. count)
    return count > 0 and "SUCCESS" or "FAILURE"
end)

bt.register_action("LuaPatrol", function()
    print("[LuaPatrol] Patrolling...")
    return "SUCCESS"
end)

-- 然后才能加载 XML
bt.load_file("bt_xml/lua_custom_nodes_example.xml")
```

### Lua 调用示例

```lua
-- 注册所有 Lua 节点（必须先做！）
-- ... 注册代码 ...

-- 加载 XML
bt.load_file("bt_xml/lua_custom_nodes_example.xml")

-- 创建实体
local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)

-- 执行完整测试树
local tree_id = bt.execute("LuaTestTree")

-- 执行简化版
local tree_id2 = bt.execute("LuaTestTreeSimple")

-- 执行混合树
local tree_id3 = bt.execute("LuaMixedTree", npc_id)
```

---

## 节点类型参考

### 控制节点（Control Nodes）

| 节点名 | 类型 | 功能 |
|--------|------|------|
| `Sequence` | 控制 | 顺序执行所有子节点，任一失败则停止 |
| `Fallback` | 控制 | 顺序尝试子节点，任一成功则停止 |
| `Parallel` | 控制 | 并行执行所有子节点 |

### 装饰器节点（Decorator Nodes）

| 节点名 | 类型 | 功能 |
|--------|------|------|
| `RetryUntilSuccessful` | 装饰器 | 重试子节点直到成功或达到最大次数 |
| `Inverter` | 装饰器 | 反转子节点结果 |
| `ForceSuccess` | 装饰器 | 强制返回成功 |

### 动作节点（Action Nodes）

| 节点名 | 来源 | 功能 |
|--------|------|------|
| `CheckEntityExists` | C++ | 检查实体是否存在 |
| `MoveToPoint` | C++ | 移动到指定坐标 |
| `FollowPath` | C++ | 沿路径点序列移动 |
| `LuaAction` | Lua | 执行 Lua 注册的动作函数 |

### 条件节点（Condition Nodes）

| 节点名 | 来源 | 功能 |
|--------|------|------|
| `LuaCondition` | Lua | 执行 Lua 注册的条件函数 |

---

## 黑板参数规范

### 自动设置的参数

| 参数名 | 设置时机 | 说明 |
|--------|----------|------|
| `entity_id` | 执行时 | 通过 `bt.execute(tree, entity_id)` 自动设置 |

### 常用传入参数

| 参数名 | 类型 | 用途 |
|--------|------|------|
| `waypoints` | string | 路径点序列，格式："x1,y1,z1;x2,y2,z2;..." |
| `delay_ms` | int | 移动延迟（毫秒） |
| `target_x` | double | 目标 X 坐标 |
| `target_y` | double | 目标 Y 坐标 |
| `target_z` | double | 目标 Z 坐标 |

---

## 使用建议

### 选择合适的行为树

| 需求 | 推荐文件 | 推荐行为树 |
|------|----------|-----------|
| 动态路径 | path_movement.xml | MainTree |
| 快速测试 | square_path.xml | SquarePath |
| 精确控制每步 | square_path_composite.xml | SquarePathComposite |
| NPC 巡逻 | waypoint_patrol.xml | WaypointPatrol |
| 自定义逻辑 | lua_custom_nodes_example.xml | LuaTestTree |

### 性能考虑

- `FollowPath` 比多个 `MoveToPoint` 性能稍好
- 固定路径适合硬编码，动态路径适合参数传入
- Lua 节点有轻微性能开销，但灵活性更高

### 调试技巧

1. 使用 `bt.get_status(tree_id)` 检查执行结果
2. 使用 `bt.get_blackboard(tree_id, key)` 查看参数值
3. 在 Lua 节点中添加 `print()` 输出调试信息
