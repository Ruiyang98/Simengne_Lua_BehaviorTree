# Tasks

- [x] Task 1: 集成 BehaviorTree.CPP 到项目构建系统
  - [x] SubTask 1.1: 修改根 CMakeLists.txt 添加 BehaviorTree.CPP 子目录
  - [x] SubTask 1.2: 创建 src/behaviortree/CMakeLists.txt 构建行为树模块
  - [x] SubTask 1.3: 配置 BehaviorTree.CPP 包含路径和链接

- [x] Task 2: 创建行为树节点基类和工具
  - [x] SubTask 2.1: 创建 include/behaviortree/SimControllerPtr.h 共享 SimControlInterface 指针
  - [x] SubTask 2.2: 创建 Blackboard 键名常量定义文件

- [x] Task 3: 实现 MoveToPoint 行为树节点
  - [x] SubTask 3.1: 创建 include/behaviortree/MoveToPoint.h 头文件
  - [x] SubTask 3.2: 实现 tick() 方法：从黑板读取 entity_id 和目标坐标
  - [x] SubTask 3.3: 调用 simController->moveEntity() 移动实体
  - [x] SubTask 3.4: 处理实体不存在的情况，返回 FAILURE

- [x] Task 4: 实现 FollowPath 行为树节点
  - [x] SubTask 4.1: 创建 include/behaviortree/FollowPath.h 头文件
  - [x] SubTask 4.2: 实现 tick() 方法：从黑板读取 entity_id 和 waypoints
  - [x] SubTask 4.3: 遍历路径点，依次调用 moveEntity 移动实体
  - [x] SubTask 4.4: 添加路径点间延迟（模拟移动时间）

- [x] Task 5: 实现 CheckEntityExists 条件节点
  - [x] SubTask 5.1: 创建 include/behaviortree/CheckEntityExists.h 头文件
  - [x] SubTask 5.2: 实现 tick() 方法：检查实体是否存在
  - [x] SubTask 5.3: 根据检查结果返回 SUCCESS 或 FAILURE

- [x] Task 6: 创建行为树 XML 定义文件
  - [x] SubTask 6.1: 创建 bt_xml/path_movement.xml 文件
  - [x] SubTask 6.2: 定义沿路径移动的行为树结构（Sequence + MoveToPoint）
  - [x] SubTask 6.3: 定义带条件检查的行为树（CheckEntityExists + FollowPath）

- [x] Task 7: 扩展主程序支持行为树模式
  - [x] SubTask 7.1: 修改 main.cpp 添加 --bt 命令行参数解析
  - [x] SubTask 7.2: 创建 BehaviorTreeExecutor 类管理行为树执行
  - [x] SubTask 7.3: 实现行为树工厂注册自定义节点
  - [x] SubTask 7.4: 实现从 XML 加载行为树并执行

- [x] Task 8: 创建路径移动测试示例
  - [x] SubTask 8.1: 创建 bt_xml/square_path.xml（正方形路径）
  - [x] SubTask 8.2: 创建 bt_xml/waypoint_patrol.xml（巡逻路径）
  - [x] SubTask 8.3: 添加示例实体创建和路径移动脚本

# Task Dependencies
- Task 2 depends on Task 1
- Task 3 depends on Task 2
- Task 4 depends on Task 2
- Task 5 depends on Task 2
- Task 6 depends on Task 3, Task 4, Task 5
- Task 7 depends on Task 3, Task 4, Task 5
- Task 8 depends on Task 6, Task 7
