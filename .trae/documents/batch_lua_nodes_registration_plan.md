# 批量注册 Lua 行为树节点和批量加载行为树实现计划

## 问题概述

当前需要逐个注册 Lua 节点和逐个加载行为树 XML 文件，这在仿真场景中有大量节点和行为树时非常繁琐。需要提供批量注册的 API，方便仿真直接调用。

## 现状分析

### 当前 API

1. **单个节点注册**：
   ```lua
   bt.register_action("LuaMoveTo", function(params) ... end)
   bt.register_condition("LuaCheckHealth", function(params) ... end)
   ```

2. **单个行为树加载**：
   ```lua
   bt.load_file("bt_xml/tree1.xml")
   bt.load_file("bt_xml/tree2.xml")
   ```

### 期望的批量 API

```lua
-- 批量注册节点
bt.register_actions({
    LuaMoveTo = function(params) ... end,
    LuaPatrol = function(params) ... end,
    LuaAttack = function(params) ... end
})

bt.register_conditions({
    LuaCheckHealth = function(params) ... end,
    LuaHasTarget = function(params) ... end
})

-- 批量加载行为树
bt.load_files({
    "bt_xml/patrol_trees.xml",
    "bt_xml/combat_trees.xml",
    "bt_xml/npc_behaviors.xml"
})

-- 或者加载整个目录
bt.load_directory("bt_xml/npcs/")
```

## 实现方案

### 方案一：批量注册 API（推荐）

#### 1. 批量注册动作节点

**C++ 实现**：
```cpp
// 在 LuaBehaviorTreeBridge 类中添加
bool registerLuaActions(sol::table actions);

// 实现
bool LuaBehaviorTreeBridge::registerLuaActions(sol::table actions) {
    bool allSuccess = true;
    for (auto& pair : actions) {
        std::string name = pair.first.as<std::string>();
        sol::protected_function func = pair.second;
        
        if (!func.valid()) {
            lastError_ = "Invalid function for action: " + name;
            allSuccess = false;
            continue;
        }
        
        LuaActionNode::setLuaFunction(name, func);
    }
    return allSuccess;
}
```

**Lua API**：
```lua
bt.register_actions({
    LuaMoveTo = function(params)
        local entity_id = params.entity_id
        local x = tonumber(params.target_x)
        local y = tonumber(params.target_y)
        sim.move_entity(entity_id, x, y, 0)
        return "SUCCESS"
    end,
    
    LuaPatrol = function(params)
        local entity_id = params.entity_id
        local radius = tonumber(params.radius) or 5
        -- 巡逻逻辑
        return "SUCCESS"
    end,
    
    LuaAttack = function(params)
        local attacker = params.attacker_id
        local target = params.target_id
        -- 攻击逻辑
        return "SUCCESS"
    end
})
```

#### 2. 批量注册条件节点

**C++ 实现**：
```cpp
bool registerLuaConditions(sol::table conditions);

// 实现与 registerLuaActions 类似
```

**Lua API**：
```lua
bt.register_conditions({
    LuaHasTarget = function(params)
        local entity_id = params.entity_id
        -- 检查是否有目标
        return true
    end,
    
    LuaIsHealthy = function(params)
        local entity_id = params.entity_id
        local min_health = tonumber(params.min_health) or 50
        -- 检查健康状态
        return true
    end
})
```

#### 3. 批量加载行为树文件

**C++ 实现**：
```cpp
// 批量加载多个 XML 文件
bool loadBehaviorTreeFiles(sol::table fileList);

// 实现
bool LuaBehaviorTreeBridge::loadBehaviorTreeFiles(sol::table fileList) {
    bool allSuccess = true;
    for (auto& pair : fileList) {
        std::string filePath = pair.second.as<std::string>();
        
        if (!loadBehaviorTreeFromFile(filePath)) {
            // 记录错误但继续加载其他文件
            allSuccess = false;
        }
    }
    return allSuccess;
}
```

**Lua API**：
```lua
-- 批量加载多个文件
bt.load_files({
    "bt_xml/common_trees.xml",
    "bt_xml/npc_trees.xml",
    "bt_xml/boss_trees.xml"
})
```

#### 4. 加载整个目录

**C++ 实现**：
```cpp
// 加载目录下所有 XML 文件
bool loadBehaviorTreeDirectory(const std::string& dirPath);

// 实现
bool LuaBehaviorTreeBridge::loadBehaviorTreeDirectory(const std::string& dirPath) {
    try {
        std::vector<std::string> xmlFiles;
        
        // 遍历目录获取所有 .xml 文件
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                xmlFiles.push_back(entry.path().string());
            }
        }
        
        // 按文件名排序，确保加载顺序一致
        std::sort(xmlFiles.begin(), xmlFiles.end());
        
        bool allSuccess = true;
        for (const auto& file : xmlFiles) {
            if (!loadBehaviorTreeFromFile(file)) {
                allSuccess = false;
            }
        }
        
        return allSuccess;
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load directory: ") + e.what();
        return false;
    }
}
```

**Lua API**：
```lua
-- 加载整个目录
bt.load_directory("bt_xml/npcs/")

-- 或者递归加载子目录
bt.load_directory("bt_xml/", true)  -- true 表示递归
```

### 方案二：节点库/模块系统（高级）

除了基本的批量注册，还可以提供模块化的节点库系统：

```lua
-- 定义一个节点库
local npc_nodes = {
    actions = {
        LuaPatrol = function(params) ... end,
        LuaFlee = function(params) ... end,
        LuaChase = function(params) ... end
    },
    conditions = {
        LuaCanSeePlayer = function(params) ... end,
        LuaIsLowHealth = function(params) ... end
    }
}

-- 注册整个库
bt.register_node_library("npc", npc_nodes)

-- 使用时可以通过前缀访问（可选）
-- <LuaAction lua_node_name="npc:LuaPatrol" ... />
```

## 详细实现步骤

### 步骤 1：修改 LuaBehaviorTreeBridge.h

添加新的公共方法声明：

```cpp
// Batch register Lua action nodes
bool registerLuaActions(sol::table actions);

// Batch register Lua condition nodes
bool registerLuaConditions(sol::table conditions);

// Batch load behavior tree files
bool loadBehaviorTreeFiles(sol::table fileList);

// Load all XML files from a directory
bool loadBehaviorTreeDirectory(const std::string& dirPath, bool recursive = false);
```

### 步骤 2：修改 LuaBehaviorTreeBridge.cpp

实现上述方法，并在 `registerLuaAPI()` 中注册到 Lua：

```cpp
void LuaBehaviorTreeBridge::registerLuaAPI() {
    // ... 现有代码 ...
    
    // Batch register actions
    btTable.set_function("register_actions", [this](sol::table actions) -> bool {
        return registerLuaActions(actions);
    });
    
    // Batch register conditions
    btTable.set_function("register_conditions", [this](sol::table conditions) -> bool {
        return registerLuaConditions(conditions);
    });
    
    // Batch load files
    btTable.set_function("load_files", [this](sol::table fileList) -> bool {
        return loadBehaviorTreeFiles(fileList);
    });
    
    // Load directory
    btTable.set_function("load_directory", [this](const std::string& dirPath, 
                                                   sol::optional<bool> recursive) -> bool {
        return loadBehaviorTreeDirectory(dirPath, recursive.value_or(false));
    });
}
```

### 步骤 3：添加头文件依赖

需要在 LuaBehaviorTreeBridge.cpp 中添加：

```cpp
#include <filesystem>
#include <algorithm>
```

### 步骤 4：实现批量注册方法

```cpp
bool LuaBehaviorTreeBridge::registerLuaActions(sol::table actions) {
    if (!actions.valid()) {
        lastError_ = "Invalid actions table";
        return false;
    }
    
    bool allSuccess = true;
    int count = 0;
    
    for (auto& pair : actions) {
        if (!pair.first.is<std::string>()) {
            continue;
        }
        
        std::string name = pair.first.as<std::string>();
        
        if (!pair.second.is<sol::protected_function>()) {
            lastError_ = "Action '" + name + "' is not a function";
            allSuccess = false;
            continue;
        }
        
        sol::protected_function func = pair.second;
        
        if (!func.valid()) {
            lastError_ = "Invalid function for action: " + name;
            allSuccess = false;
            continue;
        }
        
        LuaActionNode::setLuaFunction(name, func);
        count++;
    }
    
    std::cout << "[LuaBehaviorTreeBridge] Registered " << count << " actions" << std::endl;
    return allSuccess;
}
```

### 步骤 5：实现批量加载方法

```cpp
bool LuaBehaviorTreeBridge::loadBehaviorTreeFiles(sol::table fileList) {
    if (!fileList.valid()) {
        lastError_ = "Invalid file list table";
        return false;
    }
    
    bool allSuccess = true;
    int count = 0;
    int successCount = 0;
    
    for (auto& pair : fileList) {
        if (!pair.second.is<std::string>()) {
            continue;
        }
        
        std::string filePath = pair.second.as<std::string>();
        count++;
        
        if (loadBehaviorTreeFromFile(filePath)) {
            successCount++;
            std::cout << "[LuaBehaviorTreeBridge] Loaded: " << filePath << std::endl;
        } else {
            allSuccess = false;
            std::cerr << "[LuaBehaviorTreeBridge] Failed to load: " << filePath 
                      << " (" << lastError_ << ")" << std::endl;
        }
    }
    
    std::cout << "[LuaBehaviorTreeBridge] Loaded " << successCount << "/" << count 
              << " behavior tree files" << std::endl;
    return allSuccess;
}
```

### 步骤 6：实现目录加载方法

```cpp
bool LuaBehaviorTreeBridge::loadBehaviorTreeDirectory(const std::string& dirPath, bool recursive) {
    try {
        if (!std::filesystem::exists(dirPath)) {
            lastError_ = "Directory does not exist: " + dirPath;
            return false;
        }
        
        if (!std::filesystem::is_directory(dirPath)) {
            lastError_ = "Path is not a directory: " + dirPath;
            return false;
        }
        
        std::vector<std::string> xmlFiles;
        
        auto iterator = recursive ? 
            std::filesystem::recursive_directory_iterator(dirPath) :
            std::filesystem::directory_iterator(dirPath);
        
        for (const auto& entry : iterator) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                xmlFiles.push_back(entry.path().string());
            }
        }
        
        // 按文件名排序
        std::sort(xmlFiles.begin(), xmlFiles.end());
        
        bool allSuccess = true;
        int successCount = 0;
        
        for (const auto& file : xmlFiles) {
            if (loadBehaviorTreeFromFile(file)) {
                successCount++;
                std::cout << "[LuaBehaviorTreeBridge] Loaded: " << file << std::endl;
            } else {
                allSuccess = false;
                std::cerr << "[LuaBehaviorTreeBridge] Failed to load: " << file 
                          << " (" << lastError_ << ")" << std::endl;
            }
        }
        
        std::cout << "[LuaBehaviorTreeBridge] Loaded " << successCount << "/" 
                  << xmlFiles.size() << " XML files from " << dirPath << std::endl;
        
        return allSuccess;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to load directory: ") + e.what();
        return false;
    }
}
```

## 使用示例

### 完整 Lua 示例脚本

```lua
-- 批量注册节点和加载行为树示例
print("========================================")
print("    Batch Registration Example")
print("========================================")
print("")

-- 1. 批量注册动作节点
print("1. Batch registering action nodes...")
bt.register_actions({
    LuaMoveTo = function(params)
        local entity_id = params.entity_id or ""
        local x = tonumber(params.target_x) or 0
        local y = tonumber(params.target_y) or 0
        print(string.format("   [LuaMoveTo] Moving %s to (%.1f, %.1f)", entity_id, x, y))
        sim.move_entity(entity_id, x, y, 0)
        return "SUCCESS"
    end,
    
    LuaPatrol = function(params)
        local entity_id = params.entity_id or ""
        local radius = tonumber(params.radius) or 5
        print(string.format("   [LuaPatrol] %s patrolling with radius %.1f", entity_id, radius))
        return "SUCCESS"
    end,
    
    LuaAttack = function(params)
        local attacker = params.attacker_id or ""
        local target = params.target_id or ""
        print(string.format("   [LuaAttack] %s attacking %s", attacker, target))
        return "SUCCESS"
    end,
    
    LuaFlee = function(params)
        local entity_id = params.entity_id or ""
        print(string.format("   [LuaFlee] %s fleeing", entity_id))
        return "SUCCESS"
    end
})
print("   OK: Actions registered")
print("")

-- 2. 批量注册条件节点
print("2. Batch registering condition nodes...")
bt.register_conditions({
    LuaHasTarget = function(params)
        local entity_id = params.entity_id or ""
        -- 简化示例：假设总是有条件
        return true
    end,
    
    LuaIsHealthy = function(params)
        local entity_id = params.entity_id or ""
        local min_health = tonumber(params.min_health) or 50
        -- 简化示例：假设总是健康
        return true
    end,
    
    LuaCanSeeEnemy = function(params)
        local entity_id = params.entity_id or ""
        local range = tonumber(params.range) or 10
        -- 简化示例
        return true
    end
})
print("   OK: Conditions registered")
print("")

-- 3. 批量加载行为树文件
print("3. Batch loading behavior tree files...")
bt.load_files({
    "bt_xml/lua_custom_nodes_example.xml",
    "bt_xml/lua_nodes_with_params.xml"
})
print("")

-- 4. 或者加载整个目录
print("4. Loading all behavior trees from directory...")
bt.load_directory("bt_xml/")
print("")

-- 5. 创建实体并执行行为树
print("5. Creating entities and executing behavior trees...")
local npc1 = sim.add_entity("npc", 0, 0, 0)
local npc2 = sim.add_entity("npc", 10, 10, 0)

-- 执行不同的行为树
local tree1 = bt.execute("LuaTestTree", npc1)
print("   LuaTestTree status: " .. bt.get_status(tree1))

local tree2 = bt.execute("LuaParamsTree", npc2)
print("   LuaParamsTree status: " .. bt.get_status(tree2))
print("")

-- 清理
sim.remove_entity(npc1)
sim.remove_entity(npc2)

print("========================================")
print("    Batch Registration Complete")
print("========================================")
```

## 实现文件清单

| 文件 | 修改内容 |
|------|----------|
| `LuaBehaviorTreeBridge.h` | 添加批量注册和加载方法的声明 |
| `LuaBehaviorTreeBridge.cpp` | 实现批量注册和加载方法，注册到 Lua API |
| `CMakeLists.txt` | 确保 C++17 标准（用于 std::filesystem）|

## 注意事项

1. **C++17 要求**：`std::filesystem` 需要 C++17，检查项目是否已启用
2. **错误处理**：批量操作会记录所有错误但继续处理，返回总体成功状态
3. **加载顺序**：目录加载会按文件名排序，确保依赖关系正确处理
4. **向后兼容**：现有单个注册/加载 API 保持不变

## 后续扩展

1. **节点库系统**：支持命名空间/前缀，避免节点名称冲突
2. **热重载**：支持运行时重新加载特定节点或行为树
3. **依赖管理**：支持行为树之间的依赖声明和自动加载
