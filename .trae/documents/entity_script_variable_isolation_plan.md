# EntityScriptManager 实体变量隔离设计方案

## 问题分析

当前 `EntityScriptManager` 每个实体拥有独立的 `sol::state` Lua 状态，这确实实现了基本的隔离。但存在以下问题：

1. **全局变量污染**：所有脚本共享同一个 Lua 全局环境，一个脚本定义的变量可能被其他脚本意外访问或修改
2. **变量命名冲突**：不同脚本可能使用相同名称的全局变量，导致互相覆盖
3. **状态管理混乱**：实体状态分散在多个脚本中，没有统一的管理机制

## 目标

1. 每个实体维护自己的变量空间，脚本间变量隔离
2. EntityScriptManager 只负责执行脚本，调用全局的 luaState
3. 提供清晰的变量访问和修改接口

## 设计方案

### 方案对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| A. 每个实体独立 Lua State | 完全隔离，实现简单 | 内存开销大，无法共享数据 |
| B. 使用 Lua 沙箱/环境表 | 内存友好，可控制访问 | 需要额外管理环境表 |
| C. Entity 类管理变量 | 符合 OOP，职责清晰 | 需要修改现有架构 |

**推荐方案：B + C 混合方案**

### 详细设计

#### 1. 创建 EntityVariables 类

专门负责管理单个实体的变量存储：

```cpp
// include/scripting/EntityVariables.h
class EntityVariables {
public:
    EntityVariables(const std::string& entityId);
    
    // 变量操作
    void set(const std::string& key, sol::object value);
    sol::object get(const std::string& key);
    bool has(const std::string& key);
    void remove(const std::string& key);
    void clear();
    
    // 获取所有变量（用于调试）
    std::unordered_map<std::string, std::string> getAll() const;
    
private:
    std::string entityId_;
    sol::table variables_;  // 存储在 Lua 表中的变量
};
```

#### 2. 修改 EntityScriptManager

- 添加 `EntityVariables` 成员
- 在 Lua 中注册 `entity` 表，提供变量访问接口
- 移除独立的 `luaState_`，改为使用全局共享的 `sol::state&`

```cpp
class EntityScriptManager {
private:
    std::string entityId_;
    sol::state& globalLuaState_;  // 引用全局 Lua 状态
    EntityVariables variables_;    // 实体变量存储
    
    // 为每个实体创建独立的环境表
    sol::table env_;  // 沙箱环境
};
```

#### 3. Lua 接口设计

为每个实体在 Lua 中创建独立的 `entity` 表：

```lua
-- 在 EntityScriptManager 初始化时创建
entity = {
    id = "entity_001",
    vars = {},  -- 变量存储表
    
    -- 变量操作方法
    set_var = function(key, value) ... end,
    get_var = function(key) ... end,
    has_var = function(key) ... end,
}
```

脚本使用示例：

```lua
function execute(entity_id, sim, bt)
    -- 使用 entity 表存储变量
    local counter = entity.get_var("attack_counter") or 0
    counter = counter + 1
    entity.set_var("attack_counter", counter)
    
    -- 变量只对当前实体可见
    print(string.format("Entity %s attack count: %d", entity.id, counter))
end
```

#### 4. 脚本执行隔离

使用 Lua 的沙箱机制，为每个脚本执行创建独立的环境：

```cpp
void TacticalScript::execute() {
    if (!enabled_ || !executeFunc_.valid()) return;
    
    // 在实体的环境表中执行
    sol::set_environment(env_, executeFunc_);
    
    // 调用执行函数
    auto result = executeFunc_(entityId_);
    
    // ... 错误处理
}
```

### 文件修改清单

1. **新增文件**:
   - `include/scripting/EntityVariables.h` - 实体变量管理类头文件
   - `src/scripting/EntityVariables.cpp` - 实体变量管理类实现

2. **修改文件**:
   - `include/scripting/EntityScriptManager.h` - 添加 EntityVariables 成员，修改 luaState 为引用
   - `src/scripting/EntityScriptManager.cpp` - 实现变量接口注册
   - `include/scripting/TacticalScript.h` - 添加环境表支持
   - `src/scripting/TacticalScript.cpp` - 在沙箱环境中执行脚本
   - `include/scripting/BTScript.h` - 添加环境表支持
   - `src/scripting/BTScript.cpp` - 在沙箱环境中执行脚本

3. **示例脚本更新**:
   - `scripts/examples/tactical_auto_attack.lua` - 展示 entity.vars 用法
   - `scripts/examples/script_manager_demo.lua` - 更新示例

### 实施步骤

1. 创建 `EntityVariables` 类，实现基本的变量存取功能
2. 修改 `EntityScriptManager`，添加 `EntityVariables` 成员
3. 在 Lua 中注册 `entity` 表和变量操作方法
4. 修改 `TacticalScript` 和 `BTScript`，使用环境表隔离执行
5. 更新示例脚本，展示新的变量使用方法
6. 测试验证变量隔离效果

### 预期效果

1. 每个实体的变量完全隔离，脚本间不会互相干扰
2. EntityScriptManager 只负责脚本执行，不管理 Lua 状态生命周期
3. 提供清晰的 `entity.vars` API 供脚本使用
4. 内存开销可控，所有实体共享同一个 Lua 状态

### 注意事项

1. 全局 Lua 状态的线程安全需要外部保证
2. 脚本中不应直接操作全局变量，应使用 `entity.vars`
3. 需要清理机制防止变量无限增长
