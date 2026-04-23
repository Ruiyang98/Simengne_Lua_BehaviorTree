# Lua脚本管理类设计计划

## 需求概述

新增一个Lua脚本管理类，每个实体一个，维护要执行的脚本列表，仿真引擎每500ms调度一次实体，实体就执行一次所有的脚本。

包含两种脚本类型：
1. **纯Lua战术规则脚本** - 判断范围内有敌方自动打击
2. **Lua+行为树混合脚本** - 每次执行Lua时调度一次行为树

## 架构设计

### 1. 核心组件

```
┌─────────────────────────────────────────────────────────────────┐
│                    EntityScriptManager                          │
│                    (实体脚本管理器 - 每个实体一个)                  │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐                      │
│  │  TacticalScript │  │   BTScript      │                      │
│  │  (纯Lua战术规则) │  │ (Lua+行为树)    │                      │
│  └─────────────────┘  └─────────────────┘                      │
│         │                      │                               │
│         │                      ▼                               │
│         │              ┌─────────────────┐                     │
│         │              │ 直接tick行为树   │                     │
│         │              │ tree.tickRoot() │                     │
│         │              └─────────────────┘                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌─────────────────────┐
                    │   仿真引擎每500ms调用  │
                    │  executeAllScripts() │
                    └─────────────────────┘
```

**说明**：
- 不需要ScriptScheduler，由仿真引擎直接每500ms调用每个实体的EntityScriptManager
- 不需要BehaviorTreeScheduler，BTScript直接tick自己的行为树

### 2. 类设计

#### 2.1 EntityScriptManager (实体脚本管理器)

```cpp
class EntityScriptManager {
public:
    // 构造函数
    EntityScriptManager(const std::string& entityId, 
                        simulation::SimControlInterface* sim,
                        BT::BehaviorTreeFactory* factory);
    
    // 添加纯Lua战术脚本
    bool addTacticalScript(const std::string& scriptName, const std::string& scriptCode);
    bool addTacticalScriptFromFile(const std::string& scriptName, const std::string& filePath);
    
    // 添加Lua+行为树混合脚本
    bool addBTScript(const std::string& scriptName, 
                     const std::string& scriptCode,
                     const std::string& xmlFile, 
                     const std::string& treeName);
    
    // 移除脚本
    bool removeScript(const std::string& scriptName);
    
    // 启用/禁用脚本
    bool enableScript(const std::string& scriptName);
    bool disableScript(const std::string& scriptName);
    
    // 执行所有脚本 (由仿真引擎每500ms调用)
    void executeAllScripts();
    
    // 获取实体ID
    const std::string& getEntityId() const;
    
    // 获取脚本列表
    std::vector<std::string> getScriptNames() const;
    
    // 检查脚本是否存在
    bool hasScript(const std::string& scriptName) const;
    
    // 获取脚本数量
    size_t getScriptCount() const;
    
private:
    std::string entityId_;
    simulation::SimControlInterface* simInterface_;
    BT::BehaviorTreeFactory* factory_;
    std::unordered_map<std::string, std::shared_ptr<Script>> scripts_;
    sol::state luaState_;  // 每个管理器有自己的Lua状态
    
    // 初始化Lua状态
    void initializeLuaState();
};
```

#### 2.2 Script 基类

```cpp
enum class ScriptType {
    TACTICAL,       // 纯Lua战术规则
    BEHAVIOR_TREE   // Lua+行为树
};

class Script {
public:
    Script(const std::string& name, ScriptType type);
    virtual ~Script();
    
    virtual void execute() = 0;  // 纯虚函数，子类实现
    
    // Getters
    const std::string& getName() const;
    ScriptType getType() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    
protected:
    std::string name_;
    ScriptType type_;
    bool enabled_;
};
```

#### 2.3 TacticalScript (纯Lua战术规则脚本)

```cpp
class TacticalScript : public Script {
public:
    TacticalScript(const std::string& name, const std::string& scriptCode, 
                   sol::state& luaState, const std::string& entityId,
                   simulation::SimControlInterface* sim);
    
    void execute() override;
    
private:
    sol::state& luaState_;
    std::string entityId_;
    simulation::SimControlInterface* simInterface_;
    sol::function executeFunc_;  // Lua执行函数
};
```

**Lua战术规则脚本示例：**
```lua
-- 战术规则：检测范围内敌人并自动打击
function execute(entity_id, sim, bt)
    local sensor_range = 20
    local attack_range = 10
    
    -- 获取实体位置
    local pos = sim.get_entity_position(entity_id)
    if not pos then return end
    
    -- 检测范围内敌人
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id and entity.type == "enemy" then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range then
                print(string.format("[Tactical] Enemy detected: %s at distance %.1f", entity.id, dist))
                
                if dist <= attack_range then
                    -- 在攻击范围内，执行打击
                    print(string.format("[Tactical] Attacking enemy: %s", entity.id))
                    -- 调用打击逻辑
                else
                    -- 移动到敌人位置
                    print(string.format("[Tactical] Moving toward enemy: %s", entity.id))
                end
            end
        end
    end
end
```

#### 2.4 BTScript (Lua+行为树混合脚本)

```cpp
class BTScript : public Script {
public:
    BTScript(const std::string& name, const std::string& scriptCode,
             const std::string& xmlFile, const std::string& treeName,
             sol::state& luaState, const std::string& entityId,
             simulation::SimControlInterface* sim,
             BT::BehaviorTreeFactory* factory);
    
    void execute() override;
    
    // 初始化行为树
    bool initializeBT();
    
    // 获取行为树状态
    BT::NodeStatus getStatus() const;
    
private:
    sol::state& luaState_;
    std::string entityId_;
    simulation::SimControlInterface* simInterface_;
    BT::BehaviorTreeFactory* factory_;
    
    // 行为树相关
    std::string xmlFile_;
    std::string treeName_;
    BT::Tree tree_;
    std::shared_ptr<BT::Blackboard> blackboard_;
    bool btInitialized_;
    
    // Lua执行函数
    sol::function executeFunc_;
};
```

**BTScript::execute() 实现逻辑：**
```cpp
void BTScript::execute() {
    // 1. 执行Lua逻辑（更新blackboard等）
    if (executeFunc_.valid()) {
        auto result = executeFunc_(entityId_, simInterface_);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[BTScript] Lua error: " << err.what() << std::endl;
        }
    }
    
    // 2. 初始化行为树（如果还没初始化）
    if (!btInitialized_) {
        if (!initializeBT()) {
            std::cerr << "[BTScript] Failed to initialize BT" << std::endl;
            return;
        }
    }
    
    // 3. 直接tick行为树（不再通过BehaviorTreeScheduler）
    if (tree_.rootNode()) {
        BT::NodeStatus status = tree_.tickRoot();
        
        // 如果行为树完成，可以重置或停止
        if (status != BT::NodeStatus::RUNNING) {
            // 行为树完成，可选择重新加载或停止
        }
    }
}
```

**Lua+行为树脚本示例：**
```lua
-- Lua+行为树混合脚本
-- 每次执行时先执行Lua逻辑，然后调度行为树

function execute(entity_id, sim, bt)
    -- Lua逻辑：更新blackboard数据
    local pos = sim.get_entity_position(entity_id)
    if pos then
        bt.set_blackboard(entity_id, "current_x", pos.x)
        bt.set_blackboard(entity_id, "current_y", pos.y)
    end
    
    -- 检测威胁并更新blackboard
    local has_threat = check_for_threats(entity_id, sim)
    bt.set_blackboard(entity_id, "has_threat", has_threat)
    
    print(string.format("[BTScript] Entity %s updated blackboard, has_threat=%s", 
                        entity_id, tostring(has_threat)))
end

function check_for_threats(entity_id, sim)
    local sensor_range = 15
    local pos = sim.get_entity_position(entity_id)
    if not pos then return false end
    
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id and (entity.type == "enemy" or entity.type == "threat") then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range then
                bt.set_blackboard(entity_id, "threat_id", entity.id)
                bt.set_blackboard(entity_id, "threat_x", entity.x)
                bt.set_blackboard(entity_id, "threat_y", entity.y)
                return true
            end
        end
    end
    return false
end
```

### 3. 与仿真引擎集成

仿真引擎需要维护一个EntityScriptManager的集合，每500ms遍历并调用executeAllScripts()：

```cpp
// 在MockSimController或其他仿真控制器中
class MockSimController {
    // ... 现有代码 ...
    
public:
    // 为实体创建脚本管理器
    std::shared_ptr<scripting::EntityScriptManager> createScriptManager(
        const std::string& entityId);
    
    // 移除实体的脚本管理器
    bool removeScriptManager(const std::string& entityId);
    
    // 获取实体的脚本管理器
    std::shared_ptr<scripting::EntityScriptManager> getScriptManager(
        const std::string& entityId);
    
private:
    // 实体脚本管理器集合
    std::unordered_map<std::string, std::shared_ptr<scripting::EntityScriptManager>> entityScriptManagers_;
    
    // 在update()中调用
    void updateScripts(double deltaTime);
    
    // 脚本更新计时器
    double scriptUpdateAccumulator_ = 0.0;
    static constexpr double SCRIPT_UPDATE_INTERVAL = 0.5; // 500ms
};

void MockSimController::update(double deltaTime) {
    if (state_ == 1) {
        simTime_ = simTime_ + deltaTime * timeScale_;
        
        // 更新脚本
        updateScripts(deltaTime);
    }
}

void MockSimController::updateScripts(double deltaTime) {
    scriptUpdateAccumulator_ += deltaTime * timeScale_;
    
    if (scriptUpdateAccumulator_ >= SCRIPT_UPDATE_INTERVAL) {
        scriptUpdateAccumulator_ -= SCRIPT_UPDATE_INTERVAL;
        
        // 遍历所有实体的脚本管理器并执行
        for (auto& pair : entityScriptManagers_) {
            pair.second->executeAllScripts();
        }
    }
}
```

### 4. Lua绑定API

在Lua中提供以下API：

```lua
-- 为实体创建脚本管理器
sim.create_script_manager(entity_id)

-- 添加纯Lua战术脚本
script_manager:add_tactical_script("auto_attack", [[
    function execute(entity_id, sim, bt)
        -- 战术规则逻辑
    end
]])

-- 添加Lua+行为树混合脚本
script_manager:add_bt_script("patrol_behavior", "bt_xml/patrol.xml", "PatrolTree", [[
    function execute(entity_id, sim, bt)
        -- Lua逻辑，在行为树tick之前执行
    end
]])

-- 启用/禁用脚本
script_manager:enable_script("auto_attack")
script_manager:disable_script("auto_attack")

-- 移除脚本
script_manager:remove_script("auto_attack")

-- 获取脚本列表
local scripts = script_manager:get_scripts()
```

### 5. 与现有系统集成

#### 5.1 与BehaviorTreeScheduler的关系

- **不需要BehaviorTreeScheduler**，BTScript直接管理自己的行为树
- BTScript在execute()中直接调用 `tree_.tickRoot()`
- 每个BTScript有自己的行为树实例，独立tick

#### 5.2 与LuaSimBinding的关系

- `LuaSimBinding` 中注册新的API函数用于创建和管理脚本管理器
- 保持现有的 `sim` 和 `bt` API不变

### 6. 文件结构

```
include/
  scripting/
    EntityScriptManager.h
    Script.h
    TacticalScript.h
    BTScript.h

src/
  scripting/
    EntityScriptManager.cpp
    Script.cpp
    TacticalScript.cpp
    BTScript.cpp

scripts/
  examples/
    tactical_auto_attack.lua      # 纯Lua战术规则示例
    bt_hybrid_patrol.lua          # Lua+行为树混合示例
```

## 实现步骤

### Phase 1: 基础框架
1. 创建 `Script` 基类
2. 创建 `TacticalScript` 类
3. 创建 `EntityScriptManager` 类
4. 添加基础Lua绑定

### Phase 2: 行为树集成
1. 创建 `BTScript` 类
2. BTScript直接tick自己的行为树（不通过BehaviorTreeScheduler）
3. 实现Lua+行为树混合执行逻辑

### Phase 3: 仿真引擎集成
1. 在 `MockSimController` 中添加脚本管理器集合
2. 实现500ms定时调用
3. 添加创建/移除脚本管理器的API

### Phase 4: Lua API完善
1. 完善Lua绑定API
2. 创建示例脚本
3. 添加命令行支持

### Phase 5: 测试验证
1. 单元测试
2. 集成测试
3. 性能测试

## 使用示例

### C++ 使用示例

```cpp
// 在仿真控制器中为实体创建脚本管理器
auto manager = simController->createScriptManager("entity_1");

// 添加纯Lua战术脚本
manager->addTacticalScript("auto_attack", R"(
    function execute(entity_id, sim, bt)
        -- 自动攻击逻辑
    end
)");

// 添加Lua+行为树混合脚本
manager->addBTScript("patrol", R"(
    function execute(entity_id, sim, bt)
        -- 更新blackboard
    end
)", "bt_xml/patrol.xml", "PatrolTree");
```

### Lua 使用示例

```lua
-- 创建脚本管理器
local manager = sim.create_script_manager("entity_1")

-- 添加自动攻击战术脚本
manager:add_tactical_script("auto_attack", [[
    function execute(entity_id, sim, bt)
        local sensor_range = 20
        local pos = sim.get_entity_position(entity_id)
        
        local entities = sim.get_all_entities()
        for _, entity in ipairs(entities) do
            if entity.id ~= entity_id and entity.type == "enemy" then
                local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
                if dist <= sensor_range then
                    print("[AutoAttack] Attacking enemy: " .. entity.id)
                    -- 执行攻击
                end
            end
        end
    end
]])

-- 添加巡逻行为树脚本
manager:add_bt_script("patrol", "bt_xml/patrol.xml", "PatrolTree", [[
    function execute(entity_id, sim, bt)
        -- 每次tick前更新数据
        local pos = sim.get_entity_position(entity_id)
        bt.set_blackboard(entity_id, "pos_x", pos.x)
        bt.set_blackboard(entity_id, "pos_y", pos.y)
    end
]])
```

## 设计决策说明

### 为什么每个管理器有自己的Lua状态？

1. **隔离性**：不同实体的脚本互不影响，一个实体脚本出错不会导致其他实体脚本崩溃
2. **安全性**：避免脚本间的全局变量污染和命名冲突
3. **可维护性**：每个实体的脚本环境独立，便于调试和排查问题
4. **灵活性**：不同实体可以加载不同版本的同名脚本

### 为什么不需要BehaviorTreeScheduler？

1. **简化架构**：BTScript直接管理自己的行为树，减少中间层
2. **独立控制**：每个实体的行为树独立tick，不受其他实体影响
3. **时序一致**：Lua逻辑执行后立即tick行为树，保证数据一致性
4. **资源隔离**：每个行为树实例独立，避免全局调度器的复杂性

### 权衡考虑

| 方案 | 优点 | 缺点 |
|------|------|------|
| 每个管理器独立Lua状态 | 隔离性好、安全性高、易调试 | 内存占用稍高 |
| 共享全局Lua状态 | 内存占用低 | 容易有命名冲突 |

| 方案 | 优点 | 缺点 |
|------|------|------|
| BTScript直接tick | 架构简单、时序一致 | 每个行为树独立管理 |
| 使用BehaviorTreeScheduler | 集中管理 | 增加复杂性、需要额外调度 |

## 注意事项

1. **线程安全**：`EntityScriptManager` 需要处理多线程访问（如果仿真引擎是多线程的）
2. **Lua状态隔离**：每个 `EntityScriptManager` 有自己的 `sol::state`，避免脚本间干扰
3. **性能考虑**：500ms调度间隔是默认值，可根据需求调整
4. **错误处理**：Lua脚本执行错误不应影响其他脚本或系统稳定性
5. **行为树生命周期**：BTScript需要管理行为树的初始化和销毁
