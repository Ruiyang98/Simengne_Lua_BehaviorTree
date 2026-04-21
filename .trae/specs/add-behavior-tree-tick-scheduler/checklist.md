# Checklist

## BehaviorTreeScheduler 核心功能
- [x] BehaviorTreeScheduler.h 头文件已创建
- [x] scheduleTree() 方法能将行为树添加到调度列表
- [x] unscheduleTree() 方法能从调度列表移除行为树
- [x] update() 方法能执行所有活跃行为树的一次 tick
- [x] 线程安全机制正确工作（无数据竞争）

## 异步调度线程
- [x] start() 方法能启动调度线程
- [x] stop() 方法能正确停止调度线程
- [x] 调度循环按固定间隔执行 tick
- [x] 完成回调机制正常工作

## BehaviorTreeExecutor 集成
- [x] BehaviorTreeExecutor 包含调度器成员
- [x] executeAsync() 方法能创建并调度行为树
- [x] stopAsync() 方法能停止指定的异步行为树
- [x] getAsyncStatus() 方法能正确查询状态
- [x] listAsyncTrees() 方法能列出所有异步行为树

## AsyncMoveToPoint 异步移动节点
- [x] AsyncMoveToPoint.h 头文件已创建
- [x] tick() 方法每 tick 移动一小段距离
- [x] 到达目标后返回 SUCCESS
- [x] 节点已在 BehaviorTreeExecutor 中注册

## Lua API 扩展
- [x] bt.execute_async() 函数能从 Lua 启动异步行为树
- [x] bt.stop_async() 函数能从 Lua 停止异步行为树
- [x] bt.get_async_status() 函数能查询异步行为树状态
- [x] bt.list_async_trees() 函数能列出所有异步行为树
- [x] bt.set_tick_callback() 函数能设置 tick 回调

## 命令行交互支持
- [x] "bt-async" 命令能启动异步行为树
- [x] "bt-stop" 命令能停止异步行为树
- [x] "bt-list" 命令能列出调度中的行为树
- [x] "bt-status" 命令能查询指定行为树状态

## 示例和测试
- [x] bt_xml/async_square_path.xml 示例文件已创建
- [x] scripts/async_bt_example.lua 示例脚本已创建
- [x] 项目编译成功
