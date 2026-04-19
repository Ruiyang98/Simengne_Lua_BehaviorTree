# 实体管理接口扩展 Spec

## Why
当前仿真系统仅支持仿真引擎的基本控制（启动、暂停、恢复、停止），但缺乏对仿真世界中实体的管理能力。为了实现通过Lua脚本控制仿真实体（如添加、删除、移动实体），需要扩展仿真接口以支持实体管理功能。

## What Changes
- **新增** SimControlInterface 实体管理接口：addEntity, removeEntity, moveEntity, getEntityPosition, getAllEntities
- **新增** MockSimController 实体管理实现：使用内存存储实体数据（ID、位置、类型）
- **新增** LuaSimBinding 实体API绑定：向Lua暴露实体管理函数
- **新增** 测试Lua脚本：entity_control_test.lua 演示实体添加、移动、删除操作

## Impact
- Affected specs: 仿真控制接口、Lua脚本绑定
- Affected code: 
  - include/simulation/SimControlInterface.h
  - include/simulation/MockSimController.h
  - src/simulation/MockSimController.cpp
  - src/scripting/LuaSimBinding.cpp
  - scripts/entity_control_test.lua (new)

## ADDED Requirements

### Requirement: 实体数据结构
The system SHALL provide an Entity structure containing:
- id: unique identifier (string)
- type: entity type (string)
- x, y, z: position coordinates (double)

### Requirement: 添加实体接口
The system SHALL provide addEntity(type, x, y, z) function.

#### Scenario: 成功添加实体
- **GIVEN** 仿真系统已初始化
- **WHEN** 调用 addEntity("npc", 0.0, 0.0, 0.0)
- **THEN** 返回新实体ID，实体被添加到系统中

### Requirement: 删除实体接口
The system SHALL provide removeEntity(entityId) function.

#### Scenario: 成功删除实体
- **GIVEN** 实体已存在于系统中
- **WHEN** 调用 removeEntity(entityId)
- **THEN** 返回 true，实体从系统中移除

#### Scenario: 删除不存在的实体
- **GIVEN** 实体ID不存在
- **WHEN** 调用 removeEntity("nonexistent")
- **THEN** 返回 false

### Requirement: 移动实体接口
The system SHALL provide moveEntity(entityId, x, y, z) function.

#### Scenario: 成功移动实体
- **GIVEN** 实体已存在于系统中
- **WHEN** 调用 moveEntity(entityId, 10.0, 20.0, 0.0)
- **THEN** 返回 true，实体位置更新

#### Scenario: 移动不存在的实体
- **GIVEN** 实体ID不存在
- **WHEN** 调用 moveEntity("nonexistent", 0.0, 0.0, 0.0)
- **THEN** 返回 false

### Requirement: 获取实体位置接口
The system SHALL provide getEntityPosition(entityId) function.

#### Scenario: 获取存在实体位置
- **GIVEN** 实体已存在于系统中，位置为 (10.0, 20.0, 0.0)
- **WHEN** 调用 getEntityPosition(entityId)
- **THEN** 返回包含 x, y, z 的表

#### Scenario: 获取不存在实体位置
- **GIVEN** 实体ID不存在
- **WHEN** 调用 getEntityPosition("nonexistent")
- **THEN** 返回 nil

### Requirement: 获取所有实体接口
The system SHALL provide getAllEntities() function.

#### Scenario: 获取实体列表
- **GIVEN** 系统中有多个实体
- **WHEN** 调用 getAllEntities()
- **THEN** 返回包含所有实体信息的表数组

### Requirement: Lua脚本控制实体移动
The system SHALL provide a test Lua script demonstrating entity control.

#### Scenario: 实体移动测试脚本
- **GIVEN** 仿真系统已启动
- **WHEN** 执行 entity_control_test.lua
- **THEN** 脚本应能：添加实体、移动实体到指定位置、验证位置、删除实体
