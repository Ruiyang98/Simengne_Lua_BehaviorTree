# 简化版脚本状态设计方案

## 核心思想

不需要沙箱环境，每个脚本只需维护自己的状态表（`state`），执行时传入即可。

## 架构对比

### 旧方案（复杂）
```
EntityScriptManager
├── env_ (沙箱环境)
│   ├── entity (共享)
│   ├── sim (共享)
│   └── bt (共享)
│
└── scripts_
    └── 每个脚本在沙箱中执行
```

### 新方案（简化）
```
EntityScriptManager
├── luaState_ (全局Lua状态引用)
│   ├── entity (全局表)
│   ├── sim (全局表)
│   └── bt (全局表)
│
└── scriptStates_ (脚本状态存储)
    ├── "patrol" → sol::table {vars...}
    ├── "attack" → sol::table {vars...}
    └── "guard" → sol::table {vars...}
```

## 实现方案

### 1. 移除沙箱相关代码

**EntityScriptManager 修改：**
- 移除 `env_` 成员
- 移除 `createSandbox()` 方法
- 脚本直接使用全局 `luaState_`

### 2. 脚本状态管理

```cpp
// EntityScriptManager.h
class EntityScriptManager {
private:
    // scriptName -> state table
    std::unordered_map<std::string, sol::table> scriptStates_;
    
public:
    // 获取脚本状态（供调试或外部访问）
    sol::optional<sol::table> getScriptState(const std::string& scriptName);
    
    // 清除脚本状态
    void clearScriptState(const std::string& scriptName);
};
```

### 3. 修改 TacticalScript

```cpp
// TacticalScript.h
class TacticalScript : public Script {
private:
    sol::state& luaState_;
    std::string entityId_;
    sol::function executeFunc_;
    sol::table state_;  // 脚本自己的状态表
    
public:
    TacticalScript(const std::string& name, 
                   const std::string& scriptCode,
                   sol::state& luaState, 
                   const std::string& entityId,
                   sol::table state);  // 传入状态表
    
    void execute() override;
    
    sol::table& getState() { return state_; }
};
```

```cpp
// TacticalScript.cpp
void TacticalScript::execute() {
    if (!enabled_) return;
    if (!executeFunc_.valid()) return;
    
    try {
        // 直接传入状态表
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

### 4. Lua 脚本使用方式

```lua
-- patrol.lua
function execute(state)
    -- state 是脚本自己的状态表
    state.current_point = state.current_point or "A"
    state.patrol_count = (state.patrol_count or 0) + 1
    
    -- 访问全局 entity 表
    print("Entity " .. entity.id .. " patrolling")
    
    -- 访问全局 sim 表
    local pos = sim.get_entity_position(tonumber(entity.id))
    
    -- 与其他脚本共享数据
    entity.set_var("last_patrol_point", state.current_point)
end
```

## 文件修改清单

### 删除/简化

1. **EntityScriptManager.h/cpp**
   - 删除 `env_` 成员
   - 删除 `createSandbox()` 方法
   - 简化构造函数

2. **TacticalScript.h/cpp**
   - 删除 `env_` 参数
   - 添加 `state_` 参数
   - 简化 `execute()` 实现

3. **BTScript.h/cpp**
   - 删除 `env_` 参数
   - 添加 `state_` 参数
   - 简化 `execute()` 实现

### 新增

1. **EntityScriptManager**
   - 添加 `scriptStates_` 成员
   - 在 `addTacticalScript` / `addBTScript` 中创建状态表
   - 在 `removeScript` 中清理状态表

## 示例代码

### C++ 层

```cpp
// 添加脚本时创建状态
bool EntityScriptManager::addTacticalScript(const std::string& scriptName, 
                                            const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (scripts_.find(scriptName) != scripts_.end()) {
        lastError_ = "Script already exists: " + scriptName;
        return false;
    }
    
    try {
        // 创建脚本状态表
        sol::table state = luaState_.create_table();
        state["_script_name"] = scriptName;
        scriptStates_[scriptName] = state;
        
        // 创建脚本，传入状态表
        auto script = std::make_shared<TacticalScript>(
            scriptName, scriptCode, luaState_, entityId_, state);
        
        scripts_[scriptName] = script;
        return true;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to create script: ") + e.what();
        return false;
    }
}
```

### Lua 层

```lua
-- 脚本1: patrol.lua
function execute(state)
    -- 自己的状态
    state.point_index = (state.point_index or 0) + 1
    state.last_update = sim.get_time()
    
    -- 共享数据
    entity.set_var("patrol_status", "active")
    
    print(string.format("[Patrol] Entity %s at point %d", entity.id, state.point_index))
end

-- 脚本2: attack.lua
function execute(state)
    -- 独立的状态
    state.target_count = (state.target_count or 0) + 1
    
    -- 读取共享数据
    local status = entity.get_var("patrol_status")
    
    print(string.format("[Attack] Entity %s found %d targets, patrol is %s", 
        entity.id, state.target_count, status or "unknown"))
end
```

## 优势

1. **简单清晰**：没有沙箱的复杂性
2. **直接访问**：脚本可以直接访问全局 `entity`, `sim`, `bt`
3. **状态隔离**：每个脚本有自己的 `state` 表
4. **灵活共享**：通过 `entity.vars` 实现脚本间通信
5. **性能更好**：没有沙箱的额外开销

## 注意事项

1. 脚本可以直接修改全局变量（如 `_G.xxx = yyy`），但这是 Lua 的固有问题
2. 如果需要严格隔离，仍然可以使用沙箱，但大多数情况下不需要
3. 状态表的生命周期由 EntityScriptManager 管理
