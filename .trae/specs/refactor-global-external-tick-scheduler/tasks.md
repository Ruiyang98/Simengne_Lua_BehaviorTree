# Tasks

- [x] Task 1: 重构 BehaviorTreeScheduler 为单例模式
  - [x] SubTask 1.1: 删除原有线程相关代码（start, stop, schedulerLoop, schedulerThread_）
  - [x] SubTask 1.2: 添加 getInstance() 静态方法实现单例
  - [x] SubTask 1.3: 私有化构造函数和析构函数
  - [x] SubTask 1.4: 删除手动模式相关代码和成员变量
  - [x] SubTask 1.5: 将 tickIntervalMs_ 改为常量 500

- [x] Task 2: 实现外部 Tick 驱动机制
  - [x] SubTask 2.1: 实现 tickAll() 方法，依次执行所有行为树的一次 tick
  - [x] SubTask 2.2: 修改 tickTree() 方法，移除时间间隔检查逻辑
  - [x] SubTask 2.3: 确保没有创建任何内部线程
  - [x] SubTask 2.4: 添加 getTickIntervalMs() 方法返回固定值 500

- [x] Task 3: 实现实体注册机制
  - [x] SubTask 3.1: 修改 scheduleTree() 为 registerEntityWithTree()
  - [x] SubTask 3.2: 实现 registerEntity() 方法，支持无行为树注册
  - [x] SubTask 3.3: 修改 unscheduleTree() 为 unregisterEntity()
  - [x] SubTask 3.4: 更新数据结构，使用 entityId 作为主键
  - [x] SubTask 3.5: 添加 pauseEntity() 和 resumeEntity() 方法

- [x] Task 4: 更新状态查询接口
  - [x] SubTask 4.1: 修改 getTreeStatus() 为 getEntityStatus()
  - [x] SubTask 4.2: 修改 getScheduledTreeIds() 为 getRegisteredEntityIds()
  - [x] SubTask 4.3: 修改 getTreeInfo() 为 getEntityInfo()
  - [x] SubTask 4.4: 更新 hasTree() 为 hasEntity()

- [x] Task 5: 更新 BehaviorTreeExecutor 集成
  - [x] SubTask 5.1: 修改 BehaviorTreeExecutor 使用全局调度器
  - [x] SubTask 5.2: 更新 executeAsync() 使用新的注册接口
  - [x] SubTask 5.3: 更新 stopAsync() 使用新的注销接口
  - [x] SubTask 5.4: 移除调度器生命周期管理代码

- [x] Task 6: 更新 LuaBehaviorTreeBridge API
  - [x] SubTask 6.1: 更新 Lua 绑定使用全局调度器
  - [x] SubTask 6.2: 修改 bt.execute_async() 使用新接口
  - [x] SubTask 6.3: 修改 bt.stop_async() 使用新接口
  - [x] SubTask 6.4: 修改 bt.get_async_status() 使用新接口

- [x] Task 7: 更新主程序命令行支持
  - [x] SubTask 7.1: 修改 "bt-async" 命令使用新接口
  - [x] SubTask 7.2: 修改 "bt-stop" 命令使用新接口
  - [x] SubTask 7.3: 修改 "bt-list" 命令使用新接口
  - [x] SubTask 7.4: 在主循环中添加 tickAll() 调用

- [x] Task 8: 编译验证
  - [x] SubTask 8.1: 更新 CMakeLists.txt（如需要）
  - [x] SubTask 8.2: 修复编译错误
  - [x] SubTask 8.3: 验证项目编译成功

# Task Dependencies
- Task 2 depends on Task 1
- Task 3 depends on Task 1
- Task 4 depends on Task 3
- Task 5 depends on Task 2, Task 3, Task 4
- Task 6 depends on Task 5
- Task 7 depends on Task 5
- Task 8 depends on Task 1-7
