# Lua 行为树节点重构计划

## 目标
将 Lua 行为树节点（LuaActionNode、LuaConditionNode、LuaStatefulActionNode）从各自文件中分离出来，统一放到独立的 LuaBehaviorTreeNodes.cpp 文件中，与 LuaBehaviorTreeBridge 逻辑解耦。

## 当前代码结构分析

### 当前文件分布
1. **LuaBehaviorTreeBridge.h** - 包含 LuaActionNode、LuaConditionNode 的声明和 Bridge 类
2. **LuaBehaviorTreeBridge.cpp** - 包含 LuaActionNode、LuaConditionNode 的实现和 Bridge 的实现
3. **LuaStatefulActionNode.h** - LuaStatefulActionNode 的声明
4. **LuaStatefulActionNode.cpp** - LuaStatefulActionNode 的实现

### 需要分离/合并的类
- `LuaActionNode` - 同步动作节点
- `LuaConditionNode` - 条件节点
- `LuaStatefulActionNode` - 有状态动作节点
- `TreeExecutionInfo` - 树执行信息结构体（可选，与 Bridge 紧密相关，保留在 Bridge 中）

## 重构步骤

### 步骤 1: 创建新的头文件 LuaBehaviorTreeNodes.h
- 从 LuaBehaviorTreeBridge.h 中移动 LuaActionNode 和 LuaConditionNode 的声明
- 从 LuaStatefulActionNode.h 中移动 LuaStatefulActionNode 的声明
- 保持原有的接口不变
- 添加必要的 include 和 forward declarations

### 步骤 2: 创建新的实现文件 LuaBehaviorTreeNodes.cpp
- 从 LuaBehaviorTreeBridge.cpp 中移动 LuaActionNode 和 LuaConditionNode 的实现
- 从 LuaStatefulActionNode.cpp 中移动 LuaStatefulActionNode 的实现
- 实现包括：
  - 静态成员初始化
  - 构造函数
  - tick() / onStart() / onRunning() / onHalted() 方法
  - collectInputPorts() 方法
  - setLuaFunction/clearLuaFunction 方法

### 步骤 3: 修改 LuaBehaviorTreeBridge.h
- 移除 LuaActionNode 和 LuaConditionNode 的声明
- 添加 `#include "scripting/LuaBehaviorTreeNodes.h"`
- 保留 TreeExecutionInfo（与 Bridge 紧密相关）
- 保留 LuaBehaviorTreeBridge 类的所有方法

### 步骤 4: 修改 LuaBehaviorTreeBridge.cpp
- 添加 `#include "scripting/LuaBehaviorTreeNodes.h"`
- 移除 LuaActionNode 和 LuaConditionNode 的实现代码
- 保留 Bridge 的实现代码

### 步骤 5: 删除旧文件
- 删除 `include/scripting/LuaStatefulActionNode.h`
- 删除 `src/scripting/LuaStatefulActionNode.cpp`

### 步骤 6: 更新 CMakeLists.txt
- 在 src/scripting/CMakeLists.txt 中：
  - 添加新的源文件 LuaBehaviorTreeNodes.cpp
  - 移除 LuaStatefulActionNode.cpp

## 文件变更清单

### 新建文件
1. `include/scripting/LuaBehaviorTreeNodes.h` - 所有节点类声明（LuaActionNode、LuaConditionNode、LuaStatefulActionNode）
2. `src/scripting/LuaBehaviorTreeNodes.cpp` - 所有节点类实现

### 修改文件
1. `include/scripting/LuaBehaviorTreeBridge.h` - 移除节点声明，添加新头文件引用
2. `src/scripting/LuaBehaviorTreeBridge.cpp` - 移除节点实现，添加新头文件引用
3. `src/scripting/CMakeLists.txt` - 添加新源文件，移除旧源文件

### 删除文件
1. `include/scripting/LuaStatefulActionNode.h`
2. `src/scripting/LuaStatefulActionNode.cpp`

## 依赖关系

### LuaBehaviorTreeNodes.h 依赖
- `<behaviortree_cpp_v3/action_node.h>`
- `<behaviortree_cpp_v3/condition_node.h>`
- `<behaviortree_cpp_v3/behavior_tree.h>`
- `<sol.hpp>`
- `<string>`
- `<unordered_map>`
- `<mutex>`
- `<tuple>`
- `<memory>`
- `<chrono>`
- `<iostream>`

### LuaBehaviorTreeBridge.h 新依赖
- `"scripting/LuaBehaviorTreeNodes.h"` (新增)
- 其他原有依赖保持不变

### LuaBehaviorTreeBridge.cpp 新依赖
- `"scripting/LuaBehaviorTreeNodes.h"` (新增)
- 移除 LuaActionNode/LuaConditionNode 的具体实现代码
- 移除对 LuaStatefulActionNode.h 的引用

## 注意事项
1. 保持所有公共接口不变，确保向后兼容
2. 静态成员变量的初始化需要移到新 cpp 文件中
3. 确保所有必要的头文件都被正确包含
4. 编译测试确保没有遗漏的依赖
5. LuaBehaviorTreeBridge 中引用 LuaStatefulActionNode 的地方需要更新头文件引用
