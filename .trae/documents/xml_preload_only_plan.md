# XML 预加载实现计划

## 需求概述

将 XML 行为树加载方式从懒加载改为**纯预加载模式**，即启动时加载所有 XML 文件。

## 实现方案

### 1. 修改执行逻辑

**LuaBehaviorTreeBridge.cpp 修改**：

```cpp
std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }

    // 预加载模式：假设已经加载，如果没有则报错
    if (loadedTreeDefinitions_.count(treeName) == 0) {
        lastError_ = "Tree definition not preloaded: " + treeName;
        return "";
    }
    
    // ... 原有执行逻辑 ...
}
```

### 2. 添加预加载 API

**LuaBehaviorTreeBridge.h 添加**：

```cpp
// 预加载所有已扫描的行为树定义
bool preloadAllBehaviorTrees();

// 预加载指定目录下的所有 XML 文件
bool preloadBehaviorTreesFromDirectory(const std::string& directory);
```

**LuaBehaviorTreeBridge.cpp 实现**：

```cpp
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

bool LuaBehaviorTreeBridge::preloadBehaviorTreesFromDirectory(const std::string& directory) {
    // 先扫描目录
    if (!scanBehaviorTreeDefinitions(directory)) {
        return false;
    }
    
    // 然后预加载所有
    return preloadAllBehaviorTrees();
}
```

### 3. 注册 Lua API

```cpp
void LuaBehaviorTreeBridge::registerLuaAPI() {
    // ... 现有代码 ...
    
    // 预加载所有已扫描的行为树
    btTable.set_function("preload_all_trees", [this]() -> bool {
        return preloadAllBehaviorTrees();
    });
    
    // 扫描并预加载目录
    btTable.set_function("preload_trees_from_dir", [this](const std::string& directory) -> bool {
        return preloadBehaviorTreesFromDirectory(directory);
    });
}
```

### 4. 使用方式

**标准使用流程**：

```lua
-- 仿真初始化
bt.load_registry("scripts/bt_nodes_registry.lua")  -- 注册 Lua 节点

-- 方式一：扫描并预加载
bt.scan_trees("bt_xml/")           -- 扫描目录
bt.preload_all_trees()             -- 预加载所有 XML

-- 方式二：直接预加载目录
bt.preload_trees_from_dir("bt_xml/")  -- 扫描+预加载一步完成

-- 运行时执行（必须已预加载）
bt.execute("PatrolTree", npc_id)   -- 直接使用
bt.execute("CombatTree", npc_id2)  -- 直接使用
```

### 5. 错误处理

如果执行未预加载的行为树：

```lua
bt.execute("UnknownTree", npc_id)  -- 报错：Tree definition not preloaded: UnknownTree
```

## 实现文件清单

| 文件 | 修改内容 |
|------|----------|
| `LuaBehaviorTreeBridge.h` | 添加 `preloadAllBehaviorTrees()` 和 `preloadBehaviorTreesFromDirectory()` 声明 |
| `LuaBehaviorTreeBridge.cpp` | 修改 `executeBehaviorTree` 为纯预加载模式，实现预加载方法，注册 Lua API |

## 详细代码变更

### LuaBehaviorTreeBridge.h

```cpp
class LuaBehaviorTreeBridge {
public:
    // ... 现有方法 ...
    
    // 预加载所有已扫描的行为树定义
    bool preloadAllBehaviorTrees();
    
    // 预加载指定目录下的所有 XML 文件（扫描+预加载）
    bool preloadBehaviorTreesFromDirectory(const std::string& directory);
    
private:
    // ... 现有成员 ...
};
```

### LuaBehaviorTreeBridge.cpp

```cpp
std::string LuaBehaviorTreeBridge::executeBehaviorTree(const std::string& treeName,
                                                        const std::string& entityId,
                                                        sol::optional<sol::table> params) {
    if (!factory_) {
        lastError_ = "BT factory is null";
        return "";
    }

    // 纯预加载模式：检查是否已预加载
    if (loadedTreeDefinitions_.count(treeName) == 0) {
        lastError_ = "Tree definition not preloaded: " + treeName + 
                     ". Call bt.preload_all_trees() or bt.preload_trees_from_dir() first.";
        std::cerr << "[LuaBehaviorTreeBridge] " << lastError_ << std::endl;
        return "";
    }
    
    // ... 原有执行逻辑 ...
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

bool LuaBehaviorTreeBridge::preloadBehaviorTreesFromDirectory(const std::string& directory) {
    // 先扫描目录
    if (!scanBehaviorTreeDefinitions(directory)) {
        return false;
    }
    
    // 然后预加载所有
    return preloadAllBehaviorTrees();
}
```

## 优势

1. **简单明确**：纯预加载，没有复杂的策略切换逻辑
2. **启动时发现问题**：所有 XML 错误在启动时暴露
3. **运行时无延迟**：执行时无需等待加载
4. **易于调试**：所有行为树定义在启动后都已知
