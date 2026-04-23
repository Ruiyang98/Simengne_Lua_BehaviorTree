# 脚本状态存储与传入设计方案

## 需求分析

用户希望在 EntityScriptManager 中存储各个脚本的独立状态，并在执行时传入给脚本。这样可以实现：

1. 每个脚本有自己的变量空间（隔离）
2. 同时可以访问共享的 entity 变量
3. 脚本间可以通过共享变量通信

## 设计方案

### 架构图

```
EntityScriptManager
├── entityTable_ (共享的 entity 表)
│   ├── id = "entity_001"
│   └── vars = {}  (实体级共享变量)
│
├── scriptStates_ (脚本状态存储)
│   ├── "patrol_script" → sol::table {state vars}
│   ├── "attack_script" → sol::table {state vars}
│   └── "guard_script" → sol::table {state vars}
│
└── scripts_
    ├── TacticalScript 1
    │   └── execute(state)  ← 传入脚本自己的状态表
    ├── TacticalScript 2
    │   └── execute(state)
    └── BTScript
        └── execute(state)
```

### Lua 脚本使用方式

```lua
-- patrol.lua
function execute(state)
    -- state 是脚本自己的状态表
    state.patrol_point = state.patrol_point or "A"
    state.patrol_count = (state.patrol_count or 0) + 1
    
    -- entity 仍然是全局可访问的
    print("Entity " .. entity.id .. " patrolling point " .. state.patrol_point)
    
    -- 可以通过 entity.vars 与其他脚本共享数据
    entity.set_var("last_patrol_point", state.patrol_point)
end

-- attack.lua
function execute(state)
    -- 独立的状态空间
    state.target_id = state.target_id or nil
    state.attack_count = (state.attack_count or 0) + 1
    
    -- 读取 patrol 脚本共享的数据
    local last_point = entity.get_var("last_patrol_point")
    
    print("Entity " .. entity.id .. " attacking, last patrol was at " .. tostring(last_point))
end
```

## 实现步骤

### 步骤 1: 修改 Script 基类

添加获取脚本状态表的方法：

```cpp
// Script.h
class Script {
public:
    // ... existing code ...
    
    // Get script state table
    virtual sol::table& getState() = 0;
    
    // Set script state table (called by EntityScriptManager)
    virtual void setState(sol::table state) = 0;
};
```

### 步骤 2: 修改 TacticalScript

添加状态表成员：

```cpp
// TacticalScript.h
class TacticalScript : public Script {
private:
    // ... existing members ...
    sol::table state_;  // 脚本自己的状态表
    
public:
    // ... existing methods ...
    
    sol::table& getState() override { return state_; }
    void setState(sol::table state) override { state_ = state; }
};
```

修改 execute 方法，传入状态：

```cpp
void TacticalScript::execute() {
    if (!enabled_) return;
    if (!executeFunc_.valid()) return;
    
    try {
        // 传入脚本自己的状态表
        auto result = executeFunc_(state_);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error: " << err.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Exception: " << e.what() << std::endl;
    }
}
```

### 步骤 3: 修改 BTScript

同样添加状态表支持：

```cpp
// BTScript.h
class BTScript : public Script {
private:
    // ... existing members ...
    sol::table state_;  // 脚本自己的状态表
    
public:
    // ... existing methods ...
    
    sol::table& getState() override { return state_; }
    void setState(sol::table state) override { state_ = state; }
};
```

### 步骤 4: 修改 EntityScriptManager

添加脚本状态存储和管理：

```cpp
// EntityScriptManager.h
class EntityScriptManager {
private:
    // ... existing members ...
    
    // 脚本状态存储: scriptName -> state table
    std::unordered_map<std::string, sol::table> scriptStates_;
    
public:
    // 获取脚本状态表
    sol::optional<sol::table> getScriptState(const std::string& scriptName);
    
    // 设置脚本状态表
    bool setScriptState(const std::string& scriptName, sol::table state);
    
    // 清除所有脚本状态
    void clearAllScriptStates();
};
```

在 addTacticalScript 中创建状态表：

```cpp
bool EntityScriptManager::addTacticalScript(const std::string& scriptName, 
                                            const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        // 创建脚本自己的状态表
        sol::table scriptState = luaState_.create_table();
        scriptState["_script_name"] = scriptName;
        scriptStates_[scriptName] = scriptState;
        
        auto script = std::make_shared<TacticalScript>(
            scriptName, scriptCode, luaState_, entityId_, env_);
        
        // 将状态表设置给脚本
        script->setState(scriptState);
        
        scripts_[scriptName] = script;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create tactical script: ") + e.what();
        return false;
    }
}
```

在 removeScript 中清理状态：

```cpp
bool EntityScriptManager::removeScript(const std::string& scriptName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scripts_.find(scriptName);
    if (it == scripts_.end()) {
        lastError_ = "Script not found: " + scriptName;
        return false;
    }
    
    scripts_.erase(it);
    
    // 清理脚本状态
    auto stateIt = scriptStates_.find(scriptName);
    if (stateIt != scriptStates_.end()) {
        scriptStates_.erase(stateIt);
    }
    
    return true;
}
```

## 文件修改清单

1. **include/scripting/Script.h**

   * 添加纯虚方法 `getState()` 和 `setState()`

2. **include/scripting/TacticalScript.h**

   * 添加 `state_` 成员

   * 实现 `getState()` 和 `setState()`

3. **src/scripting/TacticalScript.cpp**

   * 修改 `execute()` 方法，传入状态表

4. **include/scripting/BTScript.h**

   * 添加 `state_` 成员

   * 实现 `getState()` 和 `setState()`

5. **src/scripting/BTScript.cpp**

   * 修改 `execute()` 方法，传入状态表

6. **include/scripting/EntityScriptManager.h**

   * 添加 `scriptStates_` 成员

   * 添加状态管理方法

7. **src/scripting/EntityScriptManager.cpp**

   * 在 `addTacticalScript` 中创建状态表

   * 在 `addBTScript` 中创建状态表

   * 在 `removeScript` 中清理状态

   * 实现状态管理方法

## 示例脚本

```lua
-- patrol.lua - 巡逻脚本
function execute(state)
    -- 初始化状态
    state.current_point = state.current_point or "A"
    state.patrol_count = (state.patrol_count or 0) + 1
    
    -- 使用脚本自己的状态
    local points = {"A", "B", "C", "D"}
    local current_index = 1
    for i, p in ipairs(points) do
        if p == state.current_point then
            current_index = i
            break
        end
    end
    
    -- 移动到下一个巡逻点
    local next_index = (current_index % #points) + 1
    state.current_point = points[next_index]
    
    -- 共享数据给其他脚本
    entity.set_var("current_patrol_point", state.current_point)
    entity.set_var("patrol_count", state.patrol_count)
    
    print(string.format("[Patrol] Entity %s moving to point %s (count: %d)",
        entity.id, state.current_point, state.patrol_count))
end

-- attack.lua - 攻击脚本
function execute(state)
    -- 独立的状态
    state.target_id = state.target_id or nil
    state.attack_count = (state.attack_count or 0) + 1
    
    -- 读取 patrol 脚本共享的数据
    local patrol_point = entity.get_var("current_patrol_point")
    local patrol_count = entity.get_var("patrol_count")
    
    print(string.format("[Attack] Entity %s attack count: %d", entity.id, state.attack_count))
    print(string.format("[Attack] Patrol is at point %s (patrol count: %d)",
        tostring(patrol_point), patrol_count or 0))
    
    -- 模拟发现敌人
    if state.attack_count % 3 == 0 then
        state.target_id = math.random(1000, 9999)
        print(string.format("[Attack] Found target: %d", state.target_id))
    end
end

-- guard.lua - 守卫脚本
function execute(state)
    -- 又一个独立的状态
    state.alert_level = state.alert_level or 0
    state.guard_count = (state.guard_count or 0) + 1
    
    -- 可以读取其他脚本的状态
    local patrol_point = entity.get_var("current_patrol_point")
    
    print(string.format("[Guard] Entity %s guarding at level %d", entity.id, state.alert_level))
    print(string.format("[Guard] Current patrol point: %s", tostring(patrol_point)))
end
```

## 优势

1. **脚本级隔离**：每个脚本有自己的 `state` 表，变量不会冲突
2. **实体级共享**：通过 `entity.vars` 实现脚本间数据共享
3. **灵活性**：脚本可以选择使用自己的状态或共享状态
4. **可调试**：可以通过 `getScriptState()` 查看每个脚本的内部状态

## 注意事项

1. 状态表的生命周期由 EntityScriptManager 管理
2. 移除脚本时会自动清理对应的状态表
3. 状态表存储在 Lua 中，可以被垃圾回收
4. 如果需要持久化，需要在 C++ 层额外实现

