# BTScript向XML传入额外参数的实现计划

## 问题分析

当前BTScript的实现：

1. 在`initializeBT()`中创建行为树时，只设置了`vehicle_id`到blackboard
2. 没有提供接口向XML传入其他参数（如航线点、速度等）
3. 用户需要在C++端动态设置这些参数

## 方案三详解：通过Lua脚本中转（利用现有机制）

### 核心思想

利用BTScript现有的**双重执行机制**：

1. **Lua脚本部分** - 执行`execute(state)`函数，可以修改state表
2. **行为树部分** - 从blackboard读取参数执行

通过Lua脚本作为**桥梁**，将参数从C++传递到blackboard。

### 实现方式

#### 方式A：通过state表 + Lua节点读取

**1. C++端设置参数到state**

```cpp
// 获取EntityScriptManager
auto manager = sim->createScriptManager(entityId);

// 添加BT脚本
manager->addBTScript("flight", luaScript, "bt_xml/flight.xml", "FlightTree");

// 使用setScriptParam将参数设置到脚本的state表
sol::state& lua = manager->getLuaState();
manager->setScriptParam("flight", "waypoints", sol::make_object(lua, "0,0;10,0;10,10"));
manager->setScriptParam("flight", "speed", sol::make_object(lua, 15.0));
manager->setScriptParam("flight", "altitude", sol::make_object(lua, 1000));
```

**2. Lua脚本中读取state并设置到blackboard**

```lua
-- flight.lua
function execute(state)
    -- 从state读取参数（C++通过setScriptParam设置）
    local waypoints = state.waypoints or ""
    local speed = state.speed or 10.0
    local altitude = state.altitude or 500
    
    -- 设置到blackboard（供XML中的节点使用）
    bt.set_blackboard(entity.id, "waypoints", waypoints)
    bt.set_blackboard(entity.id, "speed", tostring(speed))
    bt.set_blackboard(entity.id, "altitude", tostring(altitude))
    
    print(string.format("[Flight] Set waypoints: %s, speed: %.1f, altitude: %.1f",
                        waypoints, speed, altitude))
end
```

**3. XML中使用blackboard参数**

```xml
<!-- flight.xml -->
<BehaviorTree ID="FlightTree">
    <Sequence name="flight_sequence">
        <!-- 使用blackboard中的参数 -->
        <LuaAction lua_node_name="SetAltitude" 
                   altitude="{altitude}"/>
        <LuaStatefulAction lua_node_name="FollowWaypoints" 
                           waypoints="{waypoints}"
                           speed="{speed}"/>
    </Sequence>
</BehaviorTree>
```

**4. Lua节点中读取参数**

```lua
-- 在Lua节点注册文件中
bt.register_action("SetAltitude", function(params)
    local entity_id = params.entity_id
    local altitude = tonumber(params.altitude) or 500
    
    print(string.format("[SetAltitude] Setting altitude to %.1f for entity %s", 
                        altitude, entity_id))
    
    -- 执行设置高度逻辑...
    return "SUCCESS"
end)

bt.register_stateful_action("FollowWaypoints",
    function(params)
        local entity_id = params.entity_id
        local waypoints_str = params.waypoints or ""
        local speed = tonumber(params.speed) or 10.0
        
        print(string.format("[FollowWaypoints] Following path: %s at speed %.1f", 
                            waypoints_str, speed))
        
        -- 解析路径点并执行...
        return "RUNNING"
    end,
    function(params)
        -- onRunning...
        return "RUNNING"
    end,
    function(params)
        -- onHalted...
    end
)
```

***

#### 方式B：通过entity表共享（所有脚本可见）

**1. C++端设置参数到entity表**

```cpp
// 获取EntityScriptManager
auto manager = sim->createScriptManager(entityId);

// 添加BT脚本
manager->addBTScript("flight", luaScript, "bt_xml/flight.xml", "FlightTree");

// 使用setEntityField将参数设置到entity表（所有脚本可见）
sol::state& lua = manager->getLuaState();
sol::table flightParams = lua.create_table();
flightParams["waypoints"] = "0,0;10,0;10,10";
flightParams["speed"] = 15.0;
flightParams["altitude"] = 1000;
manager->setEntityField("flight_params", sol::make_object(lua, flightParams));
```

**2. Lua脚本中读取entity表并设置到blackboard**

```lua
-- flight.lua
function execute(state)
    -- 从entity表读取参数
    local params = entity.flight_params
    if not params then
        print("[Flight] ERROR: No flight_params set")
        return
    end
    
    -- 设置到blackboard
    bt.set_blackboard(entity.id, "waypoints", params.waypoints)
    bt.set_blackboard(entity.id, "speed", tostring(params.speed))
    bt.set_blackboard(entity.id, "altitude", tostring(params.altitude))
end
```

**3. XML和Lua节点**（同上）

***

#### 方式C：Lua脚本直接控制行为树（最灵活）

**1. C++端只设置基础参数**

```cpp
// 添加BT脚本
manager->addBTScript("flight", luaScript, "bt_xml/flight.xml", "FlightTree");

// 设置航线点
sol::state& lua = manager->getLuaState();
manager->setScriptParam("flight", "waypoints", sol::make_object(lua, "0,0;10,0;10,10"));
```

**2. Lua脚本完全控制行为树逻辑**

```lua
-- flight.lua
function execute(state)
    -- 从state读取航线点
    local waypoints = state.waypoints or ""
    
    -- 根据当前状态决定行为
    local current_pos = sim.get_entity_position(tonumber(entity.id))
    
    -- 动态计算速度（根据距离调整）
    local target_speed = calculateSpeed(current_pos, waypoints)
    
    -- 动态设置到blackboard
    bt.set_blackboard(entity.id, "waypoints", waypoints)
    bt.set_blackboard(entity.id, "speed", tostring(target_speed))
    
    -- 甚至可以动态决定是否启用某些行为
    if shouldAvoidObstacle(current_pos) then
        bt.set_blackboard(entity.id, "avoid_mode", "true")
    else
        bt.set_blackboard(entity.id, "avoid_mode", "false")
    end
end

function calculateSpeed(pos, waypoints)
    -- 根据距离计算速度...
    return 15.0
end

function shouldAvoidObstacle(pos)
    -- 检查是否需要避障...
    return false
end
```

**3. XML中使用条件**

```xml
<BehaviorTree ID="FlightTree">
    <Fallback>
        <!-- 根据avoid_mode决定是否执行避障 -->
        <Sequence name="avoid_obstacle">
            <LuaCondition lua_node_name="CheckBlackboard"
                          key="avoid_mode"
                          expected="true"/>
            <LuaAction lua_node_name="AvoidObstacle"/>
        </Sequence>
        
        <!-- 正常飞行 -->
        <Sequence name="normal_flight">
            <LuaStatefulAction lua_node_name="FollowWaypoints" 
                               waypoints="{waypoints}"
                               speed="{speed}"/>
        </Sequence>
    </Fallback>
</BehaviorTree>
```

***

### 三种方式的对比

| 方式            | 优点          | 缺点       | 适用场景          |
| ------------- | ----------- | -------- | ------------- |
| 方式A (state表)  | 参数隔离，每个脚本独立 | 需要多一步设置  | 单个脚本使用特定参数    |
| 方式B (entity表) | 参数共享，多个脚本可见 | 可能产生命名冲突 | 多个脚本需要共享参数    |
| 方式C (Lua控制)   | 最灵活，可动态计算   | 逻辑复杂     | 需要根据运行时状态动态调整 |

***

### 完整示例代码

```cpp
// bt_params_example.cpp
#include "scripting/EntityScriptManager.h"
#include "simulation/MockSimController.h"
#include "scripting/LuaSimBinding.h"

int main() {
    // 初始化
    LuaSimBinding::getInstance().initialize();
    MockSimController* sim = static_cast<MockSimController*>(
        SimControlInterface::getInstance());
    
    // 创建实体
    VehicleID vid = sim->addEntity("drone", 0, 0, 0);
    std::string entityId = std::to_string(vid.vehicle);
    
    // 获取脚本管理器
    auto manager = sim->createScriptManager(entityId);
    
    // Lua脚本代码
    std::string luaScript = R"(
        function execute(state)
            -- 从state读取参数
            local waypoints = state.waypoints or ""
            local speed = state.speed or 10.0
            
            -- 设置到blackboard供XML使用
            bt.set_blackboard(entity.id, "waypoints", waypoints)
            bt.set_blackboard(entity.id, "speed", tostring(speed))
            
            print(string.format("[Flight] Waypoints: %s, Speed: %.1f", 
                                waypoints, speed))
        end
    )";
    
    // 添加BT脚本
    manager->addBTScript("flight", luaScript, "bt_xml/flight.xml", "FlightTree");
    
    // 设置参数到state
    sol::state& lua = manager->getLuaState();
    manager->setScriptParam("flight", "waypoints", 
                            sol::make_object(lua, "0,0;10,0;10,10;0,10"));
    manager->setScriptParam("flight", "speed", 
                            sol::make_object(lua, 15.0));
    
    // 执行脚本
    manager->executeAllScripts();
    
    return 0;
}
```

***

## 方案三的关键点

1. **bt.set\_blackboard** - 这是LuaBehaviorTreeBridge提供的接口，用于在Lua中设置blackboard参数
2. **{param\_name}** - XML中使用花括号语法引用blackboard参数
3. **双重执行** - BTScript先执行Lua脚本（设置参数），再执行行为树（使用参数）

## 其他方案简述

* **方案一**：扩展BTScript构造函数，支持传入参数表

* **方案二**：通过EntityScriptManager添加方法动态设置参数

