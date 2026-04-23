# XML 预加载/懒加载切换开关实现计划

## 需求概述

添加一个开关，让用户可以选择 XML 行为树的加载策略：
- **预加载 (preload)**：启动时加载所有 XML 文件
- **懒加载 (lazy)**：使用时才加载 XML 文件（当前实现）

## 实现方案

### 方案一：全局配置开关（推荐）

#### 1. 添加配置枚举和成员变量

**LuaBehaviorTreeBridge.h 修改**：

```cpp
// XML 加载策略枚举
enum class XmlLoadingStrategy {
    LAZY,      // 懒加载：使用时才加载
    PRELOAD    // 预加载：启动时加载所有
};

class LuaBehaviorTreeBridge {
public:
    // ... 现有方法 ...
    
    // ==================== XML Loading Strategy API ====================
    
    // 设置 XML 加载策略
    void setXmlLoadingStrategy(XmlLoadingStrategy strategy);
    
    // 获取当前 XML 加载策略
    XmlLoadingStrategy getXmlLoadingStrategy() const;
    
    // 预加载所有已扫描的行为树定义
    bool preloadAllBehaviorTrees();
    
private:
    // ... 现有成员 ...
    
    // XML 加载策略
    XmlLoadingStrategy xmlLoadingStrategy_ = XmlLoadingStrategy::LAZY;
};
```

#### 2. 实现策略切换逻辑

**LuaBehaviorTreeBridge.cpp 修改**：

```cpp
void LuaBehaviorTreeBridge::setXmlLoadingStrategy(XmlLoadingStrategy strategy) {
    xmlLoadingStrategy_ = strategy;
    std::cout << "[LuaBehaviorTreeBridge] XML loading strategy set to: " 
              << (strategy == XmlLoadingStrategy::LAZY ? "LAZY" : "PRELOAD") 
              << std::endl;
}

XmlLoadingStrategy LuaBehaviorTreeBridge::getXmlLoadingStrategy() const {
    return xmlLoadingStrategy_;
}

bool LuaBehaviorTreeBridge::preloadAllBehaviorTrees() {
    if (treeDefinitionPaths_.empty()) {
        std::cout << "[LuaBehaviorTreeBridge] No tree definitions to preload" << std::endl;
        return true;
    }
    
    int successCount = 0;
    int totalCount = treeDefinitionPaths_.size();
    
    std::cout << "[LuaBehaviorTreeBridge] Preloading " << totalCount 
              << " behavior tree definitions..." << std::endl;
    
    for (const auto& pair : treeDefinitionPaths_) {
        const std::string& treeName = pair.first;
        const std::string& filePath = pair.second;
        
        if (loadedTreeDefinitions_.count(treeName) > 0) {
            // Already loaded
            successCount++;
            continue;
        }
        
        if (loadBehaviorTreeFromFile(filePath)) {
            loadedTreeDefinitions_.insert(treeName);
            successCount++;
            std::cout << "  [OK] Preloaded: " << treeName << std::endl;
        } else {
            std::cerr << "  [FAIL] Failed to preload: " << treeName 
                      << " (" << lastError_ << ")" << std::endl;
        }
    }
    
    std::cout << "[LuaBehaviorTreeBridge] Preloaded " << successCount 
              << "/" << totalCount << " behavior trees" << std::endl;
    
    return successCount == totalCount;
}
```

#### 3. 修改 executeBehaviorTree 支持策略切换

```cpp
std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }

    // 根据加载策略处理
    if (xmlLoadingStrategy_ == XmlLoadingStrategy::LAZY) {
        // 懒加载：按需加载
        if (!loadTreeDefinition(treeName)) {
            return "";
        }
    } else {
        // 预加载：假设已经加载，如果没有则尝试加载
        if (loadedTreeDefinitions_.count(treeName) == 0) {
            if (!tryLoadTreeDefinition(treeName)) {
                lastError_ = "Tree not found in preloaded definitions: " + treeName;
                return "";
            }
        }
    }
    
    // ... 原有执行逻辑 ...
}
```

#### 4. 注册 Lua API

```cpp
void LuaBehaviorTreeBridge::registerLuaAPI() {
    // ... 现有代码 ...
    
    // ==================== XML Loading Strategy API ====================
    
    // 设置加载策略 ("lazy" 或 "preload")
    btTable.set_function("set_loading_strategy", [this](const std::string& strategy) -> bool {
        if (strategy == "lazy") {
            setXmlLoadingStrategy(XmlLoadingStrategy::LAZY);
            return true;
        } else if (strategy == "preload") {
            setXmlLoadingStrategy(XmlLoadingStrategy::PRELOAD);
            return true;
        } else {
            lastError_ = "Invalid loading strategy: " + strategy + " (use 'lazy' or 'preload')";
            return false;
        }
    });
    
    // 获取当前加载策略
    btTable.set_function("get_loading_strategy", [this]() -> std::string {
        return getXmlLoadingStrategy() == XmlLoadingStrategy::LAZY ? "lazy" : "preload";
    });
    
    // 预加载所有已扫描的行为树
    btTable.set_function("preload_all_trees", [this]() -> bool {
        return preloadAllBehaviorTrees();
    });
}
```

### 方案二：使用方式示例

#### 方式一：懒加载（默认）

```lua
-- 仿真初始化
bt.load_registry("scripts/bt_nodes_registry.lua")  -- 注册 Lua 节点
bt.scan_trees("bt_xml/")                            -- 扫描 XML（不加载）
-- 策略默认为 lazy，无需设置

-- 运行时执行（懒加载）
bt.execute("PatrolTree", npc_id)    -- 首次：自动加载 XML
bt.execute("PatrolTree", npc_id2)   -- 后续：直接使用
```

#### 方式二：预加载

```lua
-- 仿真初始化
bt.load_registry("scripts/bt_nodes_registry.lua")  -- 注册 Lua 节点
bt.scan_trees("bt_xml/")                            -- 扫描 XML
bt.set_loading_strategy("preload")                  -- 设置为预加载模式
bt.preload_all_trees()                              -- 预加载所有 XML

-- 运行时执行（已预加载）
bt.execute("PatrolTree", npc_id)    -- 直接使用，无需加载
bt.execute("CombatTree", npc_id2)   -- 直接使用，无需加载
```

#### 方式三：混合策略

```lua
-- 仿真初始化
bt.load_registry("scripts/bt_nodes_registry.lua")
bt.scan_trees("bt_xml/")

-- 预加载核心行为树
bt.set_loading_strategy("preload")
bt.preload_tree("CoreAITree")
bt.preload_tree("CombatTree")

-- 其他使用懒加载
bt.set_loading_strategy("lazy")

-- 运行时
bt.execute("CoreAITree", npc_id)     -- 已预加载
bt.execute("RareEventTree", npc_id2) -- 懒加载（很少使用）
```

### 方案三：配置文件支持（可选扩展）

可以添加一个 Lua 配置文件来设置默认策略：

```lua
-- bt_config.lua
return {
    -- XML 加载策略: "lazy" 或 "preload"
    xml_loading_strategy = "lazy",
    
    -- 预加载时忽略的目录（懒加载）
    ondemand_directories = {
        "bt_xml/bosses/",
        "bt_xml/events/"
    },
    
    -- 总是预加载的行为树
    preload_trees = {
        "CoreAITree",
        "CombatTree"
    }
}
```

## 实现文件清单

| 文件 | 修改内容 |
|------|----------|
| `LuaBehaviorTreeBridge.h` | 添加 `XmlLoadingStrategy` 枚举，添加策略相关方法声明和成员变量 |
| `LuaBehaviorTreeBridge.cpp` | 实现策略切换方法，修改 `executeBehaviorTree` 支持策略，注册 Lua API |

## 详细代码变更

### LuaBehaviorTreeBridge.h

```cpp
#ifndef LUA_BEHAVIOR_TREE_BRIDGE_H
#define LUA_BEHAVIOR_TREE_BRIDGE_H

// ... 现有包含 ...

namespace scripting {

// XML 加载策略枚举
enum class XmlLoadingStrategy {
    LAZY,      // 懒加载：使用时才加载
    PRELOAD    // 预加载：启动时加载所有
};

// ... 现有类定义 ...

class LuaBehaviorTreeBridge {
public:
    // ... 现有方法 ...
    
    // ==================== XML Loading Strategy API ====================
    
    // 设置 XML 加载策略
    void setXmlLoadingStrategy(XmlLoadingStrategy strategy);
    
    // 获取当前 XML 加载策略
    XmlLoadingStrategy getXmlLoadingStrategy() const;
    
    // 预加载所有已扫描的行为树定义
    bool preloadAllBehaviorTrees();
    
private:
    // ... 现有成员 ...
    
    // XML 加载策略
    XmlLoadingStrategy xmlLoadingStrategy_ = XmlLoadingStrategy::LAZY;
};

} // namespace scripting

#endif // LUA_BEHAVIOR_TREE_BRIDGE_H
```

### LuaBehaviorTreeBridge.cpp 关键修改

```cpp
// 构造函数初始化策略
LuaBehaviorTreeBridge::LuaBehaviorTreeBridge(sol::state* luaState, BT::BehaviorTreeFactory* factory)
    : luaState_(luaState), factory_(factory), treeIdCounter_(0), initialized_(false),
      xmlLoadingStrategy_(XmlLoadingStrategy::LAZY) {
}

// 设置策略
void LuaBehaviorTreeBridge::setXmlLoadingStrategy(XmlLoadingStrategy strategy) {
    xmlLoadingStrategy_ = strategy;
    std::cout << "[LuaBehaviorTreeBridge] XML loading strategy: " 
              << (strategy == XmlLoadingStrategy::LAZY ? "LAZY" : "PRELOAD") 
              << std::endl;
}

// 获取策略
XmlLoadingStrategy LuaBehaviorTreeBridge::getXmlLoadingStrategy() const {
    return xmlLoadingStrategy_;
}

// 预加载所有行为树
bool LuaBehaviorTreeBridge::preloadAllBehaviorTrees() {
    if (treeDefinitionPaths_.empty()) {
        std::cout << "[LuaBehaviorTreeBridge] No trees to preload" << std::endl;
        return true;
    }
    
    int success = 0;
    for (const auto& pair : treeDefinitionPaths_) {
        if (loadTreeDefinition(pair.first)) {
            success++;
        }
    }
    
    std::cout << "[LuaBehaviorTreeBridge] Preloaded " << success << "/" 
              << treeDefinitionPaths_.size() << " trees" << std::endl;
    
    return success == treeDefinitionPaths_.size();
}
```

## 优势

1. **灵活性**：用户可以根据场景选择加载策略
2. **向后兼容**：默认为懒加载，不影响现有代码
3. **简单直观**：通过字符串 "lazy" 或 "preload" 设置
4. **可扩展**：未来可以支持更多策略（如混合策略）
