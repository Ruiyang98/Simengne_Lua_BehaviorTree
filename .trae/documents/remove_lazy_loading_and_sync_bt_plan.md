# 删除懒加载XML和非异步行为树代码计划

## 分析结果

### 1. 懒加载XML相关代码

**C++ 代码需要删除：**

| 代码 | 位置 | 说明 |
|------|------|------|
| `loadNodesRegistry` | [LuaBehaviorTreeBridge.h:L165](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L165), [LuaBehaviorTreeBridge.cpp:L766-L790](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L766-L790) | 加载节点注册表脚本 |
| `scanBehaviorTreeDefinitions` | [LuaBehaviorTreeBridge.h:L168](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L168), [LuaBehaviorTreeBridge.cpp:L883-L911](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L883-L911) | 扫描目录构建树名到文件路径映射 |
| `preloadAllBehaviorTrees` | [LuaBehaviorTreeBridge.h:L171](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L171), [LuaBehaviorTreeBridge.cpp:L913-L948](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L913-L948) | 预加载所有扫描到的行为树 |
| `preloadBehaviorTreesFromDirectory` | [LuaBehaviorTreeBridge.h:L174](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L174), [LuaBehaviorTreeBridge.cpp#L950-L958](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L950-L958) | 扫描并预加载目录中的XML文件 |
| `scanXmlFileForTrees` | [LuaBehaviorTreeBridge.cpp:L793-L816](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L793-L816) | 辅助函数：扫描单个XML文件 |
| `scanDirectoryRecursive` | [LuaBehaviorTreeBridge.cpp:L819-L881](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L819-L881) | 辅助函数：递归扫描目录 |
| `treeDefinitionPaths_` | [LuaBehaviorTreeBridge.h:L209](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L209) | 树名到文件路径映射成员变量 |
| `loadedTreeDefinitions_` | [LuaBehaviorTreeBridge.h:L212](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L212) | 已加载树定义集合成员变量 |
| `tryLoadTreeDefinition` | [LuaBehaviorTreeBridge.h:L215](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L215) | 尝试查找并加载树定义（声明但未实现） |
| Lua API绑定 | [LuaBehaviorTreeBridge.cpp:L364-L384](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L364-L384) | `load_registry`, `scan_trees`, `preload_all_trees`, `preload_trees_from_dir` |

**Lua 脚本需要删除：**

| 文件 | 说明 |
|------|------|
| [scripts/load_lua_nodes_xml.lua](file:///d:/workspace/behaviortree/TestProject/scripts/load_lua_nodes_xml.lua) | 懒加载XML示例脚本 |
| [scripts/bt_nodes_registry.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_nodes_registry.lua) | 节点注册表脚本 |

---

### 2. 非异步行为树相关代码

**C++ 代码需要删除：**

| 代码 | 位置 | 说明 |
|------|------|------|
| `loadFromFile` | [BehaviorTreeExecutor.h:L37](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L37), [BehaviorTreeExecutor.cpp:L82-L97](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L82-L97) | 从XML文件加载行为树 |
| `loadFromText` | [BehaviorTreeExecutor.h:L40](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L40), [BehaviorTreeExecutor.cpp:L99-L113](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L99-L113) | 从XML文本加载行为树 |
| `execute` (同步执行) | [BehaviorTreeExecutor.h:L43-L44](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L43-L44), [BehaviorTreeExecutor.cpp:L115-L159](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L115-L159) | 同步执行行为树（tickRoot直到完成） |
| `getBlackboard` | [BehaviorTreeExecutor.h:L51](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L51), [BehaviorTreeExecutor.cpp:L161-L169](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L161-L169) | 获取树的blackboard |
| `getTreeStatus` | [BehaviorTreeExecutor.h:L54](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L54), [BehaviorTreeExecutor.cpp:L171-L179](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L171-L179) | 获取树状态 |
| `haltTree` | [BehaviorTreeExecutor.h:L57](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L57), [BehaviorTreeExecutor.cpp:L181-L193](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L181-L193) | 停止树 |
| `hasTree` | [BehaviorTreeExecutor.h:L60](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L60), [BehaviorTreeExecutor.cpp:L195-L198](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp#L195-L198) | 检查树是否存在 |
| `activeTrees_` 相关操作 | [BehaviorTreeExecutor.cpp](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/BehaviorTreeExecutor.cpp) | 同步执行相关的树管理 |
| `TreeExecutionInfo` | [BehaviorTreeExecutor.h:L16-L25](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/BehaviorTreeExecutor.h#L16-L25) | 如果仅用于同步执行则删除 |

**Lua Bridge 非异步代码需要删除：**

| 代码 | 位置 | 说明 |
|------|------|------|
| `loadBehaviorTreeFromFile` | [LuaBehaviorTreeBridge.h:L96](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L96), [LuaBehaviorTreeBridge.cpp:L387-L400](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L387-L400) | 从文件加载行为树 |
| `loadBehaviorTreeFromText` | [LuaBehaviorTreeBridge.h:L99](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L99), [LuaBehaviorTreeBridge.cpp:L402-L415](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L402-L415) | 从文本加载行为树 |
| `executeBehaviorTree` (非异步) | [LuaBehaviorTreeBridge.h:L102-L104](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L102-L104), [LuaBehaviorTreeBridge.cpp:L417-L494](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L417-L494) | 同步执行行为树 |
| `getTreeStatus` | [LuaBehaviorTreeBridge.h:L107](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L107), [LuaBehaviorTreeBridge.cpp:L496-L510](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L496-L510) | 获取树状态 |
| `stopBehaviorTree` | [LuaBehaviorTreeBridge.h:L110](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L110), [LuaBehaviorTreeBridge.cpp:L512-L525](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L512-L525) | 停止行为树 |
| `setBlackboardValue` | [LuaBehaviorTreeBridge.h:L113-L115](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L113-L115), [LuaBehaviorTreeBridge.cpp:L527-L562](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L527-L562) | 设置blackboard值 |
| `getBlackboardValue` | [LuaBehaviorTreeBridge.h:L118-L120](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L118-L120), [LuaBehaviorTreeBridge.cpp:L564-L607](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L564-L607) | 获取blackboard值 |
| `hasTree` | [LuaBehaviorTreeBridge.h:L137](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L137), [LuaBehaviorTreeBridge.cpp:L642-L645](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L642-L645) | 检查树是否存在 |
| Lua API绑定 | [LuaBehaviorTreeBridge.cpp:L256-L295](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L256-L295), [LuaBehaviorTreeBridge.cpp:L317-L325](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L317-L325) | `load_file`, `load_text`, `execute`, `get_status`, `stop`, `set_blackboard`, `get_blackboard`, `has_tree` |
| `TreeExecutionInfo` | [LuaBehaviorTreeBridge.h:L22-L32](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L22-L32) | 如果仅用于非异步执行则删除 |
| `activeTrees_` | [LuaBehaviorTreeBridge.h:L179](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L179) | 如果仅用于非异步执行则删除 |
| `treesMutex_` | [LuaBehaviorTreeBridge.h:L180](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L180) | 如果仅用于非异步执行则删除 |
| `treeIdCounter_` | [LuaBehaviorTreeBridge.h:L182](file:///d:/workspace/behaviortree/TestProject/include/scripting/LuaBehaviorTreeBridge.h#L182) | 如果仅用于非异步执行则删除 |
| `generateTreeId` | [LuaBehaviorTreeBridge.cpp:L647-L651](file:///d:/workspace/behaviortree/TestProject/src/scripting/LuaBehaviorTreeBridge.cpp#L647-L651) | 如果仅用于非异步执行则删除 |

**XML 文件需要删除（非异步的）：**

| 文件 | 说明 |
|------|------|
| [bt_xml/square_path.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/square_path.xml) | 使用同步MoveToPoint的XML |
| [bt_xml/path_movement.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/path_movement.xml) | 使用同步FollowPath的XML |
| [bt_xml/waypoint_patrol.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/waypoint_patrol.xml) | 使用同步FollowPath的XML |
| [bt_xml/square_path_composite.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/square_path_composite.xml) | 使用同步MoveToPoint的XML |

**保留的XML文件（异步的）：**
- [bt_xml/async_square_path.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/async_square_path.xml) - 使用AsyncMoveToPoint
- [bt_xml/lua_custom_nodes_example.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/lua_custom_nodes_example.xml) - Lua节点示例
- [bt_xml/lua_nodes_with_params.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/lua_nodes_with_params.xml) - Lua节点参数示例
- [bt_xml/lua_stateful_nodes.xml](file:///d:/workspace/behaviortree/TestProject/bt_xml/lua_stateful_nodes.xml) - Lua有状态节点示例

**Lua 脚本需要删除（非异步的）：**

| 文件 | 说明 |
|------|------|
| [scripts/bt_control_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_control_example.lua) | 基础BT控制示例（使用非异步API） |
| [scripts/bt_blackboard_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_blackboard_example.lua) | Blackboard操作示例（使用非异步API） |
| [scripts/bt_custom_node_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_custom_node_example.lua) | 自定义节点示例（使用非异步API） |
| [scripts/bt_custom_node_from_file.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_custom_node_from_file.lua) | 从文件加载节点示例 |
| [scripts/bt_advanced_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_advanced_example.lua) | 高级示例（使用非异步API） |
| [scripts/bt_lua_node_with_params.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_lua_node_with_params.lua) | Lua节点参数示例（使用非异步API） |

**保留的Lua脚本（异步的）：**
- [scripts/async_bt_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/async_bt_example.lua) - 异步BT示例
- [scripts/bt_stateful_action_example.lua](file:///d:/workspace/behaviortree/TestProject/scripts/bt_stateful_action_example.lua) - 有状态动作示例

---

### 3. 相关C++节点类需要删除

| 类 | 文件 | 说明 |
|----|------|------|
| `MoveToPoint` | [MoveToPoint.h](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/MoveToPoint.h), [MoveToPoint.cpp](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/MoveToPoint.cpp) | 同步移动到点节点 |
| `FollowPath` | [FollowPath.h](file:///d:/workspace/behaviortree/TestProject/include/behaviortree/FollowPath.h), [FollowPath.cpp](file:///d:/workspace/behaviortree/TestProject/src/behaviortree/FollowPath.cpp) | 同步跟随路径节点 |

**保留的节点类：**
- `AsyncMoveToPoint` - 异步移动到点节点
- `CheckEntityExists` - 检查实体存在节点
- `LuaActionNode` - Lua动作节点
- `LuaConditionNode` - Lua条件节点
- `LuaStatefulActionNode` - Lua有状态动作节点

---

## 清理步骤

### 阶段1：删除懒加载XML相关代码

1. **删除 LuaBehaviorTreeBridge 中的懒加载方法**
   - 删除 `loadNodesRegistry` 声明和实现
   - 删除 `scanBehaviorTreeDefinitions` 声明和实现
   - 删除 `preloadAllBehaviorTrees` 声明和实现
   - 删除 `preloadBehaviorTreesFromDirectory` 声明和实现
   - 删除辅助函数 `scanXmlFileForTrees` 和 `scanDirectoryRecursive`
   - 删除成员变量 `treeDefinitionPaths_` 和 `loadedTreeDefinitions_`
   - 删除 `tryLoadTreeDefinition` 声明

2. **删除 Lua API 绑定**
   - 删除 `load_registry`, `scan_trees`, `preload_all_trees`, `preload_trees_from_dir` 的Lua绑定

3. **删除 Lua 脚本**
   - 删除 `scripts/load_lua_nodes_xml.lua`
   - 删除 `scripts/bt_nodes_registry.lua`

### 阶段2：删除非异步行为树C++代码

1. **删除 BehaviorTreeExecutor 中的非异步方法**
   - 删除 `loadFromFile` 声明和实现
   - 删除 `loadFromText` 声明和实现
   - 删除 `execute` 声明和实现
   - 删除 `getBlackboard` 声明和实现
   - 删除 `getTreeStatus` 声明和实现
   - 删除 `haltTree` 声明和实现
   - 删除 `hasTree` 声明和实现
   - 删除 `TreeExecutionInfo` 结构体（如果仅用于同步执行）
   - 删除 `activeTrees_`, `treesMutex_`, `treeIdCounter_` 成员变量
   - 删除 `generateTreeId` 方法

2. **删除 LuaBehaviorTreeBridge 中的非异步方法**
   - 删除 `loadBehaviorTreeFromFile` 声明和实现
   - 删除 `loadBehaviorTreeFromText` 声明和实现
   - 删除 `executeBehaviorTree` 声明和实现
   - 删除 `getTreeStatus` 声明和实现
   - 删除 `stopBehaviorTree` 声明和实现
   - 删除 `setBlackboardValue` 声明和实现
   - 删除 `getBlackboardValue` 声明和实现
   - 删除 `hasTree` 声明和实现
   - 删除 `TreeExecutionInfo` 结构体（如果仅用于非异步执行）
   - 删除 `activeTrees_`, `treesMutex_`, `treeIdCounter_` 成员变量
   - 删除 `generateTreeId` 方法

3. **删除 Lua API 绑定**
   - 删除 `load_file`, `load_text`, `execute`, `get_status`, `stop`, `set_blackboard`, `get_blackboard`, `has_tree` 的Lua绑定

### 阶段3：删除非异步节点类

1. **删除 MoveToPoint 类**
   - 删除 `include/behaviortree/MoveToPoint.h`
   - 删除 `src/behaviortree/MoveToPoint.cpp`
   - 从 `BehaviorTreeExecutor::registerNodes()` 中移除注册

2. **删除 FollowPath 类**
   - 删除 `include/behaviortree/FollowPath.h`
   - 删除 `src/behaviortree/FollowPath.cpp`
   - 从 `BehaviorTreeExecutor::registerNodes()` 中移除注册

### 阶段4：删除XML和Lua文件

1. **删除非异步XML文件**
   - 删除 `bt_xml/square_path.xml`
   - 删除 `bt_xml/path_movement.xml`
   - 删除 `bt_xml/waypoint_patrol.xml`
   - 删除 `bt_xml/square_path_composite.xml`

2. **删除非异步Lua脚本**
   - 删除 `scripts/bt_control_example.lua`
   - 删除 `scripts/bt_blackboard_example.lua`
   - 删除 `scripts/bt_custom_node_example.lua`
   - 删除 `scripts/bt_custom_node_from_file.lua`
   - 删除 `scripts/bt_advanced_example.lua`
   - 删除 `scripts/bt_lua_node_with_params.lua`

### 阶段5：更新main.cpp和示例

1. **更新 main.cpp**
   - 删除 `executeBehaviorTree` 函数中的同步执行逻辑
   - 删除 `handleBtCommand` 中的同步BT命令处理
   - 保留异步BT命令（`bt-async`, `bt-stop`, `bt-list`, `bt-status`）

2. **更新或删除示例文件**
   - 更新 `examples/external_scheduler_example.cpp`（如果使用了非异步API）
   - 更新 `examples/manual_scheduler_example.cpp`
   - 更新 `examples/manual_tick_integration.cpp`
   - 更新 `examples/per_instance_frequency_example.cpp`

### 阶段6：验证编译

1. 重新生成CMake构建文件
2. 编译项目，确保无错误
3. 运行测试（如果有）

---

## 影响分析

- **破坏性变更**：这是一个破坏性变更，将删除所有同步/非异步行为树执行功能
- **仅保留异步执行**：之后只能通过全局调度器异步执行行为树
- **需要更新调用代码**：所有使用被删除API的代码需要更新为使用异步API
- **Lua脚本需要重写**：使用被删除Lua API的脚本需要重写

## 保留的功能

- 异步行为树执行（`executeAsync`）
- Lua自定义节点（`LuaActionNode`, `LuaConditionNode`, `LuaStatefulActionNode`）
- 全局调度器（`BehaviorTreeScheduler`）
- 异步节点（`AsyncMoveToPoint`, `CheckEntityExists`）
