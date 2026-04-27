# C++端修改脚本参数的实现计划

## 问题分析

### 当前系统架构
1. **EntityScriptManager** - 每个实体一个，管理该实体的所有脚本
2. **TacticalScript** - 纯Lua战术脚本，每个脚本有自己的状态表(state table)
3. **Script State** - 每个脚本有独立的状态表，通过`execute(state)`传递给Lua
4. **Entity Table** - 包含实体ID，通过全局变量`entity`暴露给Lua

### 发现的问题
**`entity.set_var` 和 `entity.get_var` 是被废除的功能**，示例脚本中仍然使用了这些函数，但它们没有在C++端实现。

当前代码中 `initializeEntityTable()` 只设置了 `entity.id`，没有 `set_var`/`get_var`。

## 现在的传值方式

### 方式1：直接修改 Script State（推荐）

每个脚本有自己的 `state` 表，C++可以直接修改这个表：

```cpp
// C++端
auto manager = getScriptManager(entityId);
auto scriptState = manager->getScriptState("patrol");
if (scriptState) {
    sol::table state = scriptState.value();
    state["waypoints"] = createWaypointsTable(lua);
}
```

```lua
-- Lua端
function execute(state)
    local waypoints = state.waypoints  -- 直接读取
    -- ...
end
```

### 方式2：通过 Entity Table 扩展

在 `entity` 表中添加自定义字段：

```cpp
// C++端
sol::table entityTable = manager->getEntityTable();
entityTable["waypoints"] = createWaypointsTable(lua);
```

```lua
-- Lua端
function execute(state)
    local waypoints = entity.waypoints  -- 从entity表读取
    -- ...
end
```

### 方式3：初始化时传入参数（当前已有接口）

查看现有代码，`addTacticalScript` 没有参数传入，需要扩展。

## 推荐方案：扩展 EntityScriptManager 接口

### 方案1：添加参数设置接口（推荐）

在EntityScriptManager中添加C++接口，允许从C++端修改脚本参数：

#### 1. 修改 EntityScriptManager.h

添加以下公共方法：

```cpp
// 设置指定脚本的状态参数
void setScriptParam(const std::string& scriptName, const std::string& key, sol::object value);
sol::optional<sol::object> getScriptParam(const std::string& scriptName, const std::string& key);

// 批量设置路径点（常用功能）
void setScriptWaypoints(const std::string& scriptName, 
                        const std::vector<std::tuple<double, double, double>>& waypoints);

// 获取entity表的引用，用于直接操作
sol::table& getEntityTable() { return entityTable_; }

// 设置entity表字段（所有脚本可见）
void setEntityField(const std::string& key, sol::object value);
sol::object getEntityField(const std::string& key);
```

#### 2. 修改 EntityScriptManager.cpp

实现上述方法：

```cpp
void EntityScriptManager::setScriptParam(const std::string& scriptName, 
                                          const std::string& key, 
                                          sol::object value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        it->second[key] = value;
    }
}

sol::optional<sol::object> EntityScriptManager::getScriptParam(
    const std::string& scriptName, 
    const std::string& key) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        if (it->second[key].valid()) {
            return it->second[key];
        }
    }
    return sol::nullopt;
}

void EntityScriptManager::setScriptWaypoints(
    const std::string& scriptName,
    const std::vector<std::tuple<double, double, double>>& waypoints) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it == scriptStates_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return;
    }
    
    sol::table wpTable = luaState_->create_table();
    for (size_t i = 0; i < waypoints.size(); ++i) {
        sol::table point = luaState_->create_table();
        point["x"] = std::get<0>(waypoints[i]);
        point["y"] = std::get<1>(waypoints[i]);
        point["z"] = std::get<2>(waypoints[i]);
        wpTable[i + 1] = point; // Lua数组1-indexed
    }
    
    it->second["waypoints"] = wpTable;
}

void EntityScriptManager::setEntityField(const std::string& key, sol::object value) {
    std::lock_guard<std::mutex> lock(mutex_);
    entityTable_[key] = value;
}

sol::object EntityScriptManager::getEntityField(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entityTable_[key].valid()) {
        return entityTable_[key];
    }
    return sol::nil;
}
```

### 方案2：添加脚本时传入初始参数

扩展 `addTacticalScript` 接口，支持传入初始参数：

```cpp
// 修改后的接口
bool addTacticalScript(const std::string& scriptName, 
                       const std::string& scriptCode,
                       sol::optional<sol::table> initialParams = sol::nullopt);
```

实现：
```cpp
bool EntityScriptManager::addTacticalScript(const std::string& scriptName, 
                                             const std::string& scriptCode,
                                             sol::optional<sol::table> initialParams) {
    // ... 原有代码 ...
    
    // 创建脚本状态表
    sol::table scriptState = luaState_->create_table();
    scriptState["_script_name"] = scriptName;
    
    // 如果有初始参数，合并到state
    if (initialParams) {
        sol::table params = initialParams.value();
        for (auto& pair : params) {
            scriptState[pair.first] = pair.second;
        }
    }
    
    scriptStates_[scriptName] = scriptState;
    // ... 原有代码 ...
}
```

## 使用示例

### C++端

```cpp
#include "scripting/EntityScriptManager.h"
#include "simulation/MockSimController.h"

// 获取或创建脚本管理器
auto sim = MockSimController::getInstance();
auto manager = sim->createScriptManager("vehicle_1");

// 方法1: 添加脚本后设置参数
manager->addTacticalScriptFromFile("patrol", "scripts/patrol.lua");

std::vector<std::tuple<double, double, double>> waypoints = {
    {0.0, 0.0, 0.0},
    {10.0, 0.0, 0.0},
    {10.0, 10.0, 0.0},
    {0.0, 10.0, 0.0}
};
manager->setScriptWaypoints("patrol", waypoints);

// 方法2: 设置entity表字段（所有脚本可见）
sol::state& lua = manager->getLuaState();
sol::table sharedData = lua.create_table();
sharedData["target_id"] = "enemy_1";
sharedData["alert_level"] = 5;
manager->setEntityField("shared_data", sharedData);

// 方法3: 动态更新参数
while (running) {
    manager->executeAllScripts();
    
    // 根据游戏状态更新路径点
    if (needUpdate) {
        auto newWaypoints = calculateNewWaypoints();
        manager->setScriptWaypoints("patrol", newWaypoints);
    }
    
    // 读取脚本状态
    auto currentIdx = manager->getScriptParam("patrol", "current_index");
    if (currentIdx) {
        int idx = currentIdx.value().as<int>();
        std::cout << "Current waypoint: " << idx << std::endl;
    }
}
```

### Lua端

```lua
-- patrol.lua
function execute(state)
    -- 从state读取路径点（C++通过setScriptWaypoints设置）
    local waypoints = state.waypoints
    if not waypoints or #waypoints == 0 then
        print("[Patrol] No waypoints set")
        return
    end
    
    -- 从state读取当前索引
    state.current_index = state.current_index or 1
    
    -- 获取当前目标点
    local target = waypoints[state.current_index]
    
    -- 获取当前位置
    local pos = sim.get_entity_position(tonumber(entity.id))
    if not pos then return end
    
    -- 计算距离
    local dx = target.x - pos.x
    local dy = target.y - pos.y
    local dist = math.sqrt(dx*dx + dy*dy)
    
    if dist < 1.0 then
        -- 到达目标，切换到下一个
        state.current_index = state.current_index + 1
        if state.current_index > #waypoints then
            state.current_index = 1
        end
        print(string.format("[Patrol] Reached waypoint, next: %d", state.current_index))
    else
        -- 继续移动
        local len = math.sqrt(dx*dx + dy*dy)
        sim.set_entity_move_direction(tonumber(entity.id), dx/len, dy/len, 0)
    end
    
    -- 从entity表读取共享数据
    local shared = entity.shared_data
    if shared then
        print(string.format("[Patrol] Alert level: %d", shared.alert_level))
    end
end
```

## 具体实现步骤

### 步骤1：修改 EntityScriptManager.h

添加以下方法声明：
- `setScriptParam(scriptName, key, value)` / `getScriptParam(scriptName, key)`
- `setScriptWaypoints(scriptName, waypoints)`
- `setEntityField(key, value)` / `getEntityField(key)`
- `getEntityTable()`

### 步骤2：修改 EntityScriptManager.cpp

实现上述方法。

### 步骤3：创建示例脚本

**文件**: `scripts/examples/patrol_with_dynamic_waypoints.lua`

展示如何从 `state.waypoints` 读取动态更新的路径点。

### 步骤4：创建C++示例

**文件**: `examples/cpp_modify_script_params.cpp`

展示如何：
1. 创建EntityScriptManager
2. 添加脚本
3. 动态修改路径点
4. 读取脚本状态

## 关键设计决策

1. **线程安全**: 所有修改操作使用 `mutex_` 保护
2. **数据隔离**: 每个脚本有自己的 `state`，通过 `entity` 表共享
3. **类型支持**: 使用 `sol::object` 支持任意Lua类型
4. **路径点格式**: Lua数组1-indexed，每个点是 `{x, y, z}` 表

## 注意事项

1. **Lua状态有效性**: 确保在Lua状态有效时进行操作
2. **脚本存在性检查**: 修改前检查脚本是否存在
3. **性能考虑**: 频繁修改参数可能影响性能，建议批量更新
4. **路径点索引**: Lua数组从1开始，C++ vector从0开始，注意转换
