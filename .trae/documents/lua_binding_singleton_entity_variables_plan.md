# LuaSimBinding 单例化与实体变量隔离设计方案

## 当前架构分析

### 现有问题

1. **LuaSimBinding 非单例**：每次创建 `LuaSimBinding` 对象都会初始化新的 Lua 状态
2. **EntityScriptManager 拥有独立 Lua 状态**：每个实体管理器都有自己的 `sol::state luaState_`
3. **API 重复注册**：`LuaSimBinding` 和 `EntityScriptManager` 都注册相似的 Lua API
4. **变量无隔离**：同一实体的不同脚本共享全局环境，变量可能冲突

### 当前代码结构

```
main.cpp
  └── LuaSimBinding (全局唯一，但非单例)
        └── sol::state (Lua 全局状态)
        └── 注册 sim 表 API
        └── 注册 create_script_manager 函数

MockSimController
  └── entityScriptManagers_ (map<entityId, EntityScriptManager>)
        └── 每个 EntityScriptManager 有自己的 sol::state
        └── 重复注册 Lua API
```

## 目标

1. **LuaSimBinding 单例化**：全局只有一个 Lua 状态
2. **统一 API 注册**：只在 LuaSimBinding 中注册一次 API
3. **实体变量隔离**：每个实体有自己的变量空间，但共享同一个 Lua 状态
4. **EntityScriptManager 轻量化**：只负责脚本执行，不管理 Lua 状态

## 设计方案

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    LuaSimBinding (单例)                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              全局 sol::state (唯一)                   │   │
│  │  ┌───────────────────────────────────────────────┐  │   │
│  │  │  sim 表 (全局 API)                              │  │   │
│  │  │  - start/pause/resume/stop/reset               │  │   │
│  │  │  - add_entity/remove_entity/move_entity        │  │   │
│  │  │  - create_script_manager                       │  │   │
│  │  └───────────────────────────────────────────────┘  │   │
│  │  ┌───────────────────────────────────────────────┐  │   │
│  │  │  bt 表 (行为树 API)                             │  │   │
│  │  │  - load/execute/stop                           │  │   │
│  │  │  - blackboard 操作                             │  │   │
│  │  └───────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              EntityScriptManager (每个实体一个)               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  引用全局 sol::state& (不拥有)                        │   │
│  │  ┌───────────────────────────────────────────────┐  │   │
│  │  │  entity 表 (实体隔离)                            │  │   │
│  │  │  - id: 实体ID                                    │  │   │
│  │  │  │  - vars: 变量存储表 (每个实体独立)              │  │   │
│  │  │  │  - set_var/get_var/has_var/remove_var         │  │   │
│  │  └───────────────────────────────────────────────┘  │   │
│  │  ┌───────────────────────────────────────────────┐  │   │
│  │  │  脚本执行环境 (沙箱)                             │  │   │
│  │  │  - 使用 _ENV 隔离全局变量                        │  │   │
│  │  │  - 访问 entity.vars 存储状态                   │  │   │
│  │  └───────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计

#### 1. LuaSimBinding 单例化

```cpp
// include/scripting/LuaSimBinding.h
class LuaSimBinding {
public:
    // 获取单例实例
    static LuaSimBinding& getInstance();
    
    // 禁止拷贝和赋值
    LuaSimBinding(const LuaSimBinding&) = delete;
    LuaSimBinding& operator=(const LuaSimBinding&) = delete;
    
    // 初始化（只需调用一次）
    bool initialize(BT::BehaviorTreeFactory* factory = nullptr);
    
    // 获取 Lua 状态
    sol::state& getState();
    
    // 检查是否已初始化
    bool isInitialized() const;
    
private:
    LuaSimBinding();  // 私有构造函数
    ~LuaSimBinding();
    
    std::unique_ptr<sol::state> luaState_;
    bool initialized_;
    std::string lastError_;
    
    void registerFunctions();
    void registerSimAPI();
    void registerBehaviorTreeAPI();
    void registerUtilityFunctions();
};
```

#### 2. EntityScriptManager 修改

```cpp
// include/scripting/EntityScriptManager.h
class EntityScriptManager {
public:
    // 构造函数改为接收全局 Lua 状态引用
    EntityScriptManager(const std::string& entityId, 
                        sol::state& globalLuaState,
                        BT::BehaviorTreeFactory* factory);
    
    // 获取实体变量表
    sol::table& getVariables() { return variables_; }
    
private:
    std::string entityId_;
    BT::BehaviorTreeFactory* factory_;
    sol::state& luaState_;  // 引用全局 Lua 状态
    
    sol::table entityTable_;    // entity 表 (包含 id, vars, 方法)
    sol::table variables_;      // entity.vars 变量存储
    sol::table env_;            // 脚本执行环境 (沙箱)
    
    void initializeEntityTable();
    void createSandbox();
};
```

#### 3. Lua 实体变量 API

```lua
-- EntityScriptManager 为每个实体创建的表结构
entity = {
    id = "entity_001",  -- 实体ID
    
    -- 变量存储表 (每个实体独立)
    vars = {},
    
    -- 变量操作方法
    set_var = function(key, value)
        entity.vars[key] = value
    end,
    
    get_var = function(key, default_value)
        local val = entity.vars[key]
        if val == nil then
            return default_value
        end
        return val
    end,
    
    has_var = function(key)
        return entity.vars[key] ~= nil
    end,
    
    remove_var = function(key)
        entity.vars[key] = nil
    end,
    
    clear_vars = function()
        entity.vars = {}
    end,
    
    get_all_vars = function()
        return entity.vars
    end
}
```

#### 4. 脚本执行沙箱

```cpp
// TacticalScript::execute() 实现
void TacticalScript::execute() {
    if (!enabled_ || !executeFunc_.valid()) return;
    
    try {
        // 在沙箱环境中执行脚本
        // 设置 _ENV 为 entity 表，隔离全局变量
        sol::table entityTable = luaState_["entity"];
        sol::set_environment(entityTable, executeFunc_);
        
        // 调用 execute 函数，传入 entity_id
        auto result = executeFunc_(entityId_);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error: " << err.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Exception: " << e.what() << std::endl;
    }
}
```

#### 5. 脚本使用示例

```lua
-- 战术脚本示例：自动攻击
function execute(entity_id)
    -- 使用 entity 表存储变量
    local attack_count = entity.get_var("attack_count", 0)
    local last_target = entity.get_var("last_target")
    
    -- 获取实体位置
    local pos = sim.get_entity_position(entity.id)
    if not pos then return end
    
    -- 查找敌人
    local entities = sim.get_all_entities()
    for _, e in ipairs(entities) do
        if e.id ~= entity.id and e.type == "enemy" then
            local dist = math.sqrt((pos.x - e.x)^2 + (pos.y - e.y)^2)
            
            if dist <= 10 then
                -- 攻击逻辑
                print(string.format("[%s] Attacking enemy %d! (count: %d)", 
                    entity.id, e.id, attack_count + 1))
                
                -- 更新变量
                entity.set_var("attack_count", attack_count + 1)
                entity.set_var("last_target", e.id)
                break
            end
        end
    end
end
```

## 文件修改清单

### 1. 修改文件

| 文件 | 修改内容 |
|------|----------|
| `include/scripting/LuaSimBinding.h` | 改为单例模式，添加静态 getInstance() 方法 |
| `src/scripting/LuaSimBinding.cpp` | 实现单例，移除 btBridge_ 成员（移到外部管理） |
| `include/scripting/EntityScriptManager.h` | 修改构造函数，添加 entityTable_ 和 variables_ 成员 |
| `src/scripting/EntityScriptManager.cpp` | 移除独立 luaState_，使用全局状态引用，创建 entity 表 |
| `include/scripting/TacticalScript.h` | 添加环境表支持 |
| `src/scripting/TacticalScript.cpp` | 在沙箱环境中执行脚本 |
| `include/scripting/BTScript.h` | 添加环境表支持 |
| `src/scripting/BTScript.cpp` | 在沙箱环境中执行脚本 |
| `src/main.cpp` | 使用 LuaSimBinding::getInstance() |
| `include/simulation/MockSimController.h` | 添加 LuaSimBinding 初始化 |
| `src/simulation/MockSimController.cpp` | 修改 EntityScriptManager 创建逻辑 |

### 2. 新增文件

无新增文件，所有功能通过修改现有文件实现。

## 实施步骤

### 步骤 1: LuaSimBinding 单例化

1. 修改 `LuaSimBinding.h`：
   - 添加 `static LuaSimBinding& getInstance()`
   - 构造函数改为私有
   - 删除拷贝构造函数和赋值运算符

2. 修改 `LuaSimBinding.cpp`：
   - 实现 `getInstance()` 方法
   - 移除 `btBridge_` 成员（由外部管理）
   - `initialize()` 添加 `BT::BehaviorTreeFactory*` 参数

### 步骤 2: EntityScriptManager 改造

1. 修改构造函数：
   ```cpp
   EntityScriptManager(const std::string& entityId, 
                       sol::state& globalLuaState,
                       BT::BehaviorTreeFactory* factory);
   ```

2. 移除成员：
   - 删除 `sol::state luaState_`
   - 改为 `sol::state& luaState_` 引用

3. 添加成员：
   - `sol::table entityTable_` - entity 表
   - `sol::table variables_` - 变量存储
   - `sol::table env_` - 沙箱环境

4. 实现 `initializeEntityTable()`：
   - 创建 `entity` 表
   - 注册 `set_var/get_var/has_var/remove_var` 方法
   - 初始化 `entity.vars` 为空表

5. 实现 `createSandbox()`：
   - 创建沙箱环境表
   - 设置 `__index` 元方法访问全局表

### 步骤 3: TacticalScript 和 BTScript 改造

1. 修改构造函数接收环境表
2. 在 `execute()` 中使用 `sol::set_environment()` 设置执行环境

### 步骤 4: MockSimController 适配

1. 在初始化时调用 `LuaSimBinding::getInstance().initialize(factory)`
2. 创建 EntityScriptManager 时传入全局 Lua 状态引用

### 步骤 5: main.cpp 适配

1. 使用 `LuaSimBinding::getInstance()` 替代 `new LuaSimBinding()`
2. 移除手动初始化代码（移到 MockSimController）

## 预期效果

1. **内存优化**：所有实体共享同一个 Lua 状态，大幅减少内存占用
2. **API 统一**：只在 LuaSimBinding 中注册一次 API
3. **变量隔离**：每个实体的变量存储在独立的 `entity.vars` 表中
4. **脚本安全**：沙箱环境防止脚本污染全局命名空间

## 注意事项

1. **线程安全**：`LuaSimBinding::getInstance()` 返回的 Lua 状态不是线程安全的，需要外部同步
2. **脚本兼容性**：现有脚本需要修改，使用 `entity.vars` 替代全局变量
3. **生命周期**：确保 LuaSimBinding 在 EntityScriptManager 之前初始化，之后销毁

## 示例代码迁移

### 旧代码（全局变量）
```lua
-- 不推荐：使用全局变量
counter = 0
function execute(entity_id, sim)
    counter = counter + 1
    print("Count: " .. counter)
end
```

### 新代码（实体变量）
```lua
-- 推荐：使用 entity.vars
function execute(entity_id)
    local counter = entity.get_var("counter", 0)
    counter = counter + 1
    entity.set_var("counter", counter)
    print("[" .. entity.id .. "] Count: " .. counter)
end
```
