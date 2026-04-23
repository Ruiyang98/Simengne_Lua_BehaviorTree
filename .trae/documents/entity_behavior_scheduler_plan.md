# 实体行为调度系统架构分析与实现计划

## 一、系统架构分析

### 1.1 当前系统架构概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              主程序 (main.cpp)                               │
│  ┌─────────────────┐  ┌──────────────────┐  ┌─────────────────────────────┐ │
│  │ LuaSimBinding   │  │ BehaviorTreeExec │  │ BehaviorTreeScheduler       │ │
│  │ (Lua环境管理)    │  │ (BT执行器)        │  │ (全局调度器 - 500ms周期)     │ │
│  └────────┬────────┘  └────────┬─────────┘  └─────────────┬───────────────┘ │
│           │                    │                          │                 │
│           ▼                    ▼                          ▼                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    LuaBehaviorTreeBridge (Lua-BT桥接)                │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐  │   │
│  │  │ bt.load_file │  │ bt.execute   │  │ bt.execute_async         │  │   │
│  │  │ bt.execute   │  │ bt.stop      │  │ bt.stop_async            │  │   │
│  │  │ bt.stop      │  │ bt.get_status│  │ bt.get_async_status      │  │   │
│  │  └──────────────┘  └──────────────┘  └──────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Lua 脚本层                                         │
│  ┌─────────────────────────┐  ┌─────────────────────────────────────────┐   │
│  │ entity_behavior.lua     │  │ bt_nodes_registry.lua                   │   │
│  │ - 实体行为控制脚本       │  │ - 全局节点注册中心                        │   │
│  │ - 路径管理              │  │ - SensorDetectThreat (传感器检测威胁)    │   │
│  │ - 行为树启动            │  │ - HasObstacle (障碍物检测)               │   │
│  │ - 用户路径设置          │  │ - HasUserPath (用户路径检查)             │   │
│  │                         │  │ - MoveToThreat (移动到威胁)              │   │
│  │ 函数:                   │  │ - AvoidObstacle (绕行障碍物)             │   │
│  │ - run(entity_id)        │  │ - FollowUserPath (跟随用户路径)          │   │
│  │ - run_async(entity_id)  │  │ - ExecuteAttack (执行打击)               │   │
│  │ - set_user_path()       │  │                                         │   │
│  └─────────────────────────┘  └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         行为树定义 (XML)                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ entity_behavior.xml                                                  │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ Selector (entity_behavior_selector)                          │   │   │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │   │   │
│  │  │  │ Sequence        │  │ Sequence        │  │ Sequence     │ │   │   │
│  │  │  │ (threat_response)│  │ (obstacle_avoid)│  │ (user_path)  │ │   │   │
│  │  │  │ - 威胁检测       │  │ - 障碍物检测    │  │ - 路径检查   │ │   │   │
│  │  │  │ - 移动到威胁     │  │ - 绕行          │  │ - 跟随路径   │ │   │   │
│  │  │  │ - 执行打击       │  │ - 恢复移动      │  │              │ │   │   │
│  │  │  └─────────────────┘  └─────────────────┘  └──────────────┘ │   │   │
│  │  │  优先级: 高 > 中 > 低                                          │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         模拟控制器层                                         │
│  ┌─────────────────────────┐  ┌─────────────────────────────────────────┐   │
│  │ MockSimController       │  │ SimControlInterface                     │   │
│  │ - 实体管理              │  │ - 实体位置查询                          │   │
│  │ - 移动控制              │  │ - 距离计算                              │   │
│  │ - 方向设置              │  │ - 移动方向设置                          │   │
│  └─────────────────────────┘  └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 核心组件说明

#### 1.2.1 BehaviorTreeScheduler (全局调度器)
- **单例模式**：全局唯一实例
- **调度周期**：固定 500ms (`TICK_INTERVAL_MS = 500`)
- **核心功能**：
  - `registerEntityWithTree()` - 注册实体到调度器
  - `tickAll()` - 每 500ms 执行一次所有活动树
  - `unregisterEntity()` - 注销实体
  - `pauseEntity()/resumeEntity()` - 暂停/恢复实体

#### 1.2.2 LuaBehaviorTreeBridge (Lua-BT桥接)
- **同步执行**：`bt.execute()` - 一次性执行行为树
- **异步执行**：`bt.execute_async()` - 注册到全局调度器，每 500ms tick
- **Blackboard 操作**：`bt.set_blackboard()` / `bt.get_blackboard()`
- **节点注册**：支持 Lua 函数注册为行为树节点

#### 1.2.3 entity_behavior.lua (实体行为控制)
- **路径管理**：`set_user_path()` / `get_user_path()` / `clear_user_path()`
- **行为树启动**：`run()` (同步) / `run_async()` (异步)
- **用户路径格式**：`"x1,y1;x2,y2;x3,y3"` (如 `"0,0;15,0;15,15;0,15"`)

#### 1.2.4 bt_nodes_registry.lua (节点注册中心)
- **战术规则节点**：
  - `SensorDetectThreat` - 传感器检测威胁（最高优先级）
  - `HasObstacle` - 障碍物检测（中等优先级）
  - `HasUserPath` - 用户路径检查（最低优先级）
  - `MoveToThreat` - 移动到威胁位置
  - `AvoidObstacle` - 绕行障碍物
  - `FollowUserPath` - 跟随用户路径
  - `ExecuteAttack` - 执行打击

---

## 二、执行流程分析

### 2.1 每 500ms 周期的完整执行流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         500ms 周期调度流程                                   │
└─────────────────────────────────────────────────────────────────────────────┘

[主循环 - main.cpp:runInteractiveMode()]
    │
    │ 每 500ms
    ▼
┌─────────────────┐
│ tickAll()       │  ◄── 调度器检查每个实体的 nextTickTime
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────┐
│ 遍历所有注册的实体                                               │
│ for each entity in registeredEntities:                          │
│     if now >= entity.nextTickTime:                              │
│         tickTree(entity)  ◄── 执行行为树的一次 tick              │
│         entity.nextTickTime = now + 500ms                       │
└─────────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────┐
│ 行为树执行 (entity_behavior.xml)                                 │
│                                                                 │
│ Selector (entity_behavior_selector)                             │
│     │                                                           │
│     ├─► Sequence (threat_response)                              │
│     │       ├─► LuaCondition: SensorDetectThreat               │
│     │       │       - 检查传感器范围内是否有威胁实体             │
│     │       │       - 如果有，保存威胁ID和位置到 blackboard     │
│     │       │       - 返回 true/false                          │
│     │       │                                                   │
│     │       ├─► LuaStatefulMove: MoveToThreat                  │
│     │       │       onStart: 设置移动方向指向威胁               │
│     │       │       onRunning: 每 tick 检查是否到达              │
│     │       │       onHalted: 停止移动                          │
│     │       │                                                   │
│     │       └─► LuaAction: ExecuteAttack                       │
│     │               - 执行打击逻辑                              │
│     │                                                           │
│     ├─► Sequence (obstacle_avoidance)  ◄── 如果威胁检测失败     │
│     │       ├─► LuaCondition: HasObstacle                      │
│     │       │       - 检查前方是否有障碍物                       │
│     │       │       - 如果有，保存障碍物信息到 blackboard       │
│     │       │                                                   │
│     │       ├─► LuaAction: AvoidObstacle                       │
│     │       │       - 计算绕行方向（右侧）                       │
│     │       │       - 移动到新位置                              │
│     │       │                                                   │
│     │       └─► LuaAction: ResumeMovement                      │
│     │               - 清除障碍物信息，恢复正常移动               │
│     │                                                           │
│     └─► Sequence (follow_user_path)  ◄── 如果障碍物检测也失败    │
│             ├─► LuaCondition: HasUserPath                      │
│             │       - 检查 blackboard 中是否有用户路径          │
│             │       - 路径格式: "x1,y1;x2,y2;x3,y3"             │
│             │                                                   │
│             └─► LuaStatefulMove: FollowUserPath                │
│                     onStart: 解析路径点，找到最近点作为起点      │
│                     onRunning: 逐点移动，到达后切换到下一点      │
│                     onHalted: 停止移动，清理状态                │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Lua 脚本调用流程

```lua
-- 1. 设置用户路径
set_user_path(entity_id, "0,0;15,0;15,15;0,15")
    │
    ▼
bt.set_blackboard(entity_id, "user_path", "0,0;15,0;15,15;0,15")
    │
    ▼
保存到 BehaviorTreeScheduler 中对应实体的 Blackboard

-- 2. 启动异步行为树
run_async(entity_id, options)
    │
    ▼
bt.load_file("bt_xml/entity_behavior.xml")
    │
    ▼
bt.execute_async("EntityBehavior", entity_id, {
    sensor_range = 20,
    check_distance = 5,
    avoid_distance = 3
})
    │
    ▼
BehaviorTreeScheduler::registerEntityWithTree(entity_id, treeName, tree, blackboard)
    │
    ▼
实体被添加到调度器，每 500ms 执行一次行为树 tick
```

---

## 三、战术规则优先级系统

### 3.1 优先级层次

```
┌─────────────────────────────────────────────────────────────┐
│                      优先级决策树                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Priority 1: 威胁响应 (最高)                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 条件: SensorDetectThreat                            │   │
│  │       - 扫描传感器范围 (默认 20 单位)                │   │
│  │       - 检测 enemy/threat 类型实体                  │   │
│  │                                                      │   │
│  │ 动作: MoveToThreat → ExecuteAttack                  │   │
│  │       - 移动到威胁位置                               │   │
│  │       - 执行打击                                    │   │
│  └─────────────────────────────────────────────────────┘   │
│                         │ 如果失败                           │
│                         ▼                                    │
│  Priority 2: 障碍物处理 (中等)                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 条件: HasObstacle                                   │   │
│  │       - 检查前方距离 (默认 5 单位)                   │   │
│  │       - 检测任何阻挡实体                            │   │
│  │                                                      │   │
│  │ 动作: AvoidObstacle → ResumeMovement                │   │
│  │       - 向右侧绕行 (默认 3 单位)                     │   │
│  │       - 恢复正常移动                                │   │
│  └─────────────────────────────────────────────────────┘   │
│                         │ 如果失败                           │
│                         ▼                                    │
│  Priority 3: 用户路线 (最低)                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 条件: HasUserPath                                   │   │
│  │       - 检查 blackboard 中 user_path 字段           │   │
│  │                                                      │   │
│  │ 动作: FollowUserPath                                │   │
│  │       - 解析路径点字符串                             │   │
│  │       - 逐点巡逻移动                                │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 行为树 XML 结构

```xml
<Selector name="entity_behavior_selector">
    <!-- 分支1: 威胁响应（最高优先级） -->
    <Sequence name="threat_response">
        <LuaCondition lua_node_name="SensorDetectThreat" 
                      entity_id="{entity_id}" 
                      sensor_range="{sensor_range|20}"/>
        <LuaStatefulMove lua_node_name="MoveToThreat" 
                          entity_id="{entity_id}"
                          threshold="1.0"/>
        <LuaAction lua_node_name="ExecuteAttack" 
                   entity_id="{entity_id}" 
                   target_id="{detected_threat_id}"/>
    </Sequence>
    
    <!-- 分支2: 障碍物处理（中等优先级） -->
    <Sequence name="obstacle_avoidance">
        <LuaCondition lua_node_name="HasObstacle" 
                      entity_id="{entity_id}" 
                      check_distance="{check_distance|5}"/>
        <LuaAction lua_node_name="AvoidObstacle" 
                   entity_id="{entity_id}"
                   avoid_distance="{avoid_distance|3}"/>
        <LuaAction lua_node_name="ResumeMovement" 
                   entity_id="{entity_id}"/>
    </Sequence>
    
    <!-- 分支3: 执行用户输入路线（最低优先级） -->
    <Sequence name="follow_user_path">
        <LuaCondition lua_node_name="HasUserPath" 
                      entity_id="{entity_id}"/>
        <LuaStatefulMove lua_node_name="FollowUserPath" 
                          entity_id="{entity_id}"/>
    </Sequence>
</Selector>
```

---

## 四、数据流分析

### 4.1 Blackboard 数据流

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Blackboard 数据流                                   │
└─────────────────────────────────────────────────────────────────────────────┘

[Lua 脚本层]
    │
    │ entity_behavior.lua::set_user_path(entity_id, path)
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ bt.set_blackboard(entity_id, "user_path", "0,0;15,0;15,15;0,15") │
└─────────────────────────────────────────────────────────────────┘
    │
    ▼
[调度器层]
┌─────────────────────────────────────────────────────────────────┐
│ BehaviorTreeScheduler::registerEntityWithTree()                 │
│     - 创建 Blackboard                                           │
│     - 设置 entity_id                                            │
│     - 传入参数 (sensor_range, check_distance, avoid_distance)   │
└─────────────────────────────────────────────────────────────────┘
    │
    ▼
[行为树节点层]
┌─────────────────────────────────────────────────────────────────┐
│ 节点读取/写入 Blackboard:                                       │
│                                                                 │
│ SensorDetectThreat (Condition)                                  │
│     写入: detected_threat_id = "enemy_001"                      │
│     写入: threat_position = {x=5, y=5, z=0}                     │
│                                                                 │
│ HasObstacle (Condition)                                         │
│     写入: obstacle_id = "obstacle_001"                          │
│     写入: obstacle_position = {x=3, y=3, z=0}                   │
│                                                                 │
│ HasUserPath (Condition)                                         │
│     读取: user_path = "0,0;15,0;15,15;0,15"                     │
│                                                                 │
│ MoveToThreat (StatefulAction)                                   │
│     读取: threat_position                                       │
│                                                                 │
│ FollowUserPath (StatefulAction)                                 │
│     读取: user_path                                             │
│     完成后清除: user_path = nil                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 实体状态管理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          实体状态生命周期                                    │
└─────────────────────────────────────────────────────────────────────────────┘

[创建实体]
    │
    ▼
sim.add_entity("guard", 0, 0, 0)  →  entity_id = "vehicle_1"
    │
    ▼
[设置路径]
    │
    ▼
set_user_path("vehicle_1", "0,0;15,0;15,15;0,15")
    │
    ▼
[启动行为树]
    │
    ▼
run_async("vehicle_1", options)
    │
    ▼
[注册到调度器]
    │
    ▼
BehaviorTreeScheduler::registerEntityWithTreeAndInterval()
    │
    ├─► entityId = "vehicle_1"
    ├─► treeName = "EntityBehavior"
    ├─► tickIntervalMs = 0 (使用默认 500ms)
    ├─► nextTickTime = now
    └─► isRunning = true, paused = false
    │
    ▼
[每 500ms 执行]
    │
    ▼
tickAll() → tickTree(entityInfo)
    │
    ├─► tickCount++
    ├─► tree.tickRoot() → 执行行为树逻辑
    ├─► lastStatus = RUNNING/SUCCESS/FAILURE
    └─► nextTickTime = now + 500ms
    │
    ▼
[行为树完成或停止]
    │
    ▼
stop_async("vehicle_1")
    │
    ▼
BehaviorTreeScheduler::unregisterEntity()
    │
    └─► tree.haltTree()
    └─► 从 entities_ 映射中移除
```

---

## 五、关键代码位置

### 5.1 核心文件清单

| 文件路径 | 说明 |
|---------|------|
| `src/main.cpp` | 主程序，包含 500ms tick 循环 |
| `src/behaviortree/BehaviorTreeScheduler.cpp` | 全局调度器实现 |
| `src/behaviortree/BehaviorTreeExecutor.cpp` | 行为树执行器 |
| `src/scripting/LuaBehaviorTreeBridge.cpp` | Lua-BT 桥接 |
| `src/scripting/LuaSimBinding.cpp` | Lua 绑定初始化 |
| `scripts/entity_behavior.lua` | 实体行为控制脚本 |
| `scripts/bt_nodes_registry.lua` | 节点注册中心 |
| `bt_xml/entity_behavior.xml` | 实体行为树定义 |

### 5.2 关键代码片段

#### 5.2.1 500ms 调度循环 (main.cpp:407-501)

```cpp
// Timer for tickAll
auto lastTickTime = std::chrono::steady_clock::now();
const auto tickInterval = std::chrono::milliseconds(500);

while (true) {
    // ... 处理用户输入 ...
    
    // Check if we need to tick after processing command
    auto now = std::chrono::steady_clock::now();
    if (now - lastTickTime >= tickInterval) {
        behaviortree::BehaviorTreeScheduler::getInstance().tickAll();
        lastTickTime = now;
    }
}
```

#### 5.2.2 调度器 tickAll (BehaviorTreeScheduler.cpp:25-43)

```cpp
void BehaviorTreeScheduler::tickAll() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::shared_ptr<ScheduledTreeInfo>> treesToTick;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : entities_) {
            if (pair.second->isRunning && !pair.second->paused) {
                if (now >= pair.second->nextTickTime) {
                    treesToTick.push_back(pair.second);
                }
            }
        }
    }

    for (auto& info : treesToTick) {
        tickTree(info);
    }
}
```

#### 5.2.3 实体行为脚本入口 (entity_behavior.lua:86-117)

```lua
local function run_async(entity_id, options)
    entity_id = entity_id or "entity_1"
    options = options or {}

    local sensor_range = options.sensor_range or DEFAULT_SENSOR_RANGE
    local check_distance = options.check_distance or DEFAULT_CHECK_DISTANCE
    local avoid_distance = options.avoid_distance or DEFAULT_AVOID_DISTANCE
    local tree_name = options.tree_name or "EntityBehavior"

    -- 加载行为树
    if not bt.load_file("bt_xml/entity_behavior.xml") then
        return nil
    end

    -- 异步执行行为树
    local tree_id = bt.execute_async(tree_name, entity_id, {
        sensor_range = sensor_range,
        check_distance = check_distance,
        avoid_distance = avoid_distance
    })

    return tree_id
end
```

---

## 六、系统特点总结

### 6.1 设计优点

1. **清晰的优先级系统**：Selector 节点天然支持优先级决策
2. **灵活的 Lua 脚本**：战术规则可以用 Lua 动态定义
3. **统一调度**：全局调度器确保所有实体同步更新
4. **Blackboard 数据共享**：实体间可以共享检测到的威胁信息
5. **状态ful 动作**：支持长时间运行的动作（如移动）

### 6.2 潜在改进点

1. **动态调整 tick 间隔**：当前固定 500ms，可以根据实体重要性动态调整
2. **实体间通信**：可以增加实体间直接通信机制
3. **战术规则热更新**：支持运行时重新加载 Lua 脚本
4. **可视化调试**：增加行为树执行状态可视化

---

## 七、使用示例

### 7.1 完整使用流程

```lua
-- 1. 加载节点注册表
require("bt_nodes_registry")

-- 2. 创建实体
local entity_id = sim.add_entity("guard", 0, 0, 0)

-- 3. 设置巡逻路径
set_user_path(entity_id, "0,0;15,0;15,15;0,15")

-- 4. 启动异步行为树（每 500ms 自动执行）
run_async(entity_id, {
    sensor_range = 20,      -- 传感器检测范围
    check_distance = 5,     -- 障碍物检测距离
    avoid_distance = 3,     -- 绕行距离
    tree_name = "EntityBehavior"
})

-- 5. 实体现在会自动：
--    - 每 500ms 检查一次环境
--    - 优先响应威胁
--    - 其次处理障碍物
--    - 最后执行用户路径
```

### 7.2 命令行交互

```bash
# 启动程序后，使用以下命令：

# 添加实体
> entity add guard 0 0

# 执行 Lua 脚本（包含路径设置和行为树启动）
> lua scripts/entity_behavior.lua

# 或者手动执行异步行为树
> bt-async bt_xml/entity_behavior.xml 1 EntityBehavior

# 查看运行中的行为树
> bt-list

# 停止行为树
> bt-stop 1
```

---

## 八、总结

这个系统实现了：

1. **每 500ms 周期调度**：通过 `BehaviorTreeScheduler` 单例统一管理
2. **Lua 脚本战术规则**：在 `bt_nodes_registry.lua` 中定义检测和动作节点
3. **自动打击等战术**：`SensorDetectThreat` → `MoveToThreat` → `ExecuteAttack`
4. **用户路径驱动**：`set_user_path()` 设置路径，`FollowUserPath` 节点执行
5. **优先级决策**：Selector 节点确保威胁 > 障碍物 > 用户路径的优先级

整个系统通过 Lua 脚本灵活配置战术规则，通过行为树实现决策逻辑，通过全局调度器实现统一的周期更新。
