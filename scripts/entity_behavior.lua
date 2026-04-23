-- Entity Behavior Script
-- 实体行为控制脚本
-- 功能：根据威胁检测、障碍物检测和用户输入决定行为
-- 优先级：威胁 > 障碍物 > 用户路线

print("========================================")
print("    Entity Behavior Controller")
print("========================================")
print("")

-- 加载节点注册（确保所有节点已注册）
require("bt_nodes_registry")

-- ============================================================================
-- 配置参数
-- ============================================================================

local DEFAULT_SENSOR_RANGE = 20
local DEFAULT_CHECK_DISTANCE = 5
local DEFAULT_AVOID_DISTANCE = 3

-- ============================================================================
-- 核心功能函数
-- ============================================================================

--- 运行实体行为树
-- @param entity_id 实体ID
-- @param options 可选参数表
--   - sensor_range: 传感器检测范围（默认20）
--   - check_distance: 障碍物检测距离（默认5）
--   - avoid_distance: 绕行距离（默认3）
--   - tree_name: 行为树名称（默认"EntityBehavior"）
-- @return 行为树执行结果
local function run(entity_id, options)
    entity_id = entity_id or "entity_1"
    options = options or {}

    local sensor_range = options.sensor_range or DEFAULT_SENSOR_RANGE
    local check_distance = options.check_distance or DEFAULT_CHECK_DISTANCE
    local avoid_distance = options.avoid_distance or DEFAULT_AVOID_DISTANCE
    local tree_name = options.tree_name or "EntityBehavior"

    print(string.format("[EntityBehavior] Starting behavior for entity: %s", entity_id))
    print(string.format("[EntityBehavior] Sensor range: %d", sensor_range))
    print(string.format("[EntityBehavior] Check distance: %d", check_distance))
    print(string.format("[EntityBehavior] Avoid distance: %d", avoid_distance))
    print(string.format("[EntityBehavior] Tree name: %s", tree_name))
    print("")

    -- 加载行为树
    if not bt.load_file("bt_xml/entity_behavior.xml") then
        print("[EntityBehavior] ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return nil
    end
    print("[EntityBehavior] Behavior tree loaded successfully")

    -- 执行行为树
    local tree_id = bt.execute(tree_name, entity_id, {
        sensor_range = sensor_range,
        check_distance = check_distance,
        avoid_distance = avoid_distance
    })

    if tree_id ~= "" then
        print(string.format("[EntityBehavior] Tree executed with ID: %s", tree_id))

        -- 获取执行状态
        local status = bt.get_status(tree_id)
        print(string.format("[EntityBehavior] Final status: %s", status))

        return {
            tree_id = tree_id,
            status = status,
            entity_id = entity_id
        }
    else
        print("[EntityBehavior] ERROR: Failed to execute behavior tree: " .. bt.get_last_error())
        return nil
    end
end

--- 异步运行实体行为树
-- @param entity_id 实体ID
-- @param options 可选参数表
-- @return 行为树ID
local function run_async(entity_id, options)
    entity_id = entity_id or "entity_1"
    options = options or {}

    local sensor_range = options.sensor_range or DEFAULT_SENSOR_RANGE
    local check_distance = options.check_distance or DEFAULT_CHECK_DISTANCE
    local avoid_distance = options.avoid_distance or DEFAULT_AVOID_DISTANCE
    local tree_name = options.tree_name or "EntityBehavior"

    print(string.format("[EntityBehavior] Starting async behavior for entity: %s", entity_id))

    -- 加载行为树
    if not bt.load_file("bt_xml/entity_behavior.xml") then
        print("[EntityBehavior] ERROR: Failed to load behavior tree: " .. bt.get_last_error())
        return nil
    end

    -- 异步执行行为树
    local tree_id = bt.execute_async(tree_name, entity_id, {
        sensor_range = sensor_range,
        check_distance = check_distance,
        avoid_distance = avoid_distance
    }, 100)

    if tree_id ~= "" then
        print(string.format("[EntityBehavior] Async tree started with ID: %s", tree_id))
        return tree_id
    else
        print("[EntityBehavior] ERROR: Failed to start async behavior tree: " .. bt.get_last_error())
        return nil
    end
end

--- 设置用户路径
-- @param entity_id 实体ID
-- @param waypoints 路径点字符串，格式: "x1,y1;x2,y2;x3,y3"
-- 示例: "0,0;10,0;10,10;0,10"
local function set_user_path(entity_id, waypoints)
    entity_id = entity_id or "entity_1"

    if not waypoints or waypoints == "" then
        print(string.format("[EntityBehavior] WARNING: Empty waypoints for entity '%s'", entity_id))
        return false
    end

    -- 保存到 blackboard
    bt.set_blackboard(entity_id, "user_path", waypoints)
    print(string.format("[EntityBehavior] Set user path for '%s': %s", entity_id, waypoints))

    return true
end

--- 清除用户路径
-- @param entity_id 实体ID
local function clear_user_path(entity_id)
    entity_id = entity_id or "entity_1"

    bt.set_blackboard(entity_id, "user_path", nil)
    print(string.format("[EntityBehavior] Cleared user path for '%s'", entity_id))
end

--- 获取用户路径
-- @param entity_id 实体ID
-- @return 路径点字符串，如果没有则返回 nil
local function get_user_path(entity_id)
    entity_id = entity_id or "entity_1"

    local path = bt.get_blackboard(entity_id, "user_path")
    return path
end

--- 检查是否有用户路径
-- @param entity_id 实体ID
-- @return true 如果有用户路径，否则 false
local function has_user_path(entity_id)
    entity_id = entity_id or "entity_1"

    local path = bt.get_blackboard(entity_id, "user_path")
    return path ~= nil and path ~= ""
end

--- 停止实体行为树
-- @param tree_id 行为树ID
-- @return true 如果成功停止
local function stop(tree_id)
    if not tree_id or tree_id == "" then
        print("[EntityBehavior] WARNING: Invalid tree_id")
        return false
    end

    local result = bt.stop(tree_id)
    print(string.format("[EntityBehavior] Stopped tree '%s': %s", tree_id, tostring(result)))
    return result
end

--- 获取行为树状态
-- @param tree_id 行为树ID
-- @return 状态字符串
local function get_status(tree_id)
    if not tree_id or tree_id == "" then
        return "INVALID"
    end

    return bt.get_status(tree_id)
end

-- ============================================================================
-- 测试和示例函数
-- ============================================================================

--- 运行完整测试
local function run_tests()
    print("")
    print("========================================")
    print("    Entity Behavior Tests")
    print("========================================")
    print("")

    -- 创建测试实体
    print("Creating test entities...")
    local entity_id = sim.add_entity("npc", 0, 0, 0)
    print(string.format("Created entity: %s", entity_id))

    -- 测试1: 仅用户路径（无威胁，无障碍物）
    print("")
    print("--- Test 1: User Path Only ---")
    set_user_path(entity_id, "10,0;10,10;0,10")
    local result1 = run(entity_id, {tree_name = "EntityBehaviorPathOnly"})
    clear_user_path(entity_id)

    -- 测试2: 创建威胁实体，测试威胁响应
    print("")
    print("--- Test 2: Threat Response ---")
    local threat_id = sim.add_entity("threat", 5, 5, 0)
    print(string.format("Created threat: %s at (5, 5)", threat_id))
    local result2 = run(entity_id, {tree_name = "EntityBehaviorThreatOnly"})
    sim.remove_entity(threat_id)

    -- 测试3: 完整行为树（优先级测试）
    print("")
    print("--- Test 3: Full Behavior (Priority Test) ---")
    -- 同时设置用户路径和威胁
    set_user_path(entity_id, "20,0;20,20")
    threat_id = sim.add_entity("threat", 3, 3, 0)
    print(string.format("Created threat: %s at (3, 3)", threat_id))
    print("Expected: Should prioritize threat over user path")
    local result3 = run(entity_id, {tree_name = "EntityBehavior"})
    sim.remove_entity(threat_id)
    clear_user_path(entity_id)

    -- 清理
    print("")
    print("Cleaning up...")
    sim.remove_entity(entity_id)
    print("Tests completed!")

    return {
        test1 = result1,
        test2 = result2,
        test3 = result3
    }
end

--- 运行示例场景
local function run_example()
    print("")
    print("========================================")
    print("    Entity Behavior Example")
    print("========================================")
    print("")

    -- 创建实体
    local entity_id = sim.add_entity("guard", 0, 0, 0)
    print(string.format("Created guard entity: %s", entity_id))

    -- 设置用户巡逻路线
    set_user_path(entity_id, "0,0;15,0;15,15;0,15")

    -- 运行行为树
    print("")
    print("Starting entity behavior...")
    local result = run(entity_id)

    -- 模拟威胁出现
    print("")
    print("Simulating threat appearance...")
    local threat_id = sim.add_entity("enemy", 5, 5, 0)
    print(string.format("Threat appeared: %s at (5, 5)", threat_id))

    -- 再次运行（应该优先响应威胁）
    print("")
    print("Re-running behavior (should respond to threat)...")
    result = run(entity_id)

    -- 清理
    sim.remove_entity(threat_id)
    sim.remove_entity(entity_id)
    clear_user_path(entity_id)

    print("")
    print("Example completed!")

    return result
end

-- ============================================================================
-- 导出模块
-- ============================================================================

return {
    -- 核心功能
    run = run,
    run_async = run_async,
    stop = stop,
    get_status = get_status,

    -- 路径管理
    set_user_path = set_user_path,
    clear_user_path = clear_user_path,
    get_user_path = get_user_path,
    has_user_path = has_user_path,

    -- 测试和示例
    run_tests = run_tests,
    run_example = run_example,

    -- 配置
    default_sensor_range = DEFAULT_SENSOR_RANGE,
    default_check_distance = DEFAULT_CHECK_DISTANCE,
    default_avoid_distance = DEFAULT_AVOID_DISTANCE
}
