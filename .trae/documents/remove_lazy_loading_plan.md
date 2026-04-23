# 移除懒加载，只保留预加载功能的修改计划

## 当前代码分析

当前代码混合了两种加载模式：
1. **预加载模式**: 通过 `preloadAllBehaviorTrees()` 或 `preloadBehaviorTreesFromDirectory()` 提前加载所有 XML
2. **懒加载模式**: 在 `executeBehaviorTree()` 中检查 `loadedTreeDefinitions_`，如果不存在则报错（但保留了懒加载的基础结构）

## 需要修改的文件

### 1. LuaBehaviorTreeBridge.h

**删除内容：**
- 成员变量 `treeDefinitionPaths_` (第158行) - 用于存储树名到文件路径的映射（懒加载用）
- 成员方法 `scanBehaviorTreeDefinitions()` (第117行) - 扫描目录但不加载（懒加载用）
- 注释 "// Tree name -> file path mapping for lazy loading" (第157行)
- 注释 "// Try to find and load tree definition" 和对应的 `tryLoadTreeDefinition()` 方法 (第163-164行)

**保留内容：**
- `loadedTreeDefinitions_` - 用于追踪已预加载的树定义
- `preloadAllBehaviorTrees()` - 预加载所有扫描到的树
- `preloadBehaviorTreesFromDirectory()` - 扫描并预加载目录中的所有 XML
- `loadNodesRegistry()` - 加载节点注册表

### 2. LuaBehaviorTreeBridge.cpp

**删除内容：**

1. **静态辅助函数** (第609-698行):
   - `scanXmlFileForTrees()` - 扫描 XML 文件中的 BehaviorTree 定义
   - `scanDirectoryRecursive()` - 递归扫描目录

2. **Lua API 注册** (第188-191行):
   - `bt.scan_trees` 函数绑定

3. **方法实现** (第700-728行):
   - `scanBehaviorTreeDefinitions()` 方法实现

4. **修改 `preloadAllBehaviorTrees()` 方法** (第730-765行):
   - 当前实现依赖 `treeDefinitionPaths_` 来知道要加载哪些文件
   - 修改为直接扫描并加载，不依赖预扫描的路径映射

5. **修改 `preloadBehaviorTreesFromDirectory()` 方法** (第767-775行):
   - 移除对 `scanBehaviorTreeDefinitions()` 的调用
   - 直接实现扫描+加载逻辑

6. **清理成员变量使用**:
   - 移除所有对 `treeDefinitionPaths_` 的引用

**保留/修改内容：**

1. **保留 `loadedTreeDefinitions_` 的使用**:
   - 在 `executeBehaviorTree()` 中检查树是否已预加载 (第243-248行)
   - 在 `preloadAllBehaviorTrees()` 中记录已加载的树

2. **简化 `preloadAllBehaviorTrees()`**:
   - 改为直接接收一个目录参数
   - 扫描并立即加载所有找到的 XML 文件
   - 不再需要先调用 scan 再调用 preload 的两步流程

3. **保留的 Lua API:**
   - `bt.load_registry()` - 加载节点注册表
   - `bt.preload_all_trees()` - 预加载所有行为树
   - `bt.preload_trees_from_dir(directory)` - 从目录预加载

## 修改后的 API 设计

```cpp
// 保留的 Lua API:
bt.load_registry(path)              -- 加载节点注册表
bt.preload_trees_from_dir(dir)      -- 扫描并预加载目录中的所有 XML
bt.preload_all_trees()              -- 预加载（可能需要重新设计，因为现在不需要先 scan）

// 执行时：
bt.execute(treeName, entityId, params)  -- 只能执行已预加载的树
```

## 具体修改步骤

### 步骤 1: 修改头文件 LuaBehaviorTreeBridge.h
- 删除 `treeDefinitionPaths_` 成员变量
- 删除 `scanBehaviorTreeDefinitions()` 方法声明
- 删除 `tryLoadTreeDefinition()` 方法声明（如果存在）
- 更新注释

### 步骤 2: 修改实现文件 LuaBehaviorTreeBridge.cpp
- 删除静态辅助函数 `scanXmlFileForTrees()` 和 `scanDirectoryRecursive()`
- 删除 `scanBehaviorTreeDefinitions()` 方法实现
- 删除 `bt.scan_trees` 的 Lua API 绑定
- 重写 `preloadAllBehaviorTrees()` 方法，使其直接扫描并加载
- 重写 `preloadBehaviorTreesFromDirectory()` 方法，合并扫描和加载逻辑
- 移除所有对 `treeDefinitionPaths_` 的引用

### 步骤 3: 简化 Lua API
- 移除 `bt.scan_trees` 函数
- 保留 `bt.preload_trees_from_dir(dir)` 作为主要的预加载入口
- `bt.preload_all_trees()` 可以保留但可能需要调整语义

## 预期结果

修改后，流程变为：
1. 调用 `bt.load_registry()` 加载节点注册表（可选）
2. 调用 `bt.preload_trees_from_dir("path/to/xmls")` 预加载所有 XML
3. 调用 `bt.execute(treeName, entityId, params)` 执行已预加载的树

不再有单独的 scan 步骤，也没有懒加载机制。如果尝试执行未预加载的树，会返回错误。
