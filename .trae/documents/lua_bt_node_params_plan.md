# Lua 行为树节点参数传递实现计划

## 问题概述

当前 Lua 行为树节点（`LuaActionNode` 和 `LuaConditionNode`）只能接收 `lua_node_name` 一个参数，无法从 XML 配置中接收其他自定义参数。这限制了 Lua 节点的灵活性和复用性。

## 现状分析

### 当前实现

1. **LuaActionNode** 和 **LuaConditionNode** 的 `providedPorts()` 只定义了一个输入端口：
   ```cpp
   static BT::PortsList providedPorts() {
       return { BT::InputPort<std::string>("lua_node_name") };
   }
   ```

2. **tick()** 方法只获取 `lua_node_name`，然后调用对应的 Lua 函数：
   ```cpp
   BT::NodeStatus tick() {
       auto nodeName = getInput<std::string>("lua_node_name");
       // ... 调用 Lua 函数
       auto result = func();  // 无参数调用
   }
   ```

3. **Lua 函数注册**：
   ```cpp
   bt.register_action("LuaCheckHealth", function()
       -- 无法接收外部参数
       return "SUCCESS"
   end)
   ```

### 期望的使用方式

```xml
<LuaAction lua_node_name="LuaMoveTo" target_x="10" target_y="20" speed="5.0"/>
<LuaCondition lua_node_name="LuaCheckDistance" target_entity="npc_1" max_distance="10.0"/>
```

```lua
bt.register_action("LuaMoveTo", function(params)
    local x = params.target_x
    local y = params.target_y
    local speed = params.speed
    -- 执行移动逻辑
    return "SUCCESS"
end)
```

## 实现方案

### 方案一：动态端口 + 参数表传递（推荐）

#### 核心思想
- 在 XML 中定义任意数量的参数
- 在 tick() 中收集所有输入参数，打包成 Lua table 传递给 Lua 函数
- Lua 函数通过参数表访问传入的值

#### 实现步骤

**步骤 1：修改 LuaActionNode 和 LuaConditionNode 的 tick() 方法**

```cpp
BT::NodeStatus LuaActionNode::tick() {
    auto nodeName = getInput<std::string>("lua_node_name");
    if (!nodeName) {
        return BT::NodeStatus::FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName.value());
    if (it == luaFunctions_.end()) {
        return BT::NodeStatus::FAILURE;
    }
    
    // 收集所有输入参数
    sol::table params = collectInputPorts();
    
    sol::protected_function func = it->second;
    auto result = func(params);  // 传递参数表
    
    // ... 处理返回值
}
```

**步骤 2：实现 collectInputPorts() 方法**

```cpp
sol::table LuaActionNode::collectInputPorts() {
    sol::table params = luaState_->create_table();
    
    // 遍历所有输入端口（除了 lua_node_name）
    for (const auto& port : config().input_ports) {
        if (port.first == "lua_node_name") continue;
        
        // 尝试获取不同类型的值
        if (auto val = getInput<std::string>(port.first)) {
            params[port.first] = val.value();
        } else if (auto val = getInput<double>(port.first)) {
            params[port.first] = val.value();
        } else if (auto val = getInput<int>(port.first)) {
            params[port.first] = val.value();
        } else if (auto val = getInput<bool>(port.first)) {
            params[port.first] = val.value();
        }
    }
    
    return params;
}
```

**步骤 3：修改 Lua 函数签名**

```lua
bt.register_action("LuaMoveTo", function(params)
    -- params 是一个 table，包含 XML 中定义的所有参数
    local target_x = params.target_x or 0
    local target_y = params.target_y or 0
    local speed = params.speed or 1.0
    
    print(string.format("Moving to (%.1f, %.1f) at speed %.1f", target_x, target_y, speed))
    
    return "SUCCESS"
end)
```

**步骤 4：XML 使用示例**

```xml
<root main_tree_to_execute="ParamTestTree">
    <BehaviorTree ID="ParamTestTree">
        <Sequence name="param_test_sequence">
            <!-- 带参数的 Lua 动作节点 -->
            <LuaAction lua_node_name="LuaMoveTo" 
                       target_x="10" 
                       target_y="20" 
                       speed="5.0"/>
            
            <!-- 带参数的 Lua 条件节点 -->
            <LuaCondition lua_node_name="LuaCheckDistance" 
                          target_entity="npc_1" 
                          max_distance="10.0"/>
        </Sequence>
    </BehaviorTree>
</root>
```

### 方案二：预定义端口 + 类型擦除（备选）

如果需要在编译期知道所有可能的参数，可以预定义一组常用端口：

```cpp
static BT::PortsList providedPorts() {
    return { 
        BT::InputPort<std::string>("lua_node_name"),
        BT::InputPort<std::string>("param_str_1"),
        BT::InputPort<std::string>("param_str_2"),
        BT::InputPort<double>("param_num_1"),
        BT::InputPort<double>("param_num_2"),
        BT::InputPort<int>("param_int_1"),
        BT::InputPort<bool>("param_bool_1")
    };
}
```

**缺点**：
- 参数数量有限
- 需要记住参数命名规则
- 不够灵活

## 推荐方案：方案一

### 优势
1. **灵活性**：XML 中可以定义任意数量的参数
2. **类型安全**：支持 string、number、boolean 等常见类型
3. **向后兼容**：现有的无参数 Lua 函数仍然可以工作（传递空表）
4. **易于使用**：Lua 代码直观，通过 params.xxx 访问参数

### 实现文件清单

| 文件 | 修改内容 |
|------|----------|
| `LuaBehaviorTreeBridge.h` | 添加 `collectInputPorts()` 方法声明，添加 `luaState_` 成员引用 |
| `LuaBehaviorTreeBridge.cpp` | 修改 `LuaActionNode::tick()` 和 `LuaConditionNode::tick()`，实现参数收集逻辑 |

### 详细代码变更

#### 1. LuaActionNode 修改

```cpp
class LuaActionNode : public BT::SyncActionNode {
public:
    LuaActionNode(const std::string& name, const BT::NodeConfiguration& config);
    
    // 改为返回空列表，允许任意输入端口
    static BT::PortsList providedPorts() { return {}; }
    
    BT::NodeStatus tick() override;
    
    static void setLuaFunction(const std::string& nodeName, sol::protected_function func);
    static void clearLuaFunction(const std::string& nodeName);
    
    // 设置 Lua 状态指针，用于创建 table
    static void setLuaState(sol::state* state) { luaState_ = state; }

private:
    static std::unordered_map<std::string, sol::protected_function> luaFunctions_;
    static std::mutex mutex_;
    static sol::state* luaState_;
    
    // 收集输入参数为 Lua table
    sol::table collectInputPorts();
};
```

#### 2. LuaConditionNode 修改

与 LuaActionNode 类似，只是返回值处理不同。

#### 3. tick() 实现

```cpp
BT::NodeStatus LuaActionNode::tick() {
    auto nodeName = getInput<std::string>("lua_node_name");
    if (!nodeName) {
        std::cerr << "[LuaActionNode] Missing lua_node_name port" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = luaFunctions_.find(nodeName.value());
    if (it == luaFunctions_.end()) {
        std::cerr << "[LuaActionNode] Lua function not found: " << nodeName.value() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // 收集参数
    sol::table params = collectInputPorts();
    
    sol::protected_function func = it->second;
    auto result = func(params);
    
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[LuaActionNode] Lua function error: " << err.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // 支持返回字符串或 table {status="SUCCESS", ...}
    if (result.get_type() == sol::type::string) {
        std::string status = result.get<std::string>();
        if (status == "SUCCESS") return BT::NodeStatus::SUCCESS;
        if (status == "FAILURE") return BT::NodeStatus::FAILURE;
        if (status == "RUNNING") return BT::NodeStatus::RUNNING;
    }
    
    return BT::NodeStatus::FAILURE;
}
```

## 使用示例

### 完整 Lua 示例

```lua
-- 注册带参数的 Lua 动作节点
bt.register_action("LuaMoveTo", function(params)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local speed = tonumber(params.speed) or 1.0
    
    print(string.format("[LuaMoveTo] Moving entity %s to (%.1f, %.1f) at speed %.1f",
                        entity_id, target_x, target_y, speed))
    
    -- 执行移动
    sim.move_entity(entity_id, target_x, target_y, 0)
    
    return "SUCCESS"
end)

-- 注册带参数的 Lua 条件节点
bt.register_condition("LuaCheckDistance", function(params)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local max_distance = tonumber(params.max_distance) or 10.0
    
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return false
    end
    
    local dx = pos.x - target_x
    local dy = pos.y - target_y
    local distance = math.sqrt(dx * dx + dy * dy)
    
    print(string.format("[LuaCheckDistance] Distance to target: %.2f (max: %.2f)",
                        distance, max_distance))
    
    return distance <= max_distance
end)

-- 加载并执行带参数的行为树
local xml_content = [[
<root main_tree_to_execute="ParamTestTree">
    <BehaviorTree ID="ParamTestTree">
        <Sequence name="param_test_sequence">
            <LuaAction lua_node_name="LuaMoveTo" 
                       entity_id="npc_1"
                       target_x="10" 
                       target_y="20" 
                       speed="5.0"/>
            <LuaCondition lua_node_name="LuaCheckDistance" 
                          entity_id="npc_1"
                          target_x="10" 
                          target_y="20" 
                          max_distance="1.0"/>
        </Sequence>
    </BehaviorTree>
</root>
]]

bt.load_text(xml_content)
local tree_id = bt.execute("ParamTestTree")
```

## 注意事项

1. **类型转换**：XML 中的属性值都是字符串，需要在 Lua 中手动转换为数字
2. **默认值**：建议在 Lua 函数中为参数提供默认值
3. **向后兼容**：现有的无参数 Lua 函数仍然可以工作，params 会是空表
4. **性能**：每次 tick 都会创建一个新的 Lua table，对于高频调用的节点需要注意性能

## 后续扩展

1. **支持输出参数**：可以通过返回 table 来设置 blackboard 值
2. **支持更多类型**：如 vector3、color 等自定义类型
3. **参数验证**：在 C++ 层添加参数类型和必填验证
