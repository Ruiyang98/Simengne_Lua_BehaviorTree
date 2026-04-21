-- async_bt_example.lua
-- 异步行为树执行示例
-- 演示如何使用行为树调度器进行平滑移动

print("========================================")
print("    Async Behavior Tree Example")
print("========================================")
print()

-- 获取仿真控制器
local sim = require("sim")

-- 创建测试实体
print("Creating test entity...")
local entity_id = sim.add_entity("npc", 0, 0, 0)
print("Created entity: " .. entity_id)
print()

-- 加载行为树 XML 文件
print("Loading behavior tree XML...")
if not bt.load_file("bt_xml/async_square_path.xml") then
    print("ERROR: Failed to load behavior tree: " .. bt.get_last_error())
    return
end
print("OK: Behavior tree loaded")
print()

-- 设置完成回调
local function on_complete(tree_id, status)
    print("[Callback] Tree " .. tree_id .. " completed with status: " .. status)
end

-- 设置 tick 回调
local function on_tick(tree_id)
    -- 可选：每 tick 获取实体位置
    -- local x, y, z = sim.get_entity_position(entity_id)
    -- print("[Tick] Entity at (" .. x .. ", " .. y .. ")")
end

-- 方法 1: 使用 execute_async 启动异步行为树
print("Starting async behavior tree (AsyncSquarePath)...")
local tree_id = bt.execute_async("AsyncSquarePath", entity_id, nil, 100)

if tree_id == "" then
    print("ERROR: Failed to start async behavior tree: " .. bt.get_last_error())
    return
end

print("OK: Async behavior tree started with ID: " .. tree_id)
print()

-- 设置回调
bt.set_complete_callback(tree_id, on_complete)
bt.set_tick_callback(tree_id, on_tick)

-- 监控行为树状态
print("Monitoring behavior tree status...")
print("(Press Ctrl+C to stop)")
print()

local max_wait_seconds = 30
local waited_seconds = 0

while waited_seconds < max_wait_seconds do
    local status = bt.get_async_status(tree_id)
    print("Tree status: " .. status)

    if status == "SUCCESS" or status == "FAILURE" then
        print()
        print("Behavior tree finished!")
        break
    end

    -- 每秒检查一次状态
    sim.sleep(1000)
    waited_seconds = waited_seconds + 1
end

if waited_seconds >= max_wait_seconds then
    print()
    print("Timeout! Stopping behavior tree...")
    bt.stop_async(tree_id)
end

-- 获取最终实体位置
local x, y, z = sim.get_entity_position(entity_id)
print()
print("Final entity position: (" .. x .. ", " .. y .. ", " .. z .. ")")
print()

-- 方法 2: 演示手动模式（可选）
print("========================================")
print("    Manual Mode Example")
print("========================================")
print()

-- 创建另一个实体
local entity2_id = sim.add_entity("npc", 100, 100, 0)
print("Created second entity: " .. entity2_id)

-- 启动另一个异步行为树
local tree_id2 = bt.execute_async("AsyncPatrol", entity2_id, nil, 50)
if tree_id2 ~= "" then
    print("Started patrol behavior tree: " .. tree_id2)

    -- 列出所有运行的行为树
    print()
    print("Running async behavior trees:")
    local trees = bt.list_async_trees()
    for i, tree_info in ipairs(trees) do
        print("  " .. tree_info.id .. " - " .. tree_info.name .. " (" .. tree_info.status .. ")")
    end

    -- 停止巡逻行为树
    sim.sleep(5000)  -- 等待5秒
    print()
    print("Stopping patrol behavior tree...")
    bt.stop_async(tree_id2)
    print("OK: Patrol stopped")
end

print()
print("========================================")
print("    Example completed!")
print("========================================")
