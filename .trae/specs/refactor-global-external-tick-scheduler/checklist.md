# Checklist

## 单例模式
- [x] BehaviorTreeScheduler::getInstance() 返回全局唯一实例
- [x] 多次调用 getInstance() 返回同一实例
- [x] 构造函数和析构函数已私有化
- [x] 已删除线程相关代码（start, stop, schedulerLoop）

## 外部 Tick 驱动
- [x] tickAll() 方法能依次执行所有行为树的一次 tick
- [x] 调度器不创建任何内部线程
- [x] getTickIntervalMs() 返回固定值 500
- [x] tickTree() 不再检查时间间隔

## 实体注册机制
- [x] registerEntity() 方法能将实体添加到调度器
- [x] registerEntityWithTree() 方法能注册实体并关联行为树
- [x] unregisterEntity() 方法能从调度器移除实体
- [x] pauseEntity() 方法能暂停实体行为树调度
- [x] resumeEntity() 方法能恢复实体行为树调度
- [x] 数据结构使用 entityId 作为主键

## 状态查询接口
- [x] getEntityStatus() 方法能查询实体行为树状态
- [x] getRegisteredEntityIds() 方法返回所有实体 ID
- [x] getEntityInfo() 方法能获取实体信息
- [x] hasEntity() 方法能检查实体是否存在

## BehaviorTreeExecutor 集成
- [x] BehaviorTreeExecutor 使用全局调度器
- [x] executeAsync() 使用新的注册接口
- [x] stopAsync() 使用新的注销接口
- [x] 移除了调度器生命周期管理代码

## Lua API 更新
- [x] Lua 绑定使用全局调度器
- [x] bt.execute_async() 使用新接口
- [x] bt.stop_async() 使用新接口
- [x] bt.get_async_status() 使用新接口

## 命令行更新
- [x] "bt-async" 命令使用新接口
- [x] "bt-stop" 命令使用新接口
- [x] "bt-list" 命令使用新接口
- [x] 主循环中添加了 tickAll() 调用

## 编译验证
- [x] 项目编译成功
- [x] 无编译警告（除 Unicode 编码警告外）
