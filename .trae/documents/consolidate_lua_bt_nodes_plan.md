# Lua行为树节点统一注册与实体行为实现计划

## 任务概述

1. 将 `example_bt_advanced.lua` 中的有状态行为树节点（如 `LuaStatefulMoveTo`、`LuaStatefulPatrol`、`LuaStatefulWait`）移动到 `bt_nodes_registry.lua` 实现统一注册
2. 创建一个实体行为 Lua 脚本，实现以下逻辑：

   * 如果没有用户输入路线，检查传感器是否探测到威胁

   * 如果有威胁，进行打击

   * 如果有用户输入路线，执行用户移动路线

***

## 步骤一：分析现有节点

### 1.1 example\_bt\_advanced.lua 中的有状态节点

当前 `example_bt_advanced.lua` 中定义了以下有状态动作节点：

| 节点名称                | 功能      | 状态管理方式                         |
| ------------------- | ------- | ------------------------------ |
| `LuaStatefulMoveTo` | 移动到目标点  | `moveStates[entity_id]` 存储目标位置 |
| `LuaStatefulPatrol` | 巡逻多个路径点 | `moveStates[stateKey]` 存储路径点状态 |
| `LuaStatefulWait`   | 等待指定时间  | `waitStates[entity_id]` 存储开始时间 |

### 1.2 bt\_nodes\_registry.lua 现有节点

已有节点分类：

* 移动相关：`LuaMoveTo`, `LuaPatrol`, `LuaFlee`

* 战斗相关：`LuaAttack`, `LuaDefend`

* 交互相关：`LuaWait`, `LuaInteract`

* 健康状态条件：`LuaIsHealthy`, `LuaIsLowHealth`

* 目标检测条件：`LuaHasTarget`, `LuaCanSeeEnemy`

* 距离检查条件：`LuaIsInRange`

* 实体存在条件：`LuaHasEntities`, `LuaEntityExists`

***

## 步骤二：迁移有状态节点到注册中心

### 2.1 迁移 LuaStatefulMoveTo

将 `example_bt_advanced.lua` 第 21-113 行的 `LuaStatefulMoveTo` 注册逻辑移动到 `bt_nodes_registry.lua`。

**关键实现点：**

* 使用 `bt.register_stateful_action` 注册

* 保持 `onStart`, `onRunning`, `onHalted` 三个回调函数

* 使用模块级别的 `moveStates` 表存储状态

### 2.2 迁移 LuaStatefulPatrol

将 `example_bt_advanced.lua` 第 117-246 行的 `LuaStatefulPatrol` 注册逻辑移动到 `bt_nodes_registry.lua`。

**关键实现点：**

* 共享 `moveStates` 表（与 MoveTo 共用）

* 状态键使用 `entity_id .. "_patrol"` 区分

### 2.3 迁移 LuaStatefulWait

将 `example_bt_advanced.lua` 第 250-296 行的 `LuaStatefulWait` 注册逻辑移动到 `bt_nodes_registry.lua`。

**关键实现点：**

* 使用独立的 `waitStates` 表

* 使用 `os.time()` 计算等待时间

***

## 步骤三：创建实体行为脚本

### 3.1 脚本位置

新建文件：`scripts/entity_behavior.lua`

### 3.2 行为逻辑设计（更新）

**优先级顺序：威胁 > 障碍物 > 用户路线**

```
实体行为决策树：

[Root] Selector (优先级选择，按顺序执行第一个成功的分支)
├── [分支1] Sequence: 威胁响应（最高优先级）
│   ├── Condition: 传感器是否探测到威胁 (SensorDetectThreat)
│   ├── Action: 移动到威胁位置 (MoveToThreat)
│   └── Action: 执行打击 (ExecuteAttack)
│
├── [分支2] Sequence: 障碍物处理（中等优先级）
│   ├── Condition: 前方是否有障碍物 (HasObstacle)
│   ├── Action: 绕行障碍物 (AvoidObstacle)
│   └── Action: 继续移动 (ResumeMovement)
│
└── [分支3] Sequence: 执行用户输入路线（最低优先级）
    ├── Condition: 是否有用户输入路线 (HasUserPath)
    └── Action: 执行用户路线 (FollowUserPath)
```

### 3.3 需要新增的节点

#### 条件节点

1. **`SensorDetectThreat`** - 检查传感器是否探测到威胁（最高优先级）

   * 参数：`entity_id`, `sensor_range`

   * 调用 `sim.get_entities_in_range()` 或类似接口检查周围威胁

2. **`HasObstacle`** - 检查前方是否有障碍物（中等优先级）

   * 参数：`entity_id`, `check_distance`, `check_angle`

   * 使用射线检测或前方区域检测判断是否有障碍物

3. **`HasUserPath`** - 检查是否有用户输入路线（最低优先级）

   * 参数：`entity_id`

   * 检查 blackboard 中是否存在 `user_path` 或 `waypoints`

#### 动作节点

1. **`MoveToThreat`** - 移动到威胁位置

   * 参数：`entity_id`

   * 使用 `LuaStatefulMoveTo` 移动到检测到的威胁位置

2. **`ExecuteAttack`** - 执行打击

   * 参数：`entity_id`, `target_id`

   * 调用攻击逻辑，可使用现有的 `LuaAttack`

3. **`AvoidObstacle`** - 绕行障碍物

   * 参数：`entity_id`, `avoid_distance`

   * 计算绕行路径，暂时偏离当前移动方向

4. **`ResumeMovement`** - 继续移动（恢复原有路径）

   * 参数：`entity_id`

   * 绕行完成后恢复到原来的移动方向或路径

5. **`FollowUserPath`** - 执行用户输入的移动路线

   * 参数：`entity_id`

   * 从 blackboard 读取 `user_path`，使用 `LuaStatefulPatrol` 逻辑执行

### 3.4 行为树 XML 定义（更新）

创建 `bt_xml/entity_behavior.xml`：

```xml
<root main_tree_to_execute="EntityBehavior">
    <BehaviorTree ID="EntityBehavior">
        <Selector name="entity_behavior_selector">
            <!-- 分支1: 威胁响应（最高优先级） -->
            <Sequence name="threat_response">
                <LuaCondition lua_node_name="SensorDetectThreat" 
                              entity_id="{entity_id}" 
                              sensor_range="{sensor_range|20}"/>
                <LuaStatefulMove lua_node_name="MoveToThreat" entity_id="{entity_id}"/>
                <LuaAction lua_node_name="ExecuteAttack" 
                           entity_id="{entity_id}" 
                           target_id="{detected_threat_id}"/>
            </Sequence>
            
            <!-- 分支2: 障碍物处理（中等优先级） -->
            <Sequence name="obstacle_avoidance">
                <LuaCondition lua_node_name="HasObstacle" 
                              entity_id="{entity_id}" 
                              check_distance="{check_distance|5}"
                              check_angle="{check_angle|60}"/>
                <LuaAction lua_node_name="AvoidObstacle" 
                           entity_id="{entity_id}"
                           avoid_distance="{avoid_distance|3}"/>
                <LuaAction lua_node_name="ResumeMovement" entity_id="{entity_id}"/>
            </Sequence>
            
            <!-- 分支3: 执行用户输入路线（最低优先级） -->
            <Sequence name="follow_user_path">
                <LuaCondition lua_node_name="HasUserPath" entity_id="{entity_id}"/>
                <LuaStatefulMove lua_node_name="FollowUserPath" entity_id="{entity_id}"/>
            </Sequence>
        </Selector>
    </BehaviorTree>
</root>
```

***

## 步骤四：实现细节

### 4.1 新增节点注册代码（更新）

在 `bt_nodes_registry.lua` 中添加：

```lua
-- ============================================
-- 实体行为相关节点
-- ============================================

-- 1. 检查传感器是否探测到威胁（最高优先级）
bt.register_condition("SensorDetectThreat", function(params)
    local entity_id = params.entity_id or ""
    local sensor_range = tonumber(params.sensor_range) or 20
    
    -- 获取实体位置
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return false
    end
    
    -- 获取范围内的所有实体
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range and (entity.type == "enemy" or entity.type == "threat") then
                -- 保存检测到的威胁ID到 blackboard
                bt.set_blackboard(entity_id, "detected_threat_id", entity.id)
                bt.set_blackboard(entity_id, "threat_position", {x = entity.x, y = entity.y, z = entity.z})
                print(string.format("[SensorDetectThreat] Entity '%s' detected threat '%s' at distance %.2f", 
                                    entity_id, entity.id, dist))
                return true
            end
        end
    end
    
    return false
end)

-- 2. 检查前方是否有障碍物（中等优先级）
bt.register_condition("HasObstacle", function(params)
    local entity_id = params.entity_id or ""
    local check_distance = tonumber(params.check_distance) or 5
    local check_angle = tonumber(params.check_angle) or 60
    
    -- 获取实体位置和朝向
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return false
    end
    
    -- 简化实现：检查前方是否有实体阻挡
    -- 实际项目中可以使用射线检测
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= check_distance then
                -- 保存障碍物信息
                bt.set_blackboard(entity_id, "obstacle_id", entity.id)
                bt.set_blackboard(entity_id, "obstacle_position", {x = entity.x, y = entity.y, z = entity.z})
                print(string.format("[HasObstacle] Entity '%s' detected obstacle '%s' at distance %.2f", 
                                    entity_id, entity.id, dist))
                return true
            end
        end
    end
    
    return false
end)

-- 3. 检查是否有用户输入路线（最低优先级）
bt.register_condition("HasUserPath", function(params)
    local entity_id = params.entity_id or ""
    -- 检查 blackboard 中是否有用户路径
    local path = bt.get_blackboard(entity_id, "user_path")
    local has_path = path ~= nil and #path > 0
    print(string.format("[HasUserPath] Entity '%s' has user path: %s", entity_id, tostring(has_path)))
    return has_path
end)

-- 4. 移动到威胁位置（有状态动作）
-- 复用 LuaStatefulMoveTo 的逻辑，但目标从 blackboard 读取

-- 5. 执行打击
bt.register_action("ExecuteAttack", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""
    
    print(string.format("[ExecuteAttack] Entity '%s' attacking target '%s'", entity_id, target_id))
    -- 实际攻击逻辑
    return "SUCCESS"
end)

-- 6. 绕行障碍物
bt.register_action("AvoidObstacle", function(params)
    local entity_id = params.entity_id or ""
    local avoid_distance = tonumber(params.avoid_distance) or 3
    
    -- 获取障碍物位置
    local obstacle_pos = bt.get_blackboard(entity_id, "obstacle_position")
    if not obstacle_pos then
        return "FAILURE"
    end
    
    -- 计算绕行方向（简化：向右侧绕行）
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return "FAILURE"
    end
    
    -- 计算垂直方向（右侧）
    local dx = obstacle_pos.x - pos.x
    local dy = obstacle_pos.y - pos.y
    local dist = math.sqrt(dx * dx + dy * dy)
    
    if dist > 0 then
        -- 垂直向量（顺时针90度）
        local perp_x = dy / dist * avoid_distance
        local local perp_y = -dx / dist * avoid_distance
        
        -- 移动到新位置
        local new_x = pos.x + perp_x
        local new_y = pos.y + perp_y
        sim.move_entity(entity_id, new_x, new_y, pos.z)
        
        print(string.format("[AvoidObstacle] Entity '%s' avoiding obstacle to (%.1f, %.1f)", 
                            entity_id, new_x, new_y))
    end
    
    return "SUCCESS"
end)

-- 7. 继续移动（恢复原有路径）
bt.register_action("ResumeMovement", function(params)
    local entity_id = params.entity_id or ""
    
    -- 清除障碍物信息
    bt.set_blackboard(entity_id, "obstacle_id", nil)
    bt.set_blackboard(entity_id, "obstacle_position", nil)
    
    print(string.format("[ResumeMovement] Entity '%s' resuming normal movement", entity_id))
    return "SUCCESS"
end)

-- 8. 执行用户路线（有状态动作）
-- 复用 LuaStatefulPatrol 的逻辑，但从 blackboard 读取路径
```

### 4.2 实体行为脚本结构

`scripts/entity_behavior.lua`：

```lua
-- Entity Behavior Script
-- 实体行为控制脚本
-- 功能：根据用户输入或传感器探测决定行为

print("========================================")
print("    Entity Behavior Controller")
print("========================================")
print("")

-- 加载节点注册
require("bt_nodes_registry")

-- 主函数：运行实体行为
local function run_entity_behavior(entity_id, sensor_range)
    entity_id = entity_id or "entity_1"
    sensor_range = sensor_range or 20
    
    print(string.format("Starting behavior for entity: %s", entity_id))
    print(string.format("Sensor range: %d", sensor_range))
    print("")
    
    -- 加载行为树
    if not bt.load_file("bt_xml/entity_behavior.xml") then
        print("ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return
    end
    
    -- 执行行为树
    local tree_id = bt.execute("EntityBehavior", entity_id, {
        sensor_range = sensor_range
    })
    
    if tree_id ~= "" then
        print("Behavior tree executed with ID: " .. tree_id)
        print("Final status: " .. bt.get_status(tree_id))
    else
        print("ERROR: Failed to execute behavior tree: " .. bt.get_last_error())
    end
end

-- 设置用户路径
local function set_user_path(entity_id, waypoints)
    -- waypoints 格式: "0,0;10,0;10,10;0,10"
    bt.set_blackboard(entity_id, "user_path", waypoints)
    print(string.format("Set user path for '%s': %s", entity_id, waypoints))
end

-- 清除用户路径
local function clear_user_path(entity_id)
    bt.set_blackboard(entity_id, "user_path", nil)
    print(string.format("Cleared user path for '%s'", entity_id))
end

-- 导出函数
return {
    run = run_entity_behavior,
    set_user_path = set_user_path,
    clear_user_path = clear_user_path
}
```

***

## 步骤五：文件修改清单

### 修改的文件

1. **`scripts/bt_nodes_registry.lua`**

   * 添加有状态节点：`LuaStatefulMoveTo`, `LuaStatefulPatrol`, `LuaStatefulWait`

   * 添加实体行为节点：

     * 条件节点：`SensorDetectThreat` (最高优先级), `HasObstacle` (中等优先级), `HasUserPath` (最低优先级)

     * 动作节点：`MoveToThreat`, `ExecuteAttack`, `AvoidObstacle`, `ResumeMovement`, `FollowUserPath`

### 新建的文件

1. **`scripts/entity_behavior.lua`** - 实体行为控制脚本
2. **`bt_xml/entity_behavior.xml`** - 实体行为树定义

### 可选：清理的文件

1. **`scripts/example_bt_advanced.lua`** - 可以移除已迁移的节点注册代码（保持示例完整性也可以不移除）

***

## 步骤六：验证计划

### 6.1 测试场景（更新）

1. **场景1：有威胁（最高优先级）**

   * 在传感器范围内创建威胁实体

   * 同时设置用户路径

   * 执行行为树

   * 验证：实体优先响应威胁，移动到威胁位置并执行打击，忽略用户路径

2. **场景2：无威胁，有障碍物（中等优先级）**

   * 清除威胁

   * 在前方放置障碍物

   * 同时设置用户路径

   * 执行行为树

   * 验证：实体绕行障碍物，然后继续执行用户路径

3. **场景3：无威胁，无障碍物，有用户路径（最低优先级）**

   * 清除威胁和障碍物

   * 设置用户路径

   * 执行行为树

   * 验证：实体按照用户路径移动

4. **场景4：无任何条件满足**

   * 清除威胁、障碍物、用户路径

   * 执行行为树

   * 验证：行为树返回 FAILURE（无可用行为）

***

## 实施顺序

1. 修改 `bt_nodes_registry.lua` - 添加所有有状态节点和实体行为节点
2. 创建 `bt_xml/entity_behavior.xml` - 定义行为树结构
3. 创建 `scripts/entity_behavior.lua` - 实现实体行为控制
4. 测试验证

