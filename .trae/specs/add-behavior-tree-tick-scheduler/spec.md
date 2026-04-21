# 行为树 Tick 调度器 Spec

## Why
当前行为树执行是一次性的（单次调用 tickRoot()），无法支持需要持续运行的行为（如平滑移动、持续巡逻、条件监控等）。为了实现实体在仿真引擎中持续使用行为树进行移动，需要一个调度机制来定期（如每帧或固定时间间隔）执行行为树的 tick。

## What Changes
- **新增** BehaviorTreeScheduler 类：管理多个行为树的定期 tick 执行
- **新增** 调度模式支持：
  - 固定时间间隔模式（如每 100ms tick 一次）
  - 手动触发模式（仿真引擎每帧调用）
- **新增** 异步执行支持：行为树在独立线程中运行，不阻塞主线程
- **新增** 回调机制：当行为树完成时通知调用方
- **修改** BehaviorTreeExecutor：添加与调度器的集成接口
- **新增** Lua API 扩展：支持从 Lua 启动/停止/查询调度中的行为树
- **新增** 命令行命令：支持交互式启动和停止调度

## Impact
- Affected specs: 行为树执行流程、Lua 集成接口
- Affected code:
  - include/behaviortree/BehaviorTreeScheduler.h（新建）
  - src/behaviortree/BehaviorTreeScheduler.cpp（新建）
  - include/behaviortree/BehaviorTreeExecutor.h（修改）
  - src/behaviortree/BehaviorTreeExecutor.cpp（修改）
  - include/scripting/LuaBehaviorTreeBridge.h（修改）
  - src/scripting/LuaBehaviorTreeBridge.cpp（修改）
  - src/main.cpp（扩展命令行支持）

## ADDED Requirements

### Requirement: BehaviorTreeScheduler 核心调度功能
The system SHALL provide a BehaviorTreeScheduler class that manages periodic ticking of behavior trees.

#### Scenario: 启动调度
- **GIVEN** 行为树已成功创建并存储
- **WHEN** 调用 scheduleTree() 方法
- **THEN** 行为树被添加到调度列表，开始定期 tick

#### Scenario: 固定间隔调度
- **GIVEN** 调度器配置为 100ms 间隔
- **WHEN** 调度器运行
- **THEN** 每 100ms 执行一次行为树的 tickRoot()

#### Scenario: 手动触发调度
- **GIVEN** 调度器配置为手动模式
- **WHEN** 仿真引擎每帧调用 update()
- **THEN** 所有活跃的行为树执行一次 tick

#### Scenario: 停止调度
- **GIVEN** 一个正在调度中的行为树
- **WHEN** 调用 unscheduleTree() 方法
- **THEN** 行为树停止 tick，从调度列表移除

#### Scenario: 行为树完成回调
- **GIVEN** 一个正在调度中的行为树
- **WHEN** 行为树返回 SUCCESS 或 FAILURE（非 RUNNING）
- **THEN** 触发完成回调函数，通知调用方

### Requirement: 异步执行支持
The system SHALL support running behavior trees in a separate thread to avoid blocking the main thread.

#### Scenario: 异步调度启动
- **GIVEN** 调度器配置为异步模式
- **WHEN** 启动调度
- **THEN** 调度器在独立线程中运行，主线程继续执行

#### Scenario: 线程安全访问
- **GIVEN** 多个线程同时访问调度器
- **WHEN** 添加/移除/查询行为树
- **THEN** 所有操作线程安全，不崩溃不数据竞争

### Requirement: BehaviorTreeExecutor 集成
The system SHALL integrate the scheduler with BehaviorTreeExecutor for seamless execution.

#### Scenario: 执行并调度
- **GIVEN** 调用 executeAsync() 方法
- **WHEN** 行为树创建成功
- **THEN** 自动添加到调度器开始定期 tick

#### Scenario: 获取调度状态
- **GIVEN** 一个正在运行的异步行为树
- **WHEN** 调用 getAsyncStatus() 方法
- **THEN** 返回当前状态（RUNNING/SUCCESS/FAILURE/IDLE）

### Requirement: Lua API 扩展
The system SHALL provide Lua APIs to control the behavior tree scheduler.

#### Scenario: Lua 启动调度
- **GIVEN** Lua 脚本调用 bt.execute_async()
- **WHEN** 参数有效
- **THEN** 行为树启动并在后台定期 tick

#### Scenario: Lua 停止调度
- **GIVEN** Lua 脚本调用 bt.stop_async()
- **WHEN** tree_id 有效
- **THEN** 对应的行为树停止调度

#### Scenario: Lua 查询状态
- **GIVEN** Lua 脚本调用 bt.get_async_status()
- **WHEN** tree_id 有效
- **THEN** 返回当前行为树状态字符串

#### Scenario: Lua 设置 tick 回调
- **GIVEN** Lua 脚本调用 bt.set_tick_callback()
- **WHEN** 每次行为树 tick 完成
- **THEN** 调用注册的 Lua 回调函数

### Requirement: 命令行交互支持
The system SHALL provide interactive commands to control the scheduler.

#### Scenario: 启动异步行为树
- **GIVEN** 用户输入 "bt-async <xml_file> [tree_name]"
- **WHEN** 文件存在且有效
- **THEN** 行为树在后台启动并定期 tick

#### Scenario: 停止异步行为树
- **GIVEN** 用户输入 "bt-stop <tree_id>"
- **WHEN** tree_id 存在
- **THEN** 对应的行为树停止调度

#### Scenario: 列出调度中的行为树
- **GIVEN** 用户输入 "bt-list"
- **WHEN** 有正在调度的行为树
- **THEN** 显示所有活跃行为树的 ID、名称和状态

### Requirement: 平滑移动支持
The system SHALL support smooth entity movement through periodic ticking.

#### Scenario: 持续移动节点
- **GIVEN** 行为树包含 AsyncMoveToPoint 节点
- **WHEN** 节点执行时
- **THEN** 每 tick 移动一小段距离，直到到达目标

#### Scenario: 移动完成检测
- **GIVEN** 实体正在移动中
- **WHEN** 到达目标位置
- **THEN** 节点返回 SUCCESS，行为树继续执行下一个节点
