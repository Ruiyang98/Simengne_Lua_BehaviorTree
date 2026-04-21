# 调试指南

本文档介绍如何调试行为树和 Lua 脚本，包括日志输出、断点调试、状态检查等方法。

---

## 目录

1. [Lua 脚本调试](#lua-脚本调试)
2. [行为树调试](#行为树调试)
3. [黑板数据调试](#黑板数据调试)
4. [常见问题排查](#常见问题排查)
5. [高级调试技巧](#高级调试技巧)

---

## Lua 脚本调试

### 1. 基础打印调试

最常用的调试方法是在 Lua 脚本中添加 `print()` 语句：

```lua
-- 打印变量值
local entity_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("创建的实体 ID: " .. entity_id)

-- 打印表内容
local pos = sim.get_entity_position(entity_id)
print("位置: x=" .. pos.x .. ", y=" .. pos.y .. ", z=" .. pos.z)

-- 打印实体列表
local entities = sim.get_all_entities()
print("实体数量: " .. #entities)
for i, entity in ipairs(entities) do
    print("  [" .. i .. "] " .. entity.id .. " (" .. entity.type .. ")")
end
```

### 2. 调试信息函数

创建一个专门的调试函数：

```lua
-- 调试开关
local DEBUG = true

-- 调试打印函数
function debug_print(...)
    if DEBUG then
        print("[DEBUG]", ...)
    end
end

-- 使用示例
debug_print("执行行为树:", tree_name, "实体:", entity_id)
```

### 3. 打印行为树执行流程

```lua
-- 在行为树执行前后打印信息
print("=== 开始执行行为树 ===")
print("树名称: " .. tree_name)
print("实体 ID: " .. entity_id)

local tree_id = bt.execute(tree_name, entity_id, params)

print("树 ID: " .. tree_id)
print("执行状态: " .. bt.get_status(tree_id))
print("=== 行为树执行结束 ===")
```

### 4. 调试自定义节点

在 Lua 自定义节点中添加详细日志：

```lua
bt.register_action("DebugAction", function()
    print("[DebugAction] 开始执行")
    
    -- 打印当前状态
    local count = sim.get_entity_count()
    print("[DebugAction] 当前实体数量: " .. count)
    
    -- 打印所有实体
    local entities = sim.get_all_entities()
    for i, entity in ipairs(entities) do
        print("[DebugAction] 实体 " .. i .. ": " .. entity.id .. 
              " 位置(" .. entity.x .. ", " .. entity.y .. ")")
    end
    
    -- 执行业务逻辑
    print("[DebugAction] 执行业务逻辑...")
    local result = do_something()
    
    print("[DebugAction] 执行完成，结果: " .. tostring(result))
    return result and "SUCCESS" or "FAILURE"
end)
```

### 5. 使用断言检查

```lua
-- 自定义断言函数
function assert_not_empty(value, message)
    if not value or value == "" then
        error("断言失败: " .. (message or "值为空"))
    end
    return value
end

-- 使用示例
local tree_id = bt.execute("MainTree", entity_id)
assert_not_empty(tree_id, "行为树执行失败")
```

### 6. 错误处理

```lua
-- 包装可能出错的代码
local function safe_execute(tree_name, entity_id, params)
    local success, result = pcall(function()
        return bt.execute(tree_name, entity_id, params)
    end)
    
    if not success then
        print("[错误] 执行失败: " .. tostring(result))
        print("[错误] 最后错误信息: " .. bt.get_last_error())
        return nil
    end
    
    return result
end

-- 使用
local tree_id = safe_execute("MainTree", entity_id, params)
if not tree_id then
    print("执行失败，跳过后续操作")
    return
end
```

---

## 行为树调试

### 1. 检查行为树加载状态

```lua
-- 加载前检查文件是否存在（需要 io 库支持）
function file_exists(path)
    local file = io.open(path, "r")
    if file then
        file:close()
        return true
    end
    return false
end

-- 加载并验证
local xml_path = "bt_xml/path_movement.xml"
print("检查文件: " .. xml_path)
print("文件存在: " .. tostring(file_exists(xml_path)))

local success = bt.load_file(xml_path)
print("加载结果: " .. tostring(success))
if not success then
    print("错误信息: " .. bt.get_last_error())
end
```

### 2. 检查行为树执行状态

```lua
-- 执行行为树
local tree_id = bt.execute("MainTree", entity_id, params)

-- 检查树 ID 是否有效
if tree_id == "" then
    print("[错误] 行为树执行失败")
    print("[错误] " .. bt.get_last_error())
    return
end

-- 检查树是否存在
print("树是否存在: " .. tostring(bt.has_tree(tree_id)))

-- 获取执行状态
local status = bt.get_status(tree_id)
print("执行状态: " .. status)

-- 状态判断
if status == "SUCCESS" then
    print("行为树执行成功")
elseif status == "FAILURE" then
    print("行为树执行失败")
elseif status == "RUNNING" then
    print("行为树正在运行")
else
    print("未知状态: " .. tostring(status))
end
```

### 3. 监控长时间运行的行为树

```lua
-- 监控异步行为树（如果有异步执行功能）
local tree_id = bt.execute("LongRunningTree", entity_id)

-- 轮询检查状态
local max_checks = 10
local check_interval = 0.5  -- 秒

for i = 1, max_checks do
    sleep(check_interval)
    local status = bt.get_status(tree_id)
    print("检查 " .. i .. "/" .. max_checks .. ": 状态 = " .. status)
    
    if status ~= "RUNNING" then
        print("行为树已完成，最终状态: " .. status)
        break
    end
end

-- 如果还在运行，可以选择停止
if bt.get_status(tree_id) == "RUNNING" then
    print("超时，停止行为树")
    bt.stop(tree_id)
end
```

### 4. 调试 Lua 自定义节点

```lua
-- 带详细日志的自定义节点
bt.register_condition("DebugCondition", function()
    print("[DebugCondition] 开始评估")
    
    local count = sim.get_entity_count()
    print("[DebugCondition] 实体数量: " .. count)
    
    local result = count > 0
    print("[DebugCondition] 评估结果: " .. tostring(result))
    
    return result
end)

bt.register_action("DebugAction", function()
    print("[DebugAction] ===== 开始执行 =====")
    
    -- 记录开始时间
    local start_time = os.clock()
    
    -- 执行业务逻辑
    print("[DebugAction] 步骤 1: 获取实体")
    local entities = sim.get_all_entities()
    print("[DebugAction] 找到 " .. #entities .. " 个实体")
    
    print("[DebugAction] 步骤 2: 处理实体")
    for i, entity in ipairs(entities) do
        print("[DebugAction]   处理实体 " .. i .. ": " .. entity.id)
        -- 处理逻辑...
    end
    
    -- 记录结束时间
    local end_time = os.clock()
    print("[DebugAction] 执行耗时: " .. (end_time - start_time) .. " 秒")
    print("[DebugAction] ===== 执行完成 =====")
    
    return "SUCCESS"
end)
```

---

## 黑板数据调试

### 1. 打印所有黑板数据

```lua
-- 打印黑板中所有已知键的值
function print_blackboard(tree_id, keys)
    print("=== 黑板数据 (树: " .. tree_id .. ") ===")
    for _, key in ipairs(keys) do
        local value = bt.get_blackboard(tree_id, key)
        print("  " .. key .. " = " .. tostring(value))
    end
    print("=== 黑板数据结束 ===")
end

-- 使用示例
local tree_id = bt.execute("MainTree", entity_id, {
    waypoints = "0,0,0;10,0,0",
    delay_ms = 100
})

-- 打印已知键的值
print_blackboard(tree_id, {
    "entity_id",
    "waypoints",
    "delay_ms",
    "target_x",
    "target_y"
})
```

### 2. 验证黑板数据类型

```lua
-- 检查黑板值的类型
function check_blackboard_type(tree_id, key, expected_type)
    local value = bt.get_blackboard(tree_id, key)
    local actual_type = type(value)
    
    print("检查黑板键: " .. key)
    print("  值: " .. tostring(value))
    print("  类型: " .. actual_type)
    print("  期望类型: " .. expected_type)
    print("  类型匹配: " .. tostring(actual_type == expected_type))
    
    return actual_type == expected_type
end

-- 使用示例
check_blackboard_type(tree_id, "delay_ms", "number")
check_blackboard_type(tree_id, "entity_id", "string")
```

### 3. 黑板数据修改追踪

```lua
-- 追踪黑板数据的变化
function track_blackboard_change(tree_id, key, new_value)
    local old_value = bt.get_blackboard(tree_id, key)
    print("修改黑板键: " .. key)
    print("  原值: " .. tostring(old_value))
    print("  新值: " .. tostring(new_value))
    
    bt.set_blackboard(tree_id, key, new_value)
    
    -- 验证修改
    local verified_value = bt.get_blackboard(tree_id, key)
    print("  验证值: " .. tostring(verified_value))
    print("  修改成功: " .. tostring(tostring(verified_value) == tostring(new_value)))
end

-- 使用示例
track_blackboard_change(tree_id, "custom_value", 42)
```

---

## 常见问题排查

### 问题 1: 行为树加载失败

```lua
-- 症状：bt.load_file() 返回 false

-- 排查步骤：
print("1. 检查文件路径是否正确")
print("   尝试路径: bt_xml/path_movement.xml")

print("2. 检查 XML 语法是否正确")
print("   确保有 <?xml version=\"1.0\"?>")
print("   确保标签正确闭合")

print("3. 检查行为树 ID 是否重复")
print("   确保没有加载同名行为树")

print("4. 查看错误信息")
print("   错误: " .. bt.get_last_error())
```

### 问题 2: 行为树执行失败

```lua
-- 症状：bt.execute() 返回空字符串

-- 排查步骤：
print("1. 检查行为树名称是否正确")
print("   可用名称: MainTree, SquarePath, etc.")

print("2. 检查实体是否存在")
print("   实体存在: " .. tostring(sim.get_entity_position(entity_id) ~= nil))

print("3. 检查必要参数是否提供")
print("   例如 waypoints, delay_ms 等")

print("4. 查看错误信息")
print("   错误: " .. bt.get_last_error())
```

### 问题 3: Lua 自定义节点不执行

```lua
-- 症状：Lua 节点没有输出

-- 排查步骤：
print("1. 检查节点是否正确注册")
bt.register_action("TestAction", function()
    print("TestAction 执行了!")
    return "SUCCESS"
end)

print("2. 检查 XML 中 lua_node_name 是否匹配")
-- XML: <LuaAction lua_node_name="TestAction" />
-- Lua: bt.register_action("TestAction", ...)

print("3. 检查节点是否在行为树中被调用")
-- 确保 XML 中包含 <LuaAction lua_node_name="TestAction" />

print("4. 检查前置条件是否满足")
-- 如果是 Sequence，前面的节点必须成功
```

### 问题 4: 黑板数据读取失败

```lua
-- 症状：bt.get_blackboard() 返回 nil

-- 排查步骤：
print("1. 检查树 ID 是否正确")
print("   树存在: " .. tostring(bt.has_tree(tree_id)))

print("2. 检查键名是否正确")
print("   常见键: entity_id, waypoints, delay_ms")

print("3. 检查数据是否已设置")
-- 在执行时传入的参数会自动设置
-- 或者手动设置: bt.set_blackboard(tree_id, key, value)

print("4. 检查数据类型")
local value = bt.get_blackboard(tree_id, key)
print("   类型: " .. type(value))
print("   值: " .. tostring(value))
```

---

## 高级调试技巧

### 1. 创建调试脚本模板

```lua
-- debug_template.lua
-- 调试脚本模板

local DEBUG = true

function log(...)
    if DEBUG then
        print("[DEBUG]", ...)
    end
end

function assert_tree_loaded(xml_path)
    log("加载行为树:", xml_path)
    local success = bt.load_file(xml_path)
    if not success then
        error("加载失败: " .. bt.get_last_error())
    end
    log("加载成功")
end

function assert_entity_exists(entity_id)
    log("检查实体:", entity_id)
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        error("实体不存在: " .. entity_id)
    end
    log("实体存在，位置:", pos.x, pos.y, pos.z)
end

function execute_tree_with_log(tree_name, entity_id, params)
    log("执行行为树:", tree_name)
    log("  实体:", entity_id)
    log("  参数:", params)
    
    local tree_id = bt.execute(tree_name, entity_id, params)
    
    if tree_id == "" then
        error("执行失败: " .. bt.get_last_error())
    end
    
    log("  树 ID:", tree_id)
    log("  状态:", bt.get_status(tree_id))
    
    return tree_id
end

-- 使用模板
assert_tree_loaded("bt_xml/path_movement.xml")
local entity_id = sim.add_entity("npc", 0, 0, 0)
assert_entity_exists(entity_id)
local tree_id = execute_tree_with_log("MainTree", entity_id, {
    waypoints = "0,0,0;10,0,0",
    delay_ms = 100
})
```

### 2. 性能分析

```lua
-- 简单的性能分析
function profile_execution(func, ...)
    local start = os.clock()
    local result = func(...)
    local elapsed = os.clock() - start
    print("执行耗时: " .. elapsed .. " 秒")
    return result
end

-- 使用
local tree_id = profile_execution(bt.execute, "MainTree", entity_id, params)
```

### 3. 行为树执行追踪器

```lua
-- 创建一个简单的执行追踪器
ExecutionTracer = {
    traces = {}
}

function ExecutionTracer:start_trace(tree_name, entity_id)
    local trace = {
        tree_name = tree_name,
        entity_id = entity_id,
        start_time = os.clock(),
        steps = {}
    }
    table.insert(self.traces, trace)
    return trace
end

function ExecutionTracer:add_step(trace, step_name, details)
    table.insert(trace.steps, {
        name = step_name,
        time = os.clock(),
        details = details
    })
end

function ExecutionTracer:end_trace(trace, status)
    trace.end_time = os.clock()
    trace.status = status
    trace.duration = trace.end_time - trace.start_time
end

function ExecutionTracer:print_report()
    print("\n=== 执行追踪报告 ===")
    for i, trace in ipairs(self.traces) do
        print("\n追踪 #" .. i)
        print("  行为树: " .. trace.tree_name)
        print("  实体: " .. trace.entity_id)
        print("  状态: " .. trace.status)
        print("  总耗时: " .. trace.duration .. " 秒")
        print("  步骤:")
        for j, step in ipairs(trace.steps) do
            print("    [" .. j .. "] " .. step.name .. " (" .. step.time .. ")")
        end
    end
    print("\n=== 报告结束 ===")
end

-- 使用示例
local trace = ExecutionTracer:start_trace("MainTree", entity_id)
ExecutionTracer:add_step(trace, "加载行为树", {file = "path_movement.xml"})
bt.load_file("bt_xml/path_movement.xml")

ExecutionTracer:add_step(trace, "创建实体", {type = "npc"})
local id = sim.add_entity("npc", 0, 0, 0)

ExecutionTracer:add_step(trace, "执行行为树", {})
local tree_id = bt.execute("MainTree", id)

ExecutionTracer:end_trace(trace, bt.get_status(tree_id))
ExecutionTracer:print_report()
```

---

## 总结

调试 Lua 脚本和行为树的关键点：

1. **使用 print() 输出关键信息** - 最简单有效的方法
2. **检查返回值和错误信息** - 使用 `bt.get_last_error()`
3. **验证数据类型和值** - 特别是黑板数据
4. **分步骤调试** - 将复杂逻辑拆分成小步骤
5. **创建调试工具函数** - 提高调试效率

记住：**好的日志胜过复杂的调试器！**
