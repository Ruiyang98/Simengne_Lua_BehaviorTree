# 自动创建脚本管理器设计方案

## 需求分析

用户希望：
1. **移除 Lua 层的 `create_script_manager` 函数**
2. **在 C++ 中创建实体时自动调用创建脚本管理器**
3. 简化 Lua 使用流程，不需要手动创建脚本管理器

## 当前架构

```
Lua 层:
sim.create_script_manager(entity_id)  ← 需要手动调用

C++ 层:
MockSimController::createScriptManager(entity_id)
```

## 新架构

```
C++ 层:
MockSimController::addEntity() 
    └── 自动调用 createScriptManager(entity_id)

Lua 层:
-- 不需要 create_script_manager，直接使用
local manager = sim.get_script_manager(entity_id)
manager:add_tactical_script(...)
```

## 实现方案

### 步骤 1: 修改 MockSimController::addEntity

在创建实体时自动创建脚本管理器：

```cpp
VehicleID MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    VehicleID vehicleId = generateVehicleId();
    entities_[vehicleId] = Entity(vehicleId, type, x, y, z);

    // Auto-create script manager for this entity
    std::string entityId = std::to_string(vehicleId.vehicle);
    createScriptManager(entityId);

    if (verbose_) {
        std::cout << "[MockSim] Entity added: vehicle=" << vehicleId.vehicle
                  << " (type: " << type << ") at ("
                  << x << ", " << y << ", " << z << ")" << std::endl;
    }

    return vehicleId;
}
```

### 步骤 2: 修改 LuaSimBinding，移除 create_script_manager

从 `registerSimAPI()` 中移除：
- `sim.create_script_manager()`
- `sim.remove_script_manager()`
- `sim.has_script_manager()`

保留：
- `sim.get_script_manager()` - 用于获取已存在的脚本管理器

### 步骤 3: 修改 LuaSimBinding::registerSimAPI

```cpp
void LuaSimBinding::registerSimAPI() {
    // ... existing code ...
    
    // Remove create_script_manager, remove_script_manager, has_script_manager
    // Keep only get_script_manager for accessing existing managers
    
    simTable.set_function("get_script_manager", [this](const std::string& entityId) -> sol::optional<sol::table> {
        MockSimController* mockSim = static_cast<MockSimController*>(SimControlInterface::getInstance());
        if (!mockSim) {
            return sol::nullopt;
        }
        
        auto manager = mockSim->getScriptManager(entityId);
        if (!manager) {
            return sol::nullopt;
        }
        
        // Create script manager table
        sol::table managerTable = luaState_->create_table();
        
        managerTable.set_function("add_tactical_script", [manager](const std::string& scriptName, const std::string& scriptCode) -> bool {
            return manager->addTacticalScript(scriptName, scriptCode);
        });
        
        managerTable.set_function("add_bt_script", [manager](const std::string& scriptName, 
                                                              const std::string& xmlFile,
                                                              const std::string& treeName,
                                                              const std::string& scriptCode) -> bool {
            return manager->addBTScript(scriptName, scriptCode, xmlFile, treeName);
        });
        
        // ... other methods ...
        
        return managerTable;
    });
}
```

### 步骤 4: 更新示例脚本

```lua
-- Old way (manual creation):
-- local manager = sim.create_script_manager(entity_id)

-- New way (auto-created, just get it):
local vid = sim.add_entity("guard", 0, 0, 0)
local entity_id = tostring(vid.vehicle)

-- Script manager is auto-created, just get it
local manager = sim.get_script_manager(entity_id)
if manager then
    manager:add_tactical_script("patrol", patrol_script)
end
```

## 文件修改清单

1. **src/simulation/MockSimController.cpp**
   - 修改 `addEntity()` 方法，自动创建脚本管理器

2. **src/scripting/LuaSimBinding.cpp**
   - 移除 `create_script_manager` 函数注册
   - 移除 `remove_script_manager` 函数注册
   - 移除 `has_script_manager` 函数注册
   - 保留 `get_script_manager` 函数

3. **scripts/examples/script_manager_demo.lua**
   - 更新示例，移除 `create_script_manager` 调用
   - 使用 `add_entity` 后直接使用 `get_script_manager`

4. **scripts/examples/tactical_auto_attack.lua**
   - 更新注释说明脚本管理器是自动创建的

## 优势

1. **简化 Lua 代码**：不需要手动创建脚本管理器
2. **自动化**：实体和脚本管理器生命周期绑定
3. **减少错误**：避免忘记创建脚本管理器的问题
4. **更清晰**：Lua 层只关注脚本逻辑，不关注管理器创建

## 注意事项

1. 每个实体创建时都会自动创建脚本管理器，即使不需要脚本
2. 如果需要禁用自动创建，可以添加配置选项
3. 移除实体时会自动移除脚本管理器（已有实现）
