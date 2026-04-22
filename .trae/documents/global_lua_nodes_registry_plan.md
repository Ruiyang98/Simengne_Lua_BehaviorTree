# 全局 Lua 节点注册脚本和 XML 懒加载实现计划

## 用户需求

1. **XML 采用懒加载**：行为树 XML 文件在使用时才加载，而不是预加载
2. **全局 Lua 脚本采用单个注册方式**：直观显示每个节点的注册逻辑，便于阅读和维护

## 方案设计

### 1. 全局 Lua 节点注册脚本

创建一个全局脚本 `scripts/bt_nodes_registry.lua`，使用**单个注册方式**，直观展示每个节点的逻辑：

```lua
-- bt_nodes_registry.lua
-- 全局行为树节点注册中心
-- 采用单个注册方式，直观显示每个节点的逻辑

print("[BT Nodes Registry] Starting node registration...")

-- ============================================
-- 移动相关动作节点
-- ============================================

bt.register_action("LuaMoveTo", function(params)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local speed = tonumber(params.speed) or 1.0
    
    print(string.format("[LuaMoveTo] Moving entity '%s' to (%.1f, %.1f) at speed %.1f",
                        entity_id, target_x, target_y, speed))
    
    sim.move_entity(entity_id, target_x, target_y, 0)
    return "SUCCESS"
end)
print("  [OK] LuaMoveTo registered")

bt.register_action("LuaPatrol", function(params)
    local entity_id = params.entity_id or ""
    local radius = tonumber(params.radius) or 5.0
    local num_points = tonumber(params.num_points) or 4
    
    print(string.format("[LuaPatrol] Entity '%s' patrolling with radius %.1f, %d points",
                        entity_id, radius, num_points))
    
    local pos = sim.get_entity_position(entity_id)
    if pos then
        for i = 1, num_points do
            local angle = (2 * math.pi * (i - 1)) / num_points
            local new_x = pos.x + radius * math.cos(angle)
            local new_y = pos.y + radius * math.sin(angle)
            sim.move_entity(entity_id, new_x, new_y, pos.z)
        end
    end
    
    return "SUCCESS"
end)
print("  [OK] LuaPatrol registered")

bt.register_action("LuaFlee", function(params)
    local entity_id = params.entity_id or ""
    local distance = tonumber(params.distance) or 10.0
    
    print(string.format("[LuaFlee] Entity '%s' fleeing %d units", entity_id, distance))
    return "SUCCESS"
end)
print("  [OK] LuaFlee registered")

-- ============================================
-- 战斗相关动作节点
-- ============================================

bt.register_action("LuaAttack", function(params)
    local attacker_id = params.attacker_id or ""
    local target_id = params.target_id or ""
    local damage = tonumber(params.damage) or 10
    
    print(string.format("[LuaAttack] '%s' attacks '%s' for %d damage",
                        attacker_id, target_id, damage))
    return "SUCCESS"
end)
print("  [OK] LuaAttack registered")

bt.register_action("LuaDefend", function(params)
    local entity_id = params.entity_id or ""
    local duration = tonumber(params.duration) or 2.0
    
    print(string.format("[LuaDefend] Entity '%s' defending for %.1f seconds",
                        entity_id, duration))
    return "SUCCESS"
end)
print("  [OK] LuaDefend registered")

-- ============================================
-- 交互相关动作节点
-- ============================================

bt.register_action("LuaWait", function(params)
    local duration = tonumber(params.duration) or 1.0
    print(string.format("[LuaWait] Waiting for %.1f seconds", duration))
    return "SUCCESS"
end)
print("  [OK] LuaWait registered")

bt.register_action("LuaInteract", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""
    local action = params.action or "use"
    
    print(string.format("[LuaInteract] '%s' %s '%s'", entity_id, action, target_id))
    return "SUCCESS"
end)
print("  [OK] LuaInteract registered")

-- ============================================
-- 健康状态条件节点
-- ============================================

bt.register_condition("LuaIsHealthy", function(params)
    local entity_id = params.entity_id or ""
    local min_health = tonumber(params.min_health) or 50
    
    -- 简化示例：假设总是健康
    print(string.format("[LuaIsHealthy] Checking '%s' health >= %d: true",
                        entity_id, min_health))
    return true
end)
print("  [OK] LuaIsHealthy registered")

bt.register_condition("LuaIsLowHealth", function(params)
    local entity_id = params.entity_id or ""
    local threshold = tonumber(params.threshold) or 30
    
    -- 简化示例：假设从不低血量
    print(string.format("[LuaIsLowHealth] Checking '%s' health < %d: false",
                        entity_id, threshold))
    return false
end)
print("  [OK] LuaIsLowHealth registered")

-- ============================================
-- 目标检测条件节点
-- ============================================

bt.register_condition("LuaHasTarget", function(params)
    local entity_id = params.entity_id or ""
    local range = tonumber(params.range) or 10
    
    print(string.format("[LuaHasTarget] '%s' checking for target within range %d: true",
                        entity_id, range))
    return true
end)
print("  [OK] LuaHasTarget registered")

bt.register_condition("LuaCanSeeEnemy", function(params)
    local entity_id = params.entity_id or ""
    local range = tonumber(params.range) or 15
    local angle = tonumber(params.angle) or 120
    
    print(string.format("[LuaCanSeeEnemy] '%s' checking vision (range=%d, angle=%d): true",
                        entity_id, range, angle))
    return true
end)
print("  [OK] LuaCanSeeEnemy registered")

-- ============================================
-- 距离检查条件节点
-- ============================================

bt.register_condition("LuaIsInRange", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""
    local range = tonumber(params.range) or 5
    
    print(string.format("[LuaIsInRange] '%s' distance to '%s' <= %d: true",
                        entity_id, target_id, range))
    return true
end)
print("  [OK] LuaIsInRange registered")

-- ============================================
-- 实体存在条件节点
-- ============================================

bt.register_condition("LuaHasEntities", function(params)
    local count = sim.get_entity_count()
    print(string.format("[LuaHasEntities] Entity count: %d", count))
    return count > 0
end)
print("  [OK] LuaHasEntities registered")

bt.register_condition("LuaEntityExists", function(params)
    local entity_id = params.entity_id or ""
    local pos = sim.get_entity_position(entity_id)
    local exists = pos ~= nil
    print(string.format("[LuaEntityExists] '%s' exists: %s", entity_id, tostring(exists)))
    return exists
end)
print("  [OK] LuaEntityExists registered")

print("[BT Nodes Registry] All nodes registered successfully!")
print("")
```

### 2. XML 懒加载机制

实现懒加载，只在执行行为树时才加载对应的 XML 文件：

#### C++ 实现

```cpp
// LuaBehaviorTreeBridge.h
class LuaBehaviorTreeBridge {
public:
    // ... 现有方法 ...
    
    // 加载全局节点注册脚本
    bool loadNodesRegistry(const std::string& registryPath = "scripts/bt_nodes_registry.lua");
    
    // 扫描目录建立行为树名称到文件路径的映射
    bool scanBehaviorTreeDefinitions(const std::string& directory);
    
    // 检查行为树定义是否可用（已加载或可加载）
    bool isTreeDefinitionAvailable(const std::string& treeName);
    
    // 懒加载行为树定义
    bool loadTreeDefinition(const std::string& treeName);
    
private:
    // 行为树名称到文件路径的映射
    std::unordered_map<std::string, std::string> treeDefinitionPaths_;
    
    // 已加载的行为树定义
    std::unordered_set<std::string> loadedTreeDefinitions_;
    
    // 尝试查找并加载行为树定义
    bool tryLoadTreeDefinition(const std::string& treeName);
};
```

```cpp
// LuaBehaviorTreeBridge.cpp

// 扫描目录，建立行为树名称到文件路径的映射
bool LuaBehaviorTreeBridge::scanBehaviorTreeDefinitions(const std::string& directory) {
    try {
        if (!std::filesystem::exists(directory)) {
            lastError_ = "Directory does not exist: " + directory;
            return false;
        }
        
        int count = 0;
        
        // 递归遍历目录
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                std::string filePath = entry.path().string();
                
                // 解析 XML 获取 BehaviorTree ID
                std::ifstream file(filePath);
                if (!file.is_open()) continue;
                
                std::string line;
                while (std::getline(file, line)) {
                    // 简单解析：查找 <BehaviorTree ID="xxx">
                    size_t pos = line.find("<BehaviorTree");
                    if (pos != std::string::npos) {
                        size_t idPos = line.find("ID=\"", pos);
                        if (idPos != std::string::npos) {
                            size_t start = idPos + 4;
                            size_t end = line.find("\"", start);
                            if (end != std::string::npos) {
                                std::string treeId = line.substr(start, end - start);
                                treeDefinitionPaths_[treeId] = filePath;
                                count++;
                            }
                        }
                    }
                }
            }
        }
        
        std::cout << "[LuaBehaviorTreeBridge] Scanned " << count 
                  << " behavior tree definitions from " << directory << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = std::string("Failed to scan directory: ") + e.what();
        return false;
    }
}

// 检查行为树定义是否可用
bool LuaBehaviorTreeBridge::isTreeDefinitionAvailable(const std::string& treeName) {
    // 已加载
    if (loadedTreeDefinitions_.count(treeName) > 0) {
        return true;
    }
    // 可以加载
    return treeDefinitionPaths_.count(treeName) > 0;
}

// 懒加载行为树定义
bool LuaBehaviorTreeBridge::loadTreeDefinition(const std::string& treeName) {
    // 已加载，直接返回
    if (loadedTreeDefinitions_.count(treeName) > 0) {
        return true;
    }
    
    return tryLoadTreeDefinition(treeName);
}

// 尝试查找并加载行为树定义
bool LuaBehaviorTreeBridge::tryLoadTreeDefinition(const std::string& treeName) {
    // 查找文件路径
    auto it = treeDefinitionPaths_.find(treeName);
    if (it != treeDefinitionPaths_.end()) {
        // 找到映射，加载文件
        if (loadBehaviorTreeFromFile(it->second)) {
            loadedTreeDefinitions_.insert(treeName);
            std::cout << "[LuaBehaviorTreeBridge] Lazy loaded: " << treeName 
                      << " from " << it->second << std::endl;
            return true;
        }
        return false;
    }
    
    // 没有找到映射，尝试在默认目录中查找
    std::string defaultPath = "bt_xml/" + treeName + ".xml";
    if (std::filesystem::exists(defaultPath)) {
        if (loadBehaviorTreeFromFile(defaultPath)) {
            loadedTreeDefinitions_.insert(treeName);
            treeDefinitionPaths_[treeName] = defaultPath;
            std::cout << "[LuaBehaviorTreeBridge] Lazy loaded: " << treeName 
                      << " from " << defaultPath << std::endl;
            return true;
        }
    }
    
    lastError_ = "Tree definition not found: " + treeName;
    return false;
}

// 修改 executeBehaviorTree，添加懒加载
std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    // 懒加载行为树定义
    if (!loadTreeDefinition(treeName)) {
        // 加载失败，返回空
        return "";
    }
    
    // ... 原有执行逻辑 ...
}
```

#### Lua API

```cpp
void LuaBehaviorTreeBridge::registerLuaAPI() {
    // ... 现有代码 ...
    
    // 加载全局节点注册脚本
    btTable.set_function("load_registry", [this](sol::optional<std::string> registryPath) -> bool {
        return loadNodesRegistry(registryPath.value_or("scripts/bt_nodes_registry.lua"));
    });
    
    // 扫描行为树定义
    btTable.set_function("scan_trees", [this](const std::string& directory) -> bool {
        return scanBehaviorTreeDefinitions(directory);
    });
    
    // 检查行为树是否可用
    btTable.set_function("is_tree_available", [this](const std::string& treeName) -> bool {
        return isTreeDefinitionAvailable(treeName);
    });
    
    // 预加载特定行为树（可选，用于需要提前加载的场景）
    btTable.set_function("preload_tree", [this](const std::string& treeName) -> bool {
        return loadTreeDefinition(treeName);
    });
}
```

### 3. 使用示例

#### 仿真初始化脚本

```lua
-- simulation_init.lua
-- 仿真初始化

print("========================================")
print("    Simulation Initialization")
print("========================================")

-- 1. 加载全局节点注册表（单个注册方式，直观显示逻辑）
print("1. Loading nodes registry...")
if bt.load_registry("scripts/bt_nodes_registry.lua") then
    print("   OK: All nodes registered")
else
    print("   ERROR: " .. bt.get_last_error())
    return
end
print("")

-- 2. 扫描行为树定义（建立名称到文件的映射，但不加载）
print("2. Scanning behavior tree definitions...")
if bt.scan_trees("bt_xml/") then
    print("   OK: Tree definitions scanned")
else
    print("   WARNING: " .. bt.get_last_error())
end
print("")

-- 3. 创建实体
print("3. Creating entities...")
local npc1 = sim.add_entity("npc", 0, 0, 0)
local npc2 = sim.add_entity("npc", 10, 10, 0)
print("   Created NPCs: " .. npc1 .. ", " .. npc2)
print("")

-- 4. 检查行为树是否可用
print("4. Checking available behavior trees...")
print("   PatrolTree available: " .. tostring(bt.is_tree_available("PatrolTree")))
print("   CombatTree available: " .. tostring(bt.is_tree_available("CombatTree")))
print("")

-- 5. 执行行为树（懒加载：首次执行时自动加载 XML）
print("5. Executing behavior trees (lazy loading)...")

-- 第一次执行 PatrolTree：会自动加载 bt_xml/PatrolTree.xml
local tree1 = bt.execute("PatrolTree", npc1)
print("   PatrolTree executed: " .. tree1)

-- 第二次执行 PatrolTree：已加载，直接使用
local tree2 = bt.execute("PatrolTree", npc2)
print("   PatrolTree executed: " .. tree2)
print("")

print("========================================")
print("    Simulation Ready")
print("========================================")
```

#### 全局节点注册脚本示例

```lua
-- bt_nodes_registry.lua
-- 直观显示每个节点的注册逻辑

print("[BT Nodes Registry] Registering nodes...")

-- 移动动作
bt.register_action("LuaMoveTo", function(params)
    local entity_id = params.entity_id or ""
    local x = tonumber(params.target_x) or 0
    local y = tonumber(params.target_y) or 0
    sim.move_entity(entity_id, x, y, 0)
    return "SUCCESS"
end)
print("  [OK] LuaMoveTo")

-- 巡逻动作
bt.register_action("LuaPatrol", function(params)
    local entity_id = params.entity_id or ""
    local radius = tonumber(params.radius) or 5
    -- 巡逻逻辑...
    return "SUCCESS"
end)
print("  [OK] LuaPatrol")

-- 健康检查条件
bt.register_condition("LuaIsHealthy", function(params)
    local entity_id = params.entity_id or ""
    -- 健康检查逻辑...
    return true
end)
print("  [OK] LuaIsHealthy")

print("[BT Nodes Registry] Done!")
```

## 实现文件清单

| 文件 | 修改内容 |
|------|----------|
| `scripts/bt_nodes_registry.lua` | 创建全局节点注册脚本，使用单个注册方式 |
| `LuaBehaviorTreeBridge.h` | 添加懒加载相关方法声明 |
| `LuaBehaviorTreeBridge.cpp` | 实现懒加载逻辑，修改 execute 方法支持懒加载 |

## 懒加载流程

```
用户调用 bt.execute("PatrolTree", entity_id)
         |
         v
[检查] 是否已加载 PatrolTree 定义？
    |-- 是 --> 直接创建并执行行为树
    |
    |-- 否 --> [查找] treeName -> filePath 映射
                  |
                  |-- 找到 --> 加载 XML 文件
                  |             |
                  |             v
                  |         注册到 BT Factory
                  |             |
                  |             v
                  |         标记为已加载
                  |             |
                  |             v
                  |         创建并执行行为树
                  |
                  |-- 未找到 --> 返回错误
```

## 优势

1. **直观显示逻辑**：每个节点的注册代码清晰可见，便于维护
2. **启动速度快**：XML 文件按需加载，减少启动时间
3. **内存占用小**：只加载实际使用的行为树定义
4. **灵活**：支持预加载特定行为树（如果需要）
