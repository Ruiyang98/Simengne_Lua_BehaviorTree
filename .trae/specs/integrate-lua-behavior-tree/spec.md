# Lua 脚本与行为树集成规范

## Why

当前项目已经实现了独立的 Lua 脚本控制系统和 BehaviorTree.CPP 行为树系统，但两者之间缺乏有效的集成机制。用户需要通过 Lua 脚本来动态控制行为树的加载、执行和管理，实现更灵活的 AI 控制流程。

## What Changes

- 新增 Lua API 用于行为树控制（加载、执行、停止、状态查询）
- 新增 Lua API 用于黑板数据操作（读取、写入、监听）
- 新增 Lua API 用于自定义行为树节点注册
- 提供 Lua 脚本与行为树黑板的双向数据交换机制
- 创建示例 Lua 脚本展示集成用法

## Impact

- Affected specs: 仿真控制接口、Lua 绑定模块、行为树执行器
- Affected code: LuaSimBinding, BehaviorTreeExecutor, 新增 LuaBehaviorTreeBridge

## ADDED Requirements

### Requirement: Lua 行为树控制 API

系统 SHALL 提供 Lua API 用于控制行为树的完整生命周期。

#### Scenario: 从 Lua 加载行为树
- **GIVEN** Lua 环境已初始化且 XML 文件存在
- **WHEN** 调用 `bt.load_file(xml_path)`
- **THEN** 行为树被加载到工厂中，返回成功状态

#### Scenario: 从 Lua 执行行为树
- **GIVEN** 行为树已加载且实体存在
- **WHEN** 调用 `bt.execute(tree_name, entity_id, params)`
- **THEN** 行为树被执行，返回执行结果状态

#### Scenario: 查询行为树状态
- **GIVEN** 行为树正在执行
- **WHEN** 调用 `bt.get_status(tree_id)`
- **THEN** 返回当前行为树的执行状态

### Requirement: Lua 黑板数据操作 API

系统 SHALL 提供 Lua API 用于读写行为树黑板数据。

#### Scenario: 设置黑板变量
- **GIVEN** 行为树已创建
- **WHEN** 调用 `bt.set_blackboard(tree_id, key, value)`
- **THEN** 黑板中对应键被设置为指定值

#### Scenario: 获取黑板变量
- **GIVEN** 行为树正在执行且黑板有数据
- **WHEN** 调用 `bt.get_blackboard(tree_id, key)`
- **THEN** 返回黑板中对应键的值

#### Scenario: 黑板数据类型支持
- **GIVEN** 需要传递不同类型的数据
- **WHEN** 设置/获取数值、字符串、布尔值、表
- **THEN** 数据类型被正确保持和转换

### Requirement: Lua 自定义节点

系统 SHALL 支持从 Lua 脚本注册自定义行为树节点。

#### Scenario: 注册 Lua 动作节点
- **GIVEN** Lua 函数已定义
- **WHEN** 调用 `bt.register_action(name, lua_function)`
- **THEN** 该函数可作为行为树节点使用

#### Scenario: 注册 Lua 条件节点
- **GIVEN** Lua 函数返回布尔值
- **WHEN** 调用 `bt.register_condition(name, lua_function)`
- **THEN** 该函数可作为条件节点使用

### Requirement: 事件回调机制

系统 SHALL 支持 Lua 回调函数监听行为树事件。

#### Scenario: 节点状态变化回调
- **GIVEN** 行为树正在执行
- **WHEN** 节点状态发生变化
- **THEN** 注册的 Lua 回调函数被触发

#### Scenario: 行为树完成回调
- **GIVEN** 行为树执行完成
- **WHEN** 执行结束（成功/失败）
- **THEN** 完成回调被调用并传递结果

## MODIFIED Requirements

### Requirement: LuaSimBinding 扩展

**修改内容**: 添加行为树相关函数注册

```cpp
// 新增行为树 API 注册
void registerBehaviorTreeAPI();
```

### Requirement: BehaviorTreeExecutor 扩展

**修改内容**: 支持外部黑板操作和状态查询

```cpp
// 新增接口
BT::Blackboard::Ptr getBlackboard(const std::string& treeId);
BT::NodeStatus getTreeStatus(const std::string& treeId);
```
