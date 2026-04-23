# 战术规则实现方式对比分析

## 一、当前系统的混合架构

### 1.1 现状概述

当前系统采用的是**混合架构**：
- **行为树 (XML)**：负责控制流和优先级决策
- **Lua 脚本**：负责具体的战术逻辑实现

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        当前混合架构                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────┐      ┌─────────────────────────────────────────┐  │
│  │   行为树 (XML)       │      │   Lua 脚本 (bt_nodes_registry.lua)      │  │
│  │                     │      │                                         │  │
│  │  <Selector>         │◄────►│  bt.register_condition("SensorDetect... │  │
│  │    <Sequence>       │      │    -- 具体的威胁检测逻辑                │  │
│  │      <LuaCondition> │      │    local entities = sim.get_all...      │  │
│  │      <LuaAction>    │      │    if dist <= sensor_range then...      │  │
│  │    </Sequence>      │      │  end)                                   │  │
│  │    <Sequence>       │      │                                         │  │
│  │      ...            │      │  bt.register_action("ExecuteAttack",    │  │
│  │    </Sequence>      │      │    function(params)                     │  │
│  │  </Selector>         │      │      -- 具体的攻击逻辑                  │  │
│  │                     │      │    end)                                 │  │
│  └─────────────────────┘      └─────────────────────────────────────────┘  │
│                                                                             │
│  职责划分：                                                                  │
│  - XML: 控制流、优先级、组合逻辑                                             │
│  - Lua: 具体业务逻辑、条件判断、动作执行                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、三种实现方式对比

### 2.1 方式一：纯行为树实现 (XML Only)

```xml
<!-- 纯行为树实现示例 -->
<BehaviorTree ID="PureBT_Tactics">
    <Selector name="tactics_selector">
        <!-- 威胁检测 - 使用原生条件节点 -->
        <Sequence name="threat_response">
            <DistanceCheck entity="{entity_id}" 
                          target_type="enemy" 
                          range="{sensor_range}"/>
            <MoveToTarget entity="{entity_id}" 
                         target="{detected_threat}"/>
            <AttackAction entity="{entity_id}" 
                         target="{detected_threat}"/>
        </Sequence>
        
        <!-- 障碍物检测 -->
        <Sequence name="obstacle_avoidance">
            <ObstacleCheck entity="{entity_id}" 
                          distance="{check_distance}"/>
            <MoveAround entity="{entity_id}" 
                       direction="right"/>
        </Sequence>
        
        <!-- 巡逻路径 -->
        <PatrolPath entity="{entity_id}" 
                   waypoints="{user_path}"/>
    </Selector>
</BehaviorTree>
```

#### 优点：
| 优点 | 说明 |
|-----|------|
| 可视化友好 | 可以直接用行为树编辑器查看和编辑 |
| 性能最优 | 纯 C++ 节点，无脚本开销 |
| 类型安全 | 编译期检查，运行时错误少 |
| 易于调试 | 行为树可视化工具支持 |

#### 缺点：
| 缺点 | 说明 |
|-----|------|
| 灵活性差 | 修改规则需要重新编译 |
| 热更新难 | 不能运行时修改战术逻辑 |
| 开发效率低 | 每个小改动都要改 C++ 代码 |
| 业务耦合 | 战术逻辑与引擎代码耦合 |

---

### 2.2 方式二：纯 Lua 实现 (Script Only)

```lua
-- 纯 Lua 实现示例
local TacticsEngine = {}

-- 优先级定义
TacticsEngine.PRIORITIES = {
    THREAT_RESPONSE = 1,      -- 最高优先级
    OBSTACLE_AVOIDANCE = 2,   -- 中等优先级
    FOLLOW_PATH = 3,          -- 最低优先级
}

-- 主决策函数
function TacticsEngine.decide(entity_id, context)
    -- 检查威胁（最高优先级）
    if TacticsEngine.checkThreat(entity_id, context) then
        return TacticsEngine.handleThreat(entity_id, context)
    end
    
    -- 检查障碍物（中等优先级）
    if TacticsEngine.checkObstacle(entity_id, context) then
        return TacticsEngine.handleObstacle(entity_id, context)
    end
    
    -- 执行用户路径（最低优先级）
    if TacticsEngine.hasUserPath(entity_id) then
        return TacticsEngine.followPath(entity_id)
    end
    
    return "IDLE"
end

-- 威胁检测逻辑
function TacticsEngine.checkThreat(entity_id, context)
    local sensor_range = context.sensor_range or 20
    local pos = sim.get_entity_position(entity_id)
    if not pos then return false end
    
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range and (entity.type == "enemy" or entity.type == "threat") then
                context.detected_threat = entity
                context.threat_position = {x = entity.x, y = entity.y, z = entity.z}
                return true
            end
        end
    end
    return false
end

-- 处理威胁
function TacticsEngine.handleThreat(entity_id, context)
    -- 移动到威胁
    local threat_pos = context.threat_position
    sim.set_entity_move_direction(entity_id, 
        threat_pos.x - context.current_pos.x,
        threat_pos.y - context.current_pos.y,
        threat_pos.z - context.current_pos.z)
    
    -- 检查距离
    local dist = sim.get_entity_distance(entity_id, threat_pos.x, threat_pos.y, threat_pos.z)
    if dist <= 1.0 then
        -- 执行攻击
        sim.attack(entity_id, context.detected_threat.id)
        return "ATTACKING"
    end
    
    return "MOVING_TO_THREAT"
end

-- 其他处理函数...

return TacticsEngine
```

#### 优点：
| 优点 | 说明 |
|-----|------|
| 极高灵活性 | 运行时热更新，无需重启 |
| 开发效率高 | 脚本语言，快速迭代 |
| 业务解耦 | 战术逻辑完全独立于引擎 |
| 易于扩展 | 动态加载新战术模块 |

#### 缺点：
| 缺点 | 说明 |
|-----|------|
| 性能开销 | 每 tick 都要执行 Lua 脚本 |
| 无可视化 | 难以直观理解决策流程 |
| 类型不安全 | 运行时错误风险 |
| 状态管理复杂 | 需要手动管理有状态动作 |

---

### 2.3 方式三：混合架构 (当前方案)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        混合架构详细说明                                      │
└─────────────────────────────────────────────────────────────────────────────┘

[行为树层 - XML]
    │
    ├─► 控制流节点 (Selector, Sequence, Fallback)
    │   - 负责优先级决策
    │   - 组合子节点执行顺序
    │
    ├─► 装饰器节点 (Retry, Inverter, etc.)
    │   - 控制子节点行为
    │
    └─► Lua 包装节点 (LuaCondition, LuaAction, LuaStatefulAction)
        - 调用 Lua 函数
        - 传递参数
        - 处理返回值

[Lua 层 - bt_nodes_registry.lua]
    │
    ├─► 条件节点 (bt.register_condition)
    │   - SensorDetectThreat
    │   - HasObstacle
    │   - HasUserPath
    │
    ├─► 动作节点 (bt.register_action)
    │   - ExecuteAttack
    │   - AvoidObstacle
    │   - ResumeMovement
    │
    └─► 有状态动作 (bt.register_stateful_action)
        - MoveToThreat (onStart, onRunning, onHalted)
        - FollowUserPath (onStart, onRunning, onHalted)
```

#### 优点：
| 优点 | 说明 |
|-----|------|
| 兼顾可视化 | 行为树展示决策流程 |
| 业务灵活 | Lua 逻辑可热更新 |
| 状态管理 | 行为树自动处理有状态动作 |
| 性能平衡 | 控制流在 C++，业务在 Lua |

#### 缺点：
| 缺点 | 说明 |
|-----|------|
| 复杂度较高 | 需要理解两套系统 |
| 调试困难 | 跨语言调试较复杂 |
| 边界模糊 | 需要明确划分职责 |

---

## 三、三种方式详细对比

### 3.1 对比维度表

| 维度 | 纯行为树 | 纯 Lua | 混合架构 |
|-----|---------|--------|---------|
| **可视化** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| **灵活性** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **性能** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **开发效率** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **热更新** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **调试难度** | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **学习成本** | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **状态管理** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **团队协作** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |

### 3.2 适用场景

| 场景 | 推荐方案 | 理由 |
|-----|---------|------|
| 战术规则固定不变 | 纯行为树 | 性能最优，可视化好 |
| 频繁调整战术 | 纯 Lua | 热更新，快速迭代 |
| 复杂优先级决策 | 混合架构 | 行为树处理控制流，Lua 处理业务 |
| 需要 AI 编辑器 | 纯行为树/混合 | 可视化编辑 |
| 性能敏感 | 纯行为树 | 无脚本开销 |
| 快速原型开发 | 纯 Lua | 开发效率高 |

---

## 四、当前混合架构的优化建议

### 4.1 职责明确划分

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      推荐的职责划分                                          │
└─────────────────────────────────────────────────────────────────────────────┘

[行为树 (XML) 负责]
    ✓ 控制流和组合逻辑
    ✓ 优先级决策 (Selector)
    ✓ 顺序执行 (Sequence)
    ✓ 并行执行 (Parallel)
    ✓ 循环和重试 (Repeat, Retry)
    ✓ 状态管理 (StatefulAction)

[Lua 负责]
    ✓ 具体的业务逻辑
    ✓ 条件判断实现
    ✓ 动作执行实现
    ✓ 与模拟系统的交互
    ✗ 不应该处理控制流
    ✗ 不应该处理优先级
```

### 4.2 当前系统的优化方向

#### 4.2.1 减少 Lua 中的控制流

**当前（不推荐）：**
```lua
-- 在 Lua 中处理优先级（不推荐）
function decideTactics(entity_id)
    if checkThreat(entity_id) then
        handleThreat(entity_id)
    elseif checkObstacle(entity_id) then
        handleObstacle(entity_id)
    else
        followPath(entity_id)
    end
end
```

**优化后（推荐）：**
```lua
-- Lua 只提供原子操作
bt.register_condition("CheckThreat", function(params)
    -- 只返回 true/false
    return detectThreat(params.entity_id, params.range)
end)

bt.register_action("HandleThreat", function(params)
    -- 只执行动作
    moveToThreat(params.entity_id, params.target_id)
    return "SUCCESS"
end)

-- 优先级由行为树 XML 处理
-- <Selector>
--   <Sequence><CheckThreat/><HandleThreat/></Sequence>
--   <Sequence><CheckObstacle/><HandleObstacle/></Sequence>
--   <FollowPath/>
-- </Selector>
```

#### 4.2.2 提取通用 Lua 模块

```lua
-- utils/tactics_utils.lua
local TacticsUtils = {}

-- 通用检测函数
function TacticsUtils.detectEntitiesInRange(entity_id, range, filter_fn)
    local pos = sim.get_entity_position(entity_id)
    if not pos then return {} end
    
    local results = {}
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= range and (not filter_fn or filter_fn(entity)) then
                table.insert(results, {entity = entity, distance = dist})
            end
        end
    end
    
    return results
end

-- 通用移动函数
function TacticsUtils.moveToPosition(entity_id, target_pos, threshold)
    local pos = sim.get_entity_position(entity_id)
    if not pos then return false end
    
    local dist = sim.get_entity_distance(entity_id, target_pos.x, target_pos.y, target_pos.z)
    if dist <= threshold then
        return true -- 已到达
    end
    
    local dx = target_pos.x - pos.x
    local dy = target_pos.y - pos.y
    local dz = target_pos.z - pos.z
    sim.set_entity_move_direction(entity_id, dx, dy, dz)
    return false -- 正在移动
end

return TacticsUtils
```

---

## 五、决策建议

### 5.1 推荐方案：优化后的混合架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    推荐的优化架构                                            │
└─────────────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────┐
                    │   行为树 XML     │
                    │  (控制流 + 组合) │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
        ┌──────────┐  ┌──────────┐  ┌──────────────┐
        │ Lua条件  │  │ Lua动作  │  │ Lua有状态动作 │
        │ 节点     │  │ 节点     │  │ 节点         │
        └────┬─────┘  └────┬─────┘  └──────┬───────┘
             │             │               │
             ▼             ▼               ▼
        ┌──────────────────────────────────────────┐
        │         Lua 业务逻辑层                   │
        │  ┌──────────┐  ┌──────────┐  ┌────────┐ │
        │  │ 威胁检测  │  │ 移动控制  │  │ 攻击   │ │
        │  └──────────┘  └──────────┘  └────────┘ │
        └──────────────────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │   模拟系统接口   │
                    │  (SimController) │
                    └─────────────────┘
```

### 5.2 实施步骤

1. **保持现有架构**：当前混合架构是合理的
2. **明确职责边界**：
   - XML 只负责控制流
   - Lua 只负责业务逻辑
3. **提取公共模块**：创建 Lua 工具模块
4. **避免 Lua 中的控制流**：不要在 Lua 中写 if-else 优先级逻辑
5. **文档化约定**：明确团队开发规范

### 5.3 何时考虑其他方案

- **转向纯行为树**：战术规则完全固定，追求极致性能
- **转向纯 Lua**：需要极其灵活的战术系统，可视化不重要

---

## 六、总结

| 问题 | 答案 |
|-----|------|
| 当前架构是否合理？ | ✅ 合理，混合架构兼顾了可视化和灵活性 |
| 战术规则用行为树还是 Lua？ | 两者都用：行为树负责控制流，Lua 负责业务逻辑 |
| 需要重构吗？ | 不需要大重构，只需明确职责边界 |
| 如何优化？ | 1. 避免 Lua 中的控制流<br>2. 提取公共工具模块<br>3. 文档化开发规范 |

当前系统的混合架构是一个**平衡的设计**，既保留了行为树的可视化和状态管理能力，又通过 Lua 获得了业务逻辑的灵活性。关键在于**明确职责划分**，让 XML 处理控制流，让 Lua 处理业务逻辑。
