# 行为树路径移动控制 Spec

## Why
当前系统已经支持通过Lua脚本控制实体管理（添加、删除、移动实体），但缺乏基于行为树的AI控制机制。为了实现更复杂的实体行为（如沿路径移动、条件判断、序列执行），需要集成BehaviorTree.CPP库，创建行为树节点来控制实体移动，特别是实现沿路径点移动的功能。

## What Changes
- **新增** BehaviorTree.CPP库集成：将3rdparty中的BehaviorTree.CPP添加到项目构建系统
- **新增** 行为树节点类：创建自定义行为树节点用于实体控制
  - MoveToPoint：移动到指定坐标点
  - FollowPath：沿路径点序列移动
  - CheckEntityExists：检查实体是否存在
- **新增** 行为树黑板数据：用于在节点间共享实体ID、路径点等数据
- **新增** 示例XML行为树定义：定义沿路径移动的行为树结构
- **新增** 主程序扩展：添加行为树执行模式，支持从XML加载和执行行为树
- **新增** 测试脚本：创建路径移动测试的行为树XML文件

## Impact
- Affected specs: 实体控制接口、主程序执行流程
- Affected code:
  - CMakeLists.txt（添加BehaviorTree.CPP依赖）
  - include/behaviortree/（新建行为树节点头文件）
  - src/behaviortree/（新建行为树节点实现）
  - src/main.cpp（扩展行为树执行模式）
  - bt_xml/（新建行为树XML定义文件）

## ADDED Requirements

### Requirement: BehaviorTree.CPP库集成
The system SHALL integrate BehaviorTree.CPP from 3rdparty into the build system.

#### Scenario: 编译时链接BehaviorTree.CPP
- **GIVEN** CMake配置正确
- **WHEN** 执行cmake构建
- **THEN** BehaviorTree.CPP库被正确编译并链接

### Requirement: MoveToPoint行为树节点
The system SHALL provide a MoveToPoint action node that moves an entity to a specified coordinate.

#### Scenario: 成功移动到目标点
- **GIVEN** 实体存在于系统中，目标坐标有效
- **WHEN** MoveToPoint节点执行
- **THEN** 实体移动到目标位置，返回SUCCESS

#### Scenario: 实体不存在
- **GIVEN** 指定的实体ID不存在
- **WHEN** MoveToPoint节点执行
- **THEN** 返回FAILURE

### Requirement: FollowPath行为树节点
The system SHALL provide a FollowPath action node that moves an entity along a sequence of waypoints.

#### Scenario: 成功沿路径移动
- **GIVEN** 实体存在，路径点列表有效（至少2个点）
- **WHEN** FollowPath节点执行
- **THEN** 实体依次移动到每个路径点，完成后返回SUCCESS

#### Scenario: 路径点不足
- **GIVEN** 路径点列表少于2个点
- **WHEN** FollowPath节点执行
- **THEN** 直接返回SUCCESS（无需移动）

### Requirement: CheckEntityExists条件节点
The system SHALL provide a CheckEntityExists condition node to verify entity existence.

#### Scenario: 实体存在检查
- **GIVEN** 实体存在于系统中
- **WHEN** CheckEntityExists节点执行
- **THEN** 返回SUCCESS

#### Scenario: 实体不存在检查
- **GIVEN** 实体不存在于系统中
- **WHEN** CheckEntityExists节点执行
- **THEN** 返回FAILURE

### Requirement: 行为树黑板共享数据
The system SHALL provide a Blackboard to share data between behavior tree nodes.

#### Scenario: 共享实体ID
- **GIVEN** 行为树初始化时设置了entity_id
- **WHEN** 各节点执行时读取entity_id
- **THEN** 所有节点访问到相同的实体ID

#### Scenario: 共享路径点
- **GIVEN** FollowPath节点执行前设置了waypoints
- **WHEN** FollowPath节点读取waypoints
- **THEN** 节点获取到正确的路径点列表

### Requirement: XML行为树定义
The system SHALL support loading behavior trees from XML files.

#### Scenario: 从XML加载行为树
- **GIVEN** 有效的行为树XML文件
- **WHEN** 调用加载函数
- **THEN** 行为树被正确解析并创建

#### Scenario: 执行XML定义的路径移动
- **GIVEN** XML定义了沿路径移动的行为树
- **WHEN** 执行该行为树
- **THEN** 实体按定义沿路径移动

### Requirement: 主程序行为树模式
The system SHALL support executing behavior trees in the main program.

#### Scenario: 命令行指定行为树文件
- **GIVEN** 程序启动时指定了--bt参数和XML文件路径
- **WHEN** 程序运行
- **THEN** 加载并执行指定的行为树

#### Scenario: 行为树执行完成
- **GIVEN** 行为树执行完成
- **THEN** 程序输出执行结果并正常退出
