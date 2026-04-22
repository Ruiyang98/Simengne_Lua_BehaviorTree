# 全局外部 Tick 调度器重构 Spec

## Why
当前行为树调度器使用内部线程自动 tick，无法与仿真引擎的主循环同步，且不支持多实体行为树的统一调度管理。需要重构为外部驱动的全局单例调度器，使仿真引擎能够统一控制所有实体的行为树执行节奏。

## What Changes
- **重构** BehaviorTreeScheduler 为单例模式，全局唯一实例
- **移除** 内部调度线程，改为纯外部 tick 驱动
- **修改** 调度周期为固定 500ms，由外部统一调用
- **新增** 实体行为树注册机制，仿真实体创建时自动注册
- **新增** 轮询调度策略，依次调度不同实体的行为树
- **修改** 调度器只接收外部 tick，不主动执行
- **BREAKING** 移除 start()/stop() 线程相关方法
- **BREAKING** 移除手动模式相关接口

## Impact
- Affected specs: 行为树调度、实体管理、仿真循环
- Affected code:
  - include/behaviortree/BehaviorTreeScheduler.h（重构）
  - src/behaviortree/BehaviorTreeScheduler.cpp（重构）
  - include/behaviortree/BehaviorTreeExecutor.h（修改）
  - src/behaviortree/BehaviorTreeExecutor.cpp（修改）
  - include/simulation/Entity.h（可能需要修改）
  - src/simulation/Entity.cpp（可能需要修改）

## ADDED Requirements

### Requirement: 全局单例调度器
The system SHALL provide a global singleton BehaviorTreeScheduler that manages all entity behavior trees.

#### Scenario: 获取全局调度器实例
- **GIVEN** 系统任何地方需要访问调度器
- **WHEN** 调用 BehaviorTreeScheduler::getInstance()
- **THEN** 返回全局唯一的调度器实例

#### Scenario: 单例唯一性
- **GIVEN** 多次调用 getInstance()
- **WHEN** 比较返回的实例
- **THEN** 所有调用返回同一个实例

### Requirement: 外部 Tick 驱动
The system SHALL only execute behavior tree ticks when externally triggered, with no internal threading.

#### Scenario: 外部触发 tick
- **GIVEN** 调度器中有注册的行为树
- **WHEN** 外部每 500ms 调用 tickAll()
- **THEN** 依次执行所有活跃行为树的一次 tick

#### Scenario: 无内部线程
- **GIVEN** 调度器已初始化
- **WHEN** 检查调度器状态
- **THEN** 没有创建任何内部线程

#### Scenario: 仿真引擎驱动
- **GIVEN** 仿真引擎主循环运行中
- **WHEN** 每 500ms 到达调度时间点
- **THEN** 引擎调用调度器 tickAll() 方法

### Requirement: 实体行为树注册
The system SHALL allow entities to register their behavior trees with the global scheduler during creation.

#### Scenario: 实体创建时注册
- **GIVEN** 仿真实体被创建
- **WHEN** 实体初始化时调用 registerEntity()
- **THEN** 实体的行为树被添加到调度器管理列表

#### Scenario: 带行为树注册
- **GIVEN** 实体有特定的行为树配置
- **WHEN** 调用 registerEntityWithTree(entityId, treeName, blackboard)
- **THEN** 实体关联指定行为树并加入调度

#### Scenario: 实体销毁时注销
- **GIVEN** 实体已被注册到调度器
- **WHEN** 实体被销毁时调用 unregisterEntity()
- **THEN** 实体的行为树从调度器移除

### Requirement: 轮询调度策略
The system SHALL schedule behavior trees in a round-robin fashion during each tick cycle.

#### Scenario: 依次调度
- **GIVEN** 调度器中有多个实体的行为树
- **WHEN** 调用 tickAll()
- **THEN** 按照注册顺序依次执行每个行为树的一次 tick

#### Scenario: 固定时间间隔
- **GIVEN** 调度器配置为 500ms 间隔
- **WHEN** 外部每 500ms 调用 tickAll()
- **THEN** 所有行为树各执行一次 tick

#### Scenario: 调度周期控制
- **GIVEN** 需要查询调度周期
- **WHEN** 调用 getTickIntervalMs()
- **THEN** 返回 500（固定值）

### Requirement: 行为树状态管理
The system SHALL track and report the status of each registered behavior tree.

#### Scenario: 查询实体行为树状态
- **GIVEN** 实体已注册行为树
- **WHEN** 调用 getEntityStatus(entityId)
- **THEN** 返回该实体行为树的当前状态

#### Scenario: 获取所有注册实体
- **GIVEN** 调度器中有多个注册实体
- **WHEN** 调用 getRegisteredEntityIds()
- **THEN** 返回所有实体 ID 列表

#### Scenario: 暂停实体行为树
- **GIVEN** 实体行为树正在运行
- **WHEN** 调用 pauseEntity(entityId)
- **THEN** 该实体行为树暂停，不再被调度

#### Scenario: 恢复实体行为树
- **GIVEN** 实体行为树已暂停
- **WHEN** 调用 resumeEntity(entityId)
- **THEN** 该实体行为树恢复调度

## MODIFIED Requirements

### Requirement: BehaviorTreeScheduler 接口重构
The existing BehaviorTreeScheduler class SHALL be refactored to support the new architecture.

#### 移除的接口:
- start() - 不再使用内部线程
- stop() - 不再使用内部线程
- setManualMode() - 现在只有外部驱动模式
- isManualMode() - 现在只有外部驱动模式
- setTickInterval() - 固定 500ms

#### 修改的接口:
- scheduleTree() → registerEntity() / registerEntityWithTree()
- unscheduleTree() → unregisterEntity()
- getTreeStatus() → getEntityStatus()
- getScheduledTreeIds() → getRegisteredEntityIds()

#### 新增的接口:
- static BehaviorTreeScheduler& getInstance()
- void tickAll() - 外部调用，执行所有行为树的一次 tick
- int getTickIntervalMs() const - 返回固定 500ms

## REMOVED Requirements

### Requirement: 内部线程调度
**Reason**: 改为外部驱动模式，由仿真引擎统一控制
**Migration**: 使用 tickAll() 方法由外部调用

### Requirement: 可配置 tick 间隔
**Reason**: 固定 500ms 间隔简化设计
**Migration**: 间隔硬编码为 500ms，如需修改需改代码

### Requirement: 手动模式
**Reason**: 现在只有外部驱动一种模式
**Migration**: 无需迁移，直接使用 tickAll()
