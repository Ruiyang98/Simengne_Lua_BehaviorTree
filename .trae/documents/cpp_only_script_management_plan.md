# C++ 层独立脚本管理设计方案

## 核心思想

所有脚本相关功能完全在 C++ 层实现，不暴露任何脚本管理接口给 Lua。

## 架构

```
Lua 层:
- 只能操作 entity, sim, bt 等基础表
- 无法直接操作脚本管理器
- 通过 entity.vars 间接影响脚本行为

C++ 层:
MockSimController
├── addEntity() → 自动创建 EntityScriptManager
├── removeEntity() → 自动移除 EntityScriptManager
│
└── EntityScriptManager (每个实体一个)
    ├── 自动加载预定义脚本
    ├── 自动执行脚本
    └── 脚本状态完全由 C++ 管理
```

## 实现方案

### 步骤 1: 完全移除 LuaSimBinding 中的脚本管理 API

```cpp
// LuaSimBinding.cpp - registerSimAPI()
// 删除所有脚本管理相关函数：
// - create_script_manager
// - remove_script_manager
// - get_script_manager
// - has_script_manager
```

### 步骤 2: MockSimController 自动管理脚本

```cpp
// MockSimController.h
class MockSimController {
public:
    // ... existing methods ...
    
    // 添加实体时指定脚本配置（可选）
    VehicleID addEntity(const std::string& type, double x, double y, double z);
    
    // 为实体添加脚本（C++ 层调用）
    bool addScriptToEntity(const std::string& entityId, 
                           const std::string& scriptName,
                           const std::string& scriptCode);
    
    // 从文件加载脚本并添加到实体
    bool addScriptToEntityFromFile(const std::string& entityId,
                                    const std::string& scriptName,
                                    const std::string& filePath);
    
    // 移除实体脚本
    bool removeScriptFromEntity(const std::string& entityId,
                                 const std::string& scriptName);
    
    // 启用/禁用实体脚本
    bool enableEntityScript(const std::string& entityId,
                            const std::string& scriptName);
    bool disableEntityScript(const std::string& entityId,
                             const std::string& scriptName);
    
    // 获取实体脚本列表
    std::vector<std::string> getEntityScriptNames(const std::string& entityId);
};
```

### 步骤 3: 实现 C++ 层脚本管理

```cpp
// MockSimController.cpp

VehicleID MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    VehicleID vehicleId = generateVehicleId();
    entities_[vehicleId] = Entity(vehicleId, type, x, y, z);
    
    // 自动创建脚本管理器
    std::string entityId = std::to_string(vehicleId.vehicle);
    createScriptManager(entityId);
    
    // 根据实体类型自动加载默认脚本（可选）
    if (type == "guard") {
        addScriptToEntityFromFile(entityId, "patrol", "scripts/default/guard_patrol.lua");
        addScriptToEntityFromFile(entityId, "attack", "scripts/default/guard_attack.lua");
    } else if (type == "enemy") {
        addScriptToEntityFromFile(entityId, "wander", "scripts/default/enemy_wander.lua");
    }
    
    if (verbose_) {
        std::cout << "[MockSim] Entity added: vehicle=" << vehicleId.vehicle
                  << " (type: " << type << ")" << std::endl;
    }
    
    return vehicleId;
}

bool MockSimController::addScriptToEntity(const std::string& entityId,
                                           const std::string& scriptName,
                                           const std::string& scriptCode) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        std::cerr << "[MockSim] No script manager for entity: " << entityId << std::endl;
        return false;
    }
    
    return manager->addTacticalScript(scriptName, scriptCode);
}

bool MockSimController::addScriptToEntityFromFile(const std::string& entityId,
                                                   const std::string& scriptName,
                                                   const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[MockSim] Cannot open script file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return addScriptToEntity(entityId, scriptName, buffer.str());
}

bool MockSimController::removeScriptFromEntity(const std::string& entityId,
                                                const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->removeScript(scriptName);
}

bool MockSimController::enableEntityScript(const std::string& entityId,
                                            const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->enableScript(scriptName);
}

bool MockSimController::disableEntityScript(const std::string& entityId,
                                             const std::string& scriptName) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return false;
    }
    
    return manager->disableScript(scriptName);
}

std::vector<std::string> MockSimController::getEntityScriptNames(const std::string& entityId) {
    auto manager = getScriptManager(entityId);
    if (!manager) {
        return {};
    }
    
    return manager->getScriptNames();
}
```

### 步骤 4: Lua 脚本通过 entity.vars 通信

由于 Lua 层无法直接操作脚本管理器，脚本间通信通过 `entity.vars`：

```lua
-- scripts/default/guard_patrol.lua
function execute(state)
    -- 使用自己的 state
    state.current_point = state.current_point or "A"
    state.patrol_count = (state.patrol_count or 0) + 1
    
    -- 与其他脚本共享数据
    entity.set_var("patrol_point", state.current_point)
    entity.set_var("patrol_count", state.patrol_count)
    
    print(string.format("[Patrol] Entity %s at point %s", entity.id, state.current_point))
end

-- scripts/default/guard_attack.lua
function execute(state)
    -- 独立的 state
    state.attack_count = (state.attack_count or 0) + 1
    
    -- 读取 patrol 脚本的数据
    local patrol_point = entity.get_var("patrol_point")
    
    print(string.format("[Attack] Entity %s attacking from %s", 
        entity.id, tostring(patrol_point)))
end
```

### 步骤 5: C++ 层控制脚本生命周期

```cpp
// 在 main.cpp 或测试代码中使用

int main() {
    // ... setup ...
    
    MockSimController* sim = new MockSimController();
    SimControlInterface::setInstance(sim);
    
    // 创建实体（自动创建脚本管理器）
    auto guard1 = sim->addEntity("guard", 0, 0, 0);
    auto guard2 = sim->addEntity("guard", 10, 0, 0);
    auto enemy1 = sim->addEntity("enemy", 50, 50, 0);
    
    // C++ 层动态添加脚本
    sim->addScriptToEntityFromFile(
        std::to_string(guard1.vehicle), 
        "custom_behavior",
        "scripts/custom/guard_custom.lua"
    );
    
    // 禁用某个脚本
    sim->disableEntityScript(std::to_string(guard1.vehicle), "patrol");
    
    // 启用脚本
    sim->enableEntityScript(std::to_string(guard1.vehicle), "patrol");
    
    // 移除脚本
    sim->removeScriptFromEntity(std::to_string(guard1.vehicle), "custom_behavior");
    
    // 获取脚本列表
    auto scripts = sim->getEntityScriptNames(std::to_string(guard1.vehicle));
    for (const auto& name : scripts) {
        std::cout << "Script: " << name << std::endl;
    }
    
    // 启动模拟（脚本自动执行）
    sim->start();
    
    // ...
}
```

## 文件修改清单

### 删除 Lua API

1. **src/scripting/LuaSimBinding.cpp**
   - 删除 `create_script_manager` 函数注册
   - 删除 `remove_script_manager` 函数注册
   - 删除 `get_script_manager` 函数注册
   - 删除 `has_script_manager` 函数注册

### 添加 C++ API

2. **include/simulation/MockSimController.h**
   - 添加脚本管理方法声明

3. **src/simulation/MockSimController.cpp**
   - 修改 `addEntity()` 自动创建脚本管理器
   - 实现脚本管理方法

### 更新示例

4. **删除或简化 Lua 示例脚本**
   - `scripts/examples/script_manager_demo.lua` - 简化为只展示 entity.vars 使用
   - `scripts/examples/tactical_auto_attack.lua` - 保持作为默认脚本示例

## 优势

1. **完全隔离**：Lua 层无法直接操作脚本，安全性更高
2. **简化 Lua**：Lua 只关注脚本逻辑，不关注管理
3. **C++ 控制**：脚本生命周期完全由 C++ 控制
4. **灵活性**：C++ 层可以动态添加/移除/启用/禁用脚本

## Lua 层可用接口

```lua
-- 基础表（保留）
entity.id              -- 实体ID
entity.vars            -- 实体变量表
entity.set_var()       -- 设置变量
entity.get_var()       -- 获取变量
sim.add_entity()       -- 添加实体（自动创建脚本管理器）
sim.remove_entity()    -- 移除实体（自动移除脚本管理器）
sim.get_entity_position()
sim.get_all_entities()
-- ... 其他 sim 函数

-- 脚本管理接口（删除）
-- sim.create_script_manager()  -- 删除
-- sim.remove_script_manager()  -- 删除
-- sim.get_script_manager()     -- 删除
-- sim.has_script_manager()     -- 删除
```
