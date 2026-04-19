# Tasks

- [x] Task 1: 创建 LuaBehaviorTreeBridge 桥接类
  - [x] SubTask 1.1: 创建 LuaBehaviorTreeBridge.h 头文件，定义桥接接口
  - [x] SubTask 1.2: 创建 LuaBehaviorTreeBridge.cpp 实现文件，实现核心桥接逻辑
  - [x] SubTask 1.3: 实现行为树生命周期管理（加载、执行、停止）

- [x] Task 2: 扩展 LuaSimBinding 注册行为树 API
  - [x] SubTask 2.1: 添加 `bt.load_file(xml_path)` API
  - [x] SubTask 2.2: 添加 `bt.execute(tree_name, entity_id, params)` API
  - [x] SubTask 2.3: 添加 `bt.get_status(tree_id)` API
  - [x] SubTask 2.4: 添加 `bt.stop(tree_id)` API

- [x] Task 3: 实现黑板数据操作 API
  - [x] SubTask 3.1: 添加 `bt.set_blackboard(tree_id, key, value)` API
  - [x] SubTask 3.2: 添加 `bt.get_blackboard(tree_id, key)` API
  - [x] SubTask 3.3: 实现 Lua 与 C++ 数据类型转换（支持 number, string, boolean, table）

- [x] Task 4: 支持 Lua 自定义行为树节点
  - [x] SubTask 4.1: 创建 LuaActionNode 类包装 Lua 函数为动作节点
  - [x] SubTask 4.2: 创建 LuaConditionNode 类包装 Lua 函数为条件节点
  - [x] SubTask 4.3: 添加 `bt.register_action(name, lua_function)` API
  - [x] SubTask 4.4: 添加 `bt.register_condition(name, lua_function)` API

- [x] Task 5: 扩展 BehaviorTreeExecutor 支持外部操作
  - [x] SubTask 5.1: 添加 `getBlackboard(tree_id)` 方法
  - [x] SubTask 5.2: 添加 `getTreeStatus(tree_id)` 方法
  - [x] SubTask 5.3: 添加 `haltTree(tree_id)` 方法

- [x] Task 6: 创建示例 Lua 脚本
  - [x] SubTask 6.1: 创建 `scripts/bt_control_example.lua` 基础控制示例
  - [x] SubTask 6.2: 创建 `scripts/bt_blackboard_example.lua` 黑板操作示例
  - [x] SubTask 6.3: 创建 `scripts/bt_custom_node_example.lua` 自定义节点示例
  - [x] SubTask 6.4: 创建 `scripts/bt_advanced_example.lua` 综合高级示例

- [x] Task 7: 更新 CMake 配置
  - [x] SubTask 7.1: 添加 LuaBehaviorTreeBridge 到编译目标
  - [x] SubTask 7.2: 验证所有新文件被正确编译链接

- [x] Task 8: 更新 main.cpp 集成
  - [x] SubTask 8.1: 初始化 Lua-BT 桥接器
  - [x] SubTask 8.2: 添加 lua-bt 命令显示示例列表

- [x] Task 9: 编译验证
  - [x] SubTask 9.1: 修复 BT::Expected 类型问题
  - [x] SubTask 9.2: 成功编译所有代码

# Task Dependencies

- Task 2 depends on Task 1
- Task 3 depends on Task 1
- Task 4 depends on Task 1
- Task 6 depends on Task 2, Task 3, Task 4
