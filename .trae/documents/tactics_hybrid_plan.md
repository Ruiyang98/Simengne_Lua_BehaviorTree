# 战术规则分层实现方案

## 一、核心原则

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     分层实现原则                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   简单战术 ────────────────────────────────► 纯 Lua 脚本                     │
│   (单一逻辑、无优先级)                                                       │
│                                                                             │
│   复杂战术 ────────────────────────────────► 行为树 + Lua 节点               │
│   (多条件、优先级、组合逻辑)                                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、简单战术规则 (纯 Lua)

### 2.1 适用场景
- 单一判断逻辑
- 无优先级决策
- 简单的状态机
- 临时/测试逻辑

### 2.2 实现方式

```lua
-- simple_tactics.lua
-- 简单战术规则 - 纯 Lua 实现

local SimpleTactics = {}

-- ============================================
-- 简单巡逻 (无优先级，单一逻辑)
-- ============================================
function SimpleTactics.patrol(entity_id, waypoints_str)
    local waypoints = parseWaypoints(waypoints_str)
    local current_index = 1
    
    return function()
        local target = waypoints[current_index]
        local pos = sim.get_entity_position(entity_id)
        
        local dist = math.sqrt((pos.x - target.x)^2 + (pos.y - target.y)^2)
        if dist < 0.5 then
            current_index = current_index + 1
            if current_index > #waypoints then
                current_index = 1
            end
        end
        
        local dx = target.x - pos.x
        local dy = target.y - pos.y
        sim.set_entity_move_direction(entity_id, dx, dy, 0)
    end
end

-- ============================================
-- 简单跟随 (单一目标)
-- ============================================
function SimpleTactics.follow(entity_id, target_id, follow_distance)
    return function()
        local my_pos = sim.get_entity_position(entity_id)
        local target_pos = sim.get_entity_position(target_id)
        
        if not target_pos then
            return -- 目标不存在
        end
        
        local dist = math.sqrt((my_pos.x - target_pos.x)^2 + (my_pos.y - target_pos.y)^2)
        
        if dist > follow_distance then
            local dx = target_pos.x - my_pos.x
            local dy = target_pos.y - my_pos.y
            sim.set_entity_move_direction(entity_id, dx, dy, 0)
        else
            sim.set_entity_move_direction(entity_id, 0, 0, 0)
        end
    end
end

-- ============================================
-- 简单攻击 (范围内自动攻击)
-- ============================================
function SimpleTactics.autoAttack(entity_id, attack_range)
    return function()
        local my_pos = sim.get_entity_position(entity_id)
        local entities = sim.get_all_entities()
        
        local nearest_enemy = nil
        local min_dist = attack_range
        
        for _, entity in ipairs(entities) do
            if entity.id ~= entity_id and entity.type == "enemy" then
                local dist = math.sqrt((my_pos.x - entity.x)^2 + (my_pos.y - entity.y)^2)
                if dist < min_dist then
                    min_dist = dist
                    nearest_enemy = entity
                end
            end
        end
        
        if nearest_enemy then
            sim.attack(entity_id, nearest_enemy.id)
        end
    end
end

-- ============================================
-- 简单战术调度器 (每 500ms 调用)
-- ============================================
local activeTactics = {}

function SimpleTactics.start(entity_id, tactic_func)
    activeTactics[entity_id] = tactic_func
end

function SimpleTactics.stop(entity_id)
    activeTactics[entity_id] = nil
end

function SimpleTactics.updateAll()
    for entity_id, tactic in pairs(activeTactics) do
        tactic()
    end
end

return SimpleTactics
```

### 2.3 使用示例

```lua
local SimpleTactics = require("simple_tactics")

-- 创建实体
local entity_id = sim.add_entity("guard", 0, 0, 0)

-- 启动简单巡逻 (纯 Lua，无行为树)
local patrol_func = SimpleTactics.patrol(entity_id, "0,0;10,0;10,10;0,10")
SimpleTactics.start(entity_id, patrol_func)

-- 在主循环中每 500ms 调用
SimpleTactics.updateAll()
```

---

## 三、复杂战术规则 (行为树 + Lua)

### 3.1 适用场景
- 多条件组合判断
- 明确的优先级顺序
- 需要中断和恢复
- 复杂的状态管理

### 3.2 实现方式

#### 3.2.1 行为树定义 (XML)

```xml
<!-- complex_tactics.xml -->
<root main_tree_to_execute="ComplexTactics">

    <!-- 复杂战术：威胁 > 障碍物 > 补给 > 巡逻 -->
    <BehaviorTree ID="ComplexTactics">
        <Selector name="main_selector">
            
            <!-- 分支1: 威胁响应 (最高优先级) -->
            <Sequence name="threat_response">
                <LuaCondition lua_node_name="DetectThreat" 
                              entity_id="{entity_id}" 
                              range="{sensor_range}"/>
                <LuaStatefulAction lua_node_name="EngageThreat" 
                                   entity_id="{entity_id}"/>
            </Sequence>
            
            <!-- 分支2: 障碍物处理 -->
            <Sequence name="obstacle_handling">
                <LuaCondition lua_node_name="DetectObstacle" 
                              entity_id="{entity_id}"/>
                <LuaAction lua_node_name="NavigateAround" 
                           entity_id="{entity_id}"/>
            </Sequence>
            
            <!-- 分支3: 补给检查 -->
            <Sequence name="resupply">
                <LuaCondition lua_node_name="NeedResupply" 
                              entity_id="{entity_id}"/>
                <LuaStatefulAction lua_node_name="MoveToSupply" 
                                   entity_id="{entity_id}"/>
                <LuaAction lua_node_name="Resupply" 
                           entity_id="{entity_id}"/>
            </Sequence>
            
            <!-- 分支4: 执行巡逻 -->
            <LuaStatefulAction lua_node_name="PatrolPath" 
                               entity_id="{entity_id}"/>
            
        </Selector>
    </BehaviorTree>

</root>
```

#### 3.2.2 Lua 节点实现

```lua
-- complex_tactics_nodes.lua
-- 复杂战术规则 - 行为树节点实现

-- ============================================
-- 条件节点
-- ============================================

-- 检测威胁
bt.register_condition("DetectThreat", function(params)
    local entity_id = params.entity_id
    local range = tonumber(params.range) or 20
    
    local pos = sim.get_entity_position(entity_id)
    if not pos then return false end
    
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id and entity.type == "enemy" then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= range then
                bt.set_blackboard(entity_id, "threat_id", entity.id)
                bt.set_blackboard(entity_id, "threat_pos", {x=entity.x, y=entity.y})
                return true
            end
        end
    end
    
    return false
end)

-- 检测障碍物
bt.register_condition("DetectObstacle", function(params)
    local entity_id = params.entity_id
    local pos = sim.get_entity_position(entity_id)
    if not pos then return false end
    
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist < 3 then
                bt.set_blackboard(entity_id, "obstacle", entity)
                return true
            end
        end
    end
    
    return false
end)

-- 需要补给
bt.register_condition("NeedResupply", function(params)
    local entity_id = params.entity_id
    -- 从 blackboard 获取弹药/血量
    local ammo = bt.get_blackboard(entity_id, "ammo") or 100
    return ammo < 30
end)

-- ============================================
-- 有状态动作节点
-- ============================================

local tacticStates = {}

-- 接战威胁
bt.register_stateful_action("EngageThreat",
    function(params) -- onStart
        local entity_id = params.entity_id
        local threat_pos = bt.get_blackboard(entity_id, "threat_pos")
        
        if not threat_pos then return "FAILURE" end
        
        tacticStates[entity_id] = {state = "approaching", target = threat_pos}
        return "RUNNING"
    end,
    
    function(params) -- onRunning
        local entity_id = params.entity_id
        local state = tacticStates[entity_id]
        
        if not state then return "FAILURE" end
        
        local pos = sim.get_entity_position(entity_id)
        local dist = math.sqrt((pos.x - state.target.x)^2 + (pos.y - state.target.y)^2)
        
        if state.state == "approaching" then
            if dist < 5 then
                state.state = "attacking"
            else
                local dx = state.target.x - pos.x
                local dy = state.target.y - pos.y
                sim.set_entity_move_direction(entity_id, dx, dy, 0)
            end
            return "RUNNING"
            
        elseif state.state == "attacking" then
            local threat_id = bt.get_blackboard(entity_id, "threat_id")
            sim.attack(entity_id, threat_id)
            
            -- 检查威胁是否还存在
            if not sim.get_entity_position(threat_id) then
                tacticStates[entity_id] = nil
                return "SUCCESS"
            end
            
            return "RUNNING"
        end
        
        return "FAILURE"
    end,
    
    function(params) -- onHalted
        local entity_id = params.entity_id
        tacticStates[entity_id] = nil
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
    end
)

-- 其他复杂节点...
```

### 3.3 使用示例

```lua
-- 复杂战术 - 使用行为树
local entity_id = sim.add_entity("soldier", 0, 0, 0)

-- 设置参数
bt.set_blackboard(entity_id, "sensor_range", 25)
bt.set_blackboard(entity_id, "ammo", 100)

-- 加载并启动行为树
bt.load_file("bt_xml/complex_tactics.xml")
bt.execute_async("ComplexTactics", entity_id)
```

---

## 四、混合使用示例

### 4.1 场景：一个实体同时有简单和复杂战术

```lua
-- hybrid_example.lua
local SimpleTactics = require("simple_tactics")

-- 创建两个实体
local guard_id = sim.add_entity("guard", 0, 0, 0)
local soldier_id = sim.add_entity("soldier", 10, 10, 0)

-- ============================================
-- Guard: 简单战术 - 纯 Lua 巡逻
-- ============================================
local patrol = SimpleTactics.patrol(guard_id, "0,0;5,0;5,5;0,5")
SimpleTactics.start(guard_id, patrol)

-- ============================================
-- Soldier: 复杂战术 - 行为树
-- ============================================
-- 设置用户路径
bt.set_blackboard(soldier_id, "user_path", "10,10;20,10;20,20;10,20")

-- 启动复杂行为树
bt.load_file("bt_xml/entity_behavior.xml")
bt.execute_async("EntityBehavior", soldier_id, {
    sensor_range = 20,
    check_distance = 5,
    avoid_distance = 3
})

-- ============================================
-- 主循环
-- ============================================
while true do
    -- 更新简单战术 (纯 Lua)
    SimpleTactics.updateAll()
    
    -- 复杂战术由 BehaviorTreeScheduler 自动处理 (每 500ms)
    -- 不需要手动调用
    
    sleep(100) -- 100ms 更新周期
end
```

---

## 五、决策流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     战术规则选择决策树                                       │
└─────────────────────────────────────────────────────────────────────────────┘

开始
  │
  ▼
是否需要优先级决策? ──► 是 ──► 是否需要可视化? ──► 是 ──► 使用行为树 + Lua
  │                              │
  否                             否
  │                              ▼
  ▼                         使用纯 Lua + 优先级逻辑
是否需要状态管理?
  │
  ├─► 是 ──► 使用行为树 (StatefulAction)
  │
  └─► 否 ──► 逻辑复杂? ──► 是 ──► 使用行为树
              │
              否
              ▼
         使用纯 Lua
```

---

## 六、实施建议

### 6.1 目录结构

```
scripts/
├── simple_tactics.lua          # 简单战术库
├── complex_tactics_nodes.lua   # 复杂战术节点
├── entity_behavior.lua         # 实体行为控制
└── tactics_selector.lua        # 战术选择器

bt_xml/
├── simple_patrol.xml           # 简单巡逻行为树
├── complex_tactics.xml         # 复杂战术行为树
└── entity_behavior.xml         # 实体主行为树
```

### 6.2 统一接口

```lua
-- tactics_selector.lua
-- 统一的战术启动接口

local SimpleTactics = require("simple_tactics")
local TacticsSelector = {}

-- 统一的启动接口
function TacticsSelector.start(entity_id, tactic_type, params)
    if tactic_type == "simple_patrol" then
        -- 简单巡逻 - 纯 Lua
        local patrol = SimpleTactics.patrol(entity_id, params.waypoints)
        SimpleTactics.start(entity_id, patrol)
        
    elseif tactic_type == "simple_follow" then
        -- 简单跟随 - 纯 Lua
        local follow = SimpleTactics.follow(entity_id, params.target, params.distance)
        SimpleTactics.start(entity_id, follow)
        
    elseif tactic_type == "complex_combat" then
        -- 复杂战斗 - 行为树
        bt.load_file("bt_xml/complex_tactics.xml")
        bt.execute_async("ComplexTactics", entity_id, params)
        
    elseif tactic_type == "entity_behavior" then
        -- 标准实体行为 - 行为树
        bt.set_blackboard(entity_id, "user_path", params.waypoints)
        bt.load_file("bt_xml/entity_behavior.xml")
        bt.execute_async("EntityBehavior", entity_id, params)
    end
end

-- 统一的停止接口
function TacticsSelector.stop(entity_id, tactic_type)
    if tactic_type == "simple" then
        SimpleTactics.stop(entity_id)
    else
        bt.stop_async(entity_id)
    end
end

return TacticsSelector
```

### 6.3 使用示例

```lua
local TacticsSelector = require("tactics_selector")

-- 简单巡逻
TacticsSelector.start("entity_1", "simple_patrol", {
    waypoints = "0,0;10,0;10,10;0,10"
})

-- 复杂战斗
TacticsSelector.start("entity_2", "complex_combat", {
    sensor_range = 25,
    attack_range = 10
})

-- 标准实体行为
TacticsSelector.start("entity_3", "entity_behavior", {
    waypoints = "0,0;15,0;15,15;0,15",
    sensor_range = 20,
    check_distance = 5
})
```

---

## 七、总结

| 场景 | 推荐方案 | 示例 |
|-----|---------|------|
| 简单巡逻 | 纯 Lua | `SimpleTactics.patrol()` |
| 简单跟随 | 纯 Lua | `SimpleTactics.follow()` |
| 简单攻击 | 纯 Lua | `SimpleTactics.autoAttack()` |
| 威胁响应 + 障碍物 + 巡逻 | 行为树 | `EntityBehavior` |
| 多优先级复杂战术 | 行为树 | `ComplexTactics` |
| 需要可视化调试 | 行为树 | 任何 XML 定义的行为树 |

**核心原则**：
1. 简单逻辑用 Lua，复杂逻辑用行为树
2. 需要可视化的用行为树
3. 单一职责的用 Lua，多条件组合的用行为树
4. 通过统一接口 `TacticsSelector` 简化调用
