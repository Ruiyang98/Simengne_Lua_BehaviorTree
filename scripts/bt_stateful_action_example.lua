-- bt_stateful_action_example.lua
-- 演示如何使用 LuaStatefulAction 实现持续移动
-- 这是 StatefulActionNode 的 Lua 版本，支持 onStart/onRunning/onHalted 生命周期

print("[BT Stateful Action Example] Starting...")

-- ============================================
-- 状态管理表（用于保存每个实体的移动状态）
-- ============================================
local moveStates = {}

-- ============================================
-- 辅助函数：计算两点距离
-- ============================================
local function distance(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

-- ============================================
-- LuaStatefulMoveTo: 有状态的移动到目标点
-- 这个节点会在多次 tick 中持续执行，直到到达目标
-- ============================================
bt.register_stateful_action("LuaStatefulMoveTo",
    -- onStart: 首次进入时调用
    function(params)
        local entity_id = params.entity_id or ""
        local target_x = tonumber(params.x) or 0
        local target_y = tonumber(params.y) or 0
        local speed = tonumber(params.speed) or 1.0
        local threshold = tonumber(params.threshold) or 0.5

        print(string.format("[LuaStatefulMoveTo:onStart] Entity '%s' starting move to (%.1f, %.1f)",
                            entity_id, target_x, target_y))

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulMoveTo:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- 检查是否已经在目标点
        local dist = distance(pos.x, pos.y, target_x, target_y)
        if dist <= threshold then
            print(string.format("[LuaStatefulMoveTo:onStart] Already at destination (distance: %.2f)", dist))
            return "SUCCESS"
        end

        -- 初始化移动状态
        moveStates[entity_id] = {
            targetX = target_x,
            targetY = target_y,
            speed = speed,
            threshold = threshold,
            startX = pos.x,
            startY = pos.y,
            startTime = os.time()
        }

        print(string.format("[LuaStatefulMoveTo:onStart] Distance to target: %.2f, starting movement", dist))
        return "RUNNING"  -- 返回 RUNNING 表示需要继续执行
    end,

    -- onRunning: 每次 tick 时调用（只要返回 RUNNING 就会持续调用）
    function(params)
        local entity_id = params.entity_id or ""

        -- 获取移动状态
        local state = moveStates[entity_id]
        if not state then
            print(string.format("[LuaStatefulMoveTo:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulMoveTo:onRunning] ERROR: Entity '%s' not found", entity_id))
            moveStates[entity_id] = nil  -- 清理状态
            return "FAILURE"
        end

        -- 检查是否到达目标
        local dist = distance(pos.x, pos.y, state.targetX, state.targetY)
        if dist <= state.threshold then
            print(string.format("[LuaStatefulMoveTo:onRunning] Entity '%s' arrived at destination (%.1f, %.1f)",
                                entity_id, state.targetX, state.targetY))
            moveStates[entity_id] = nil  -- 清理状态
            return "SUCCESS"  -- 返回 SUCCESS 表示完成
        end

        -- 计算移动方向
        local dx = state.targetX - pos.x
        local dy = state.targetY - pos.y
        local moveDist = math.sqrt(dx * dx + dy * dy)

        -- 移动一步
        local newX, newY
        if moveDist <= state.speed then
            -- 可以到达目标
            newX = state.targetX
            newY = state.targetY
        else
            -- 向目标移动 speed 距离
            local ratio = state.speed / moveDist
            newX = pos.x + dx * ratio
            newY = pos.y + dy * ratio
        end

        -- 更新实体位置
        sim.move_entity(entity_id, newX, newY, pos.z)

        print(string.format("[LuaStatefulMoveTo:onRunning] Entity '%s' moved to (%.1f, %.1f), distance to target: %.2f",
                            entity_id, newX, newY, dist))

        return "RUNNING"  -- 返回 RUNNING 表示继续执行，下次 tick 继续调用 onRunning
    end,

    -- onHalted: 节点被中断时调用
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulMoveTo:onHalted] Movement halted for entity '%s'", entity_id))
        moveStates[entity_id] = nil  -- 清理状态
    end
)
print("  [OK] LuaStatefulMoveTo registered")

-- ============================================
-- LuaStatefulPatrol: 有状态的巡逻节点
-- 在多个路径点之间持续移动
-- ============================================
bt.register_stateful_action("LuaStatefulPatrol",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""
        local waypoints_str = params.waypoints or "0,0;5,0;5,5;0,5"
        local speed = tonumber(params.speed) or 1.0

        print(string.format("[LuaStatefulPatrol:onStart] Entity '%s' starting patrol", entity_id))

        -- 解析路径点
        local waypoints = {}
        for x, y in string.gmatch(waypoints_str, "([%d%.%-]+),([%d%.%-]+)") do
            table.insert(waypoints, {x = tonumber(x), y = tonumber(y)})
        end

        if #waypoints == 0 then
            print("[LuaStatefulPatrol:onStart] ERROR: No valid waypoints")
            return "FAILURE"
        end

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulPatrol:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- 找到最近的路径点作为起点
        local currentWaypoint = 1
        local minDist = math.huge
        for i, wp in ipairs(waypoints) do
            local dist = distance(pos.x, pos.y, wp.x, wp.y)
            if dist < minDist then
                minDist = dist
                currentWaypoint = i
            end
        end

        -- 初始化巡逻状态
        moveStates[entity_id .. "_patrol"] = {
            waypoints = waypoints,
            currentIndex = currentWaypoint,
            speed = speed,
            cycles = tonumber(params.cycles) or 1,
            currentCycle = 1
        }

        print(string.format("[LuaStatefulPatrol:onStart] Starting at waypoint %d/%d, cycles: %d",
                            currentWaypoint, #waypoints, moveStates[entity_id .. "_patrol"].cycles))
        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local state = moveStates[entity_id .. "_patrol"]

        if not state then
            print(string.format("[LuaStatefulPatrol:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulPatrol:onRunning] ERROR: Entity '%s' not found", entity_id))
            moveStates[entity_id .. "_patrol"] = nil
            return "FAILURE"
        end

        -- 获取当前目标路径点
        local targetWp = state.waypoints[state.currentIndex]
        local dist = distance(pos.x, pos.y, targetWp.x, targetWp.y)

        -- 检查是否到达当前路径点
        if dist <= 0.5 then
            print(string.format("[LuaStatefulPatrol:onRunning] Reached waypoint %d/%d",
                                state.currentIndex, #state.waypoints))

            -- 移动到下一个路径点
            state.currentIndex = state.currentIndex + 1

            -- 检查是否完成所有路径点
            if state.currentIndex > #state.waypoints then
                state.currentCycle = state.currentCycle + 1

                if state.currentCycle > state.cycles then
                    print(string.format("[LuaStatefulPatrol:onRunning] Patrol completed after %d cycles", state.cycles))
                    moveStates[entity_id .. "_patrol"] = nil
                    return "SUCCESS"
                end

                -- 开始下一轮
                state.currentIndex = 1
                print(string.format("[LuaStatefulPatrol:onRunning] Starting cycle %d/%d",
                                    state.currentCycle, state.cycles))
            end

            -- 更新目标
            targetWp = state.waypoints[state.currentIndex]
        end

        -- 向目标路径点移动
        local dx = targetWp.x - pos.x
        local dy = targetWp.y - pos.y
        local moveDist = math.sqrt(dx * dx + dy * dy)

        local newX, newY
        if moveDist <= state.speed then
            newX = targetWp.x
            newY = targetWp.y
        else
            local ratio = state.speed / moveDist
            newX = pos.x + dx * ratio
            newY = pos.y + dy * ratio
        end

        sim.move_entity(entity_id, newX, newY, pos.z)

        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulPatrol:onHalted] Patrol halted for entity '%s'", entity_id))
        moveStates[entity_id .. "_patrol"] = nil
    end
)
print("  [OK] LuaStatefulPatrol registered")

-- ============================================
-- LuaStatefulWait: 有状态的等待节点
-- 等待指定时间（需要多次tick）
-- ============================================
local waitStates = {}

bt.register_stateful_action("LuaStatefulWait",
    -- onStart
    function(params)
        local duration = tonumber(params.duration) or 1.0
        local entity_id = params.entity_id or "default"

        print(string.format("[LuaStatefulWait:onStart] Starting wait for %.1f seconds", duration))

        waitStates[entity_id] = {
            startTime = os.time(),
            duration = duration
        }

        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or "default"
        local state = waitStates[entity_id]

        if not state then
            return "FAILURE"
        end

        local elapsed = os.time() - state.startTime
        if elapsed >= state.duration then
            print(string.format("[LuaStatefulWait:onRunning] Wait completed after %d seconds", elapsed))
            waitStates[entity_id] = nil
            return "SUCCESS"
        end

        print(string.format("[LuaStatefulWait:onRunning] Waiting... %d/%d seconds", elapsed, state.duration))
        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or "default"
        print(string.format("[LuaStatefulWait:onHalted] Wait interrupted for '%s'", entity_id))
        waitStates[entity_id] = nil
    end
)
print("  [OK] LuaStatefulWait registered")

print("[BT Stateful Action Example] All stateful actions registered!")
print("")
print("使用示例:")
print("  1. 加载此脚本: bt.load_registry('scripts/bt_stateful_action_example.lua')")
print("  2. 加载XML: bt.load_file('bt_xml/lua_stateful_nodes.xml')")
print("  3. 执行: bt.execute_async('LuaStatefulMoveExample', 'entity_001')")
print("")
