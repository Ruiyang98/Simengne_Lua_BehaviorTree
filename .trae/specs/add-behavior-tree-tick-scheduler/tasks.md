# Tasks

- [x] Task 1: 创建 BehaviorTreeScheduler 核心类
  - [x] SubTask 1.1: 创建 BehaviorTreeScheduler.h 头文件，定义调度器接口
  - [x] SubTask 1.2: 实现 scheduleTree() 方法，添加行为树到调度列表
  - [x] SubTask 1.3: 实现 unscheduleTree() 方法，从调度列表移除
  - [x] SubTask 1.4: 实现 update() 方法，执行一次所有活跃行为树的 tick
  - [x] SubTask 1.5: 添加线程安全机制（mutex 保护共享数据）

- [x] Task 2: 实现异步调度线程
  - [x] SubTask 2.1: 实现 start() 方法，启动调度线程
  - [x] SubTask 2.2: 实现 stop() 方法，停止调度线程
  - [x] SubTask 2.3: 实现调度循环，按固定间隔调用 update()
  - [x] SubTask 2.4: 添加完成回调机制

- [x] Task 3: 扩展 BehaviorTreeExecutor 支持异步执行
  - [x] SubTask 3.1: 修改 BehaviorTreeExecutor.h，添加调度器成员
  - [x] SubTask 3.2: 实现 executeAsync() 方法，创建并调度行为树
  - [x] SubTask 3.3: 实现 stopAsync() 方法，停止指定的异步行为树
  - [x] SubTask 3.4: 实现 getAsyncStatus() 方法，查询异步行为树状态
  - [x] SubTask 3.5: 实现 listAsyncTrees() 方法，列出所有异步行为树

- [x] Task 4: 创建 AsyncMoveToPoint 异步移动节点
  - [x] SubTask 4.1: 创建 AsyncMoveToPoint.h 头文件
  - [x] SubTask 4.2: 实现 tick() 方法，每 tick 移动一小段距离
  - [x] SubTask 4.3: 实现距离检测，到达目标返回 SUCCESS
  - [x] SubTask 4.4: 在 BehaviorTreeExecutor 中注册 AsyncMoveToPoint 节点

- [x] Task 5: 扩展 LuaBehaviorTreeBridge API
  - [x] SubTask 5.1: 添加 bt.execute_async() Lua 函数
  - [x] SubTask 5.2: 添加 bt.stop_async() Lua 函数
  - [x] SubTask 5.3: 添加 bt.get_async_status() Lua 函数
  - [x] SubTask 5.4: 添加 bt.list_async_trees() Lua 函数
  - [x] SubTask 5.5: 添加 bt.set_tick_callback() Lua 函数

- [x] Task 6: 扩展主程序命令行支持
  - [x] SubTask 6.1: 添加 "bt-async" 命令，启动异步行为树
  - [x] SubTask 6.2: 添加 "bt-stop" 命令，停止异步行为树
  - [x] SubTask 6.3: 添加 "bt-list" 命令，列出调度中的行为树
  - [x] SubTask 6.4: 添加 "bt-status" 命令，查询指定行为树状态

- [x] Task 7: 创建异步移动示例
  - [x] SubTask 7.1: 创建 bt_xml/async_square_path.xml 示例文件
  - [x] SubTask 7.2: 创建 scripts/async_bt_example.lua 示例脚本
  - [x] SubTask 7.3: 测试异步移动功能

- [x] Task 8: 更新 CMakeLists.txt 并编译验证
  - [x] SubTask 8.1: 更新 src/behaviortree/CMakeLists.txt 添加新源文件
  - [x] SubTask 8.2: 修复编译错误
  - [x] SubTask 8.3: 验证项目编译成功

# Task Dependencies
- Task 2 depends on Task 1
- Task 3 depends on Task 2
- Task 4 depends on Task 1
- Task 5 depends on Task 3
- Task 6 depends on Task 3
- Task 7 depends on Task 4, Task 5, Task 6
- Task 8 depends on Task 1-7
