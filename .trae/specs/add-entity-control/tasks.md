# Tasks

- [x] Task 1: 扩展 SimControlInterface 添加实体管理虚函数
  - [x] SubTask 1.1: 添加 Entity 结构体定义
  - [x] SubTask 1.2: 添加 addEntity, removeEntity, moveEntity, getEntityPosition, getAllEntities 纯虚函数
  - [x] SubTask 1.3: 添加 getEntityCount 辅助函数

- [x] Task 2: 在 MockSimController 中实现实体管理功能
  - [x] SubTask 2.1: 添加实体存储容器 (std::map<std::string, Entity>)
  - [x] SubTask 2.2: 实现 addEntity 方法
  - [x] SubTask 2.3: 实现 removeEntity 方法
  - [x] SubTask 2.4: 实现 moveEntity 方法
  - [x] SubTask 2.5: 实现 getEntityPosition 方法
  - [x] SubTask 2.6: 实现 getAllEntities 方法
  - [x] SubTask 2.7: 实现 getEntityCount 方法
  - [x] SubTask 2.8: 添加实体ID生成逻辑

- [x] Task 3: 在 LuaSimBinding 中绑定实体管理API
  - [x] SubTask 3.1: 注册 sim.add_entity(type, x, y, z) 函数
  - [x] SubTask 3.2: 注册 sim.remove_entity(entity_id) 函数
  - [x] SubTask 3.3: 注册 sim.move_entity(entity_id, x, y, z) 函数
  - [x] SubTask 3.4: 注册 sim.get_entity_position(entity_id) 函数
  - [x] SubTask 3.5: 注册 sim.get_all_entities() 函数
  - [x] SubTask 3.6: 注册 sim.get_entity_count() 函数

- [x] Task 4: 创建实体控制测试Lua脚本
  - [x] SubTask 4.1: 创建 scripts/entity_control_test.lua
  - [x] SubTask 4.2: 编写添加实体测试代码
  - [x] SubTask 4.3: 编写移动实体测试代码
  - [x] SubTask 4.4: 编写位置验证测试代码
  - [x] SubTask 4.5: 编写删除实体测试代码
  - [x] SubTask 4.6: 编写实体列表查询测试代码

# Task Dependencies
- Task 2 depends on Task 1
- Task 3 depends on Task 2
- Task 4 depends on Task 3
