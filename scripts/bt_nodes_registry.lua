-- bt_nodes_registry.lua
-- 全局行为树节点注册中心
-- 采用单个注册方式，直观显示每个节点的逻辑

print("[BT Nodes Registry] Starting node registration...")

-- ============================================
-- 移动相关动作节点
-- ============================================

bt.register_action("LuaMoveTo", function(params)
    local entity_id = params.entity_id or ""
    local target_x = tonumber(params.target_x) or 0
    local target_y = tonumber(params.target_y) or 0
    local speed = tonumber(params.speed) or 1.0

    print(string.format("[LuaMoveTo] Moving entity '%s' to (%.1f, %.1f) at speed %.1f",
                        entity_id, target_x, target_y, speed))

    sim.move_entity(entity_id, target_x, target_y, 0)
    return "SUCCESS"
end)
print("  [OK] LuaMoveTo registered")

bt.register_action("LuaPatrol", function(params)
    local entity_id = params.entity_id or ""
    local radius = tonumber(params.radius) or 5.0
    local num_points = tonumber(params.num_points) or 4

    print(string.format("[LuaPatrol] Entity '%s' patrolling with radius %.1f, %d points",
                        entity_id, radius, num_points))

    local pos = sim.get_entity_position(entity_id)
    if pos then
        for i = 1, num_points do
            local angle = (2 * math.pi * (i - 1)) / num_points
            local new_x = pos.x + radius * math.cos(angle)
            local new_y = pos.y + radius * math.sin(angle)
            sim.move_entity(entity_id, new_x, new_y, pos.z)
        end
    end

    return "SUCCESS"
end)
print("  [OK] LuaPatrol registered")

bt.register_action("LuaFlee", function(params)
    local entity_id = params.entity_id or ""
    local distance = tonumber(params.distance) or 10.0

    print(string.format("[LuaFlee] Entity '%s' fleeing %d units", entity_id, distance))
    return "SUCCESS"
end)
print("  [OK] LuaFlee registered")

-- ============================================
-- 战斗相关动作节点
-- ============================================

bt.register_action("LuaAttack", function(params)
    local attacker_id = params.attacker_id or ""
    local target_id = params.target_id or ""
    local damage = tonumber(params.damage) or 10

    print(string.format("[LuaAttack] '%s' attacks '%s' for %d damage",
                        attacker_id, target_id, damage))
    return "SUCCESS"
end)
print("  [OK] LuaAttack registered")

bt.register_action("LuaDefend", function(params)
    local entity_id = params.entity_id or ""
    local duration = tonumber(params.duration) or 2.0

    print(string.format("[LuaDefend] Entity '%s' defending for %.1f seconds",
                        entity_id, duration))
    return "SUCCESS"
end)
print("  [OK] LuaDefend registered")

-- ============================================
-- 交互相关动作节点
-- ============================================

bt.register_action("LuaWait", function(params)
    local duration = tonumber(params.duration) or 1.0
    print(string.format("[LuaWait] Waiting for %.1f seconds", duration))
    return "SUCCESS"
end)
print("  [OK] LuaWait registered")

bt.register_action("LuaInteract", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""
    local action = params.action or "use"

    print(string.format("[LuaInteract] '%s' %s '%s'", entity_id, action, target_id))
    return "SUCCESS"
end)
print("  [OK] LuaInteract registered")

-- ============================================
-- 健康状态条件节点
-- ============================================

bt.register_condition("LuaIsHealthy", function(params)
    local entity_id = params.entity_id or ""
    local min_health = tonumber(params.min_health) or 50

    -- 简化示例：假设总是健康
    print(string.format("[LuaIsHealthy] Checking '%s' health >= %d: true",
                        entity_id, min_health))
    return true
end)
print("  [OK] LuaIsHealthy registered")

bt.register_condition("LuaIsLowHealth", function(params)
    local entity_id = params.entity_id or ""
    local threshold = tonumber(params.threshold) or 30

    -- 简化示例：假设从不低血量
    print(string.format("[LuaIsLowHealth] Checking '%s' health < %d: false",
                        entity_id, threshold))
    return false
end)
print("  [OK] LuaIsLowHealth registered")

-- ============================================
-- 目标检测条件节点
-- ============================================

bt.register_condition("LuaHasTarget", function(params)
    local entity_id = params.entity_id or ""
    local range = tonumber(params.range) or 10

    print(string.format("[LuaHasTarget] '%s' checking for target within range %d: true",
                        entity_id, range))
    return true
end)
print("  [OK] LuaHasTarget registered")

bt.register_condition("LuaCanSeeEnemy", function(params)
    local entity_id = params.entity_id or ""
    local range = tonumber(params.range) or 15
    local angle = tonumber(params.angle) or 120

    print(string.format("[LuaCanSeeEnemy] '%s' checking vision (range=%d, angle=%d): true",
                        entity_id, range, angle))
    return true
end)
print("  [OK] LuaCanSeeEnemy registered")

-- ============================================
-- 距离检查条件节点
-- ============================================

bt.register_condition("LuaIsInRange", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""
    local range = tonumber(params.range) or 5

    print(string.format("[LuaIsInRange] '%s' distance to '%s' <= %d: true",
                        entity_id, target_id, range))
    return true
end)
print("  [OK] LuaIsInRange registered")

-- ============================================
-- 实体存在条件节点
-- ============================================

bt.register_condition("LuaHasEntities", function(params)
    local count = sim.get_entity_count()
    print(string.format("[LuaHasEntities] Entity count: %d", count))
    return count > 0
end)
print("  [OK] LuaHasEntities registered")

bt.register_condition("LuaEntityExists", function(params)
    local entity_id = params.entity_id or ""
    local pos = sim.get_entity_position(entity_id)
    local exists = pos ~= nil
    print(string.format("[LuaEntityExists] '%s' exists: %s", entity_id, tostring(exists)))
    return exists
end)
print("  [OK] LuaEntityExists registered")

-- ============================================
-- 有状态动作节点 (从 example_bt_advanced.lua 迁移)
-- ============================================

-- 状态管理表
local moveStates = {}
local waitStates = {}

-- LuaStatefulMoveTo: 有状态移动到目标点
bt.register_stateful_action("LuaStatefulMoveTo",
    -- onStart: 首次进入节点时调用
    function(params)
        local entity_id = params.entity_id or ""
        local target_x = tonumber(params.x) or 0
        local target_y = tonumber(params.y) or 0
        local target_z = tonumber(params.z) or 0
        local threshold = tonumber(params.threshold) or 0.5

        print(string.format("[LuaStatefulMoveTo:onStart] Entity '%s' starting move to (%.1f, %.1f, %.1f)",
                            entity_id, target_x, target_y, target_z))

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[LuaStatefulMoveTo:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- 使用 get_entity_distance 检查距离
        local dist = sim.get_entity_distance(entity_id, target_x, target_y, target_z)
        if dist <= threshold then
            print(string.format("[LuaStatefulMoveTo:onStart] Already at destination (distance: %.2f)", dist))
            return "SUCCESS"
        end

        -- 计算方向向量
        local dx = target_x - pos.x
        local dy = target_y - pos.y
        local dz = target_z - pos.z

        -- 设置移动方向
        if not sim.set_entity_move_direction(entity_id, dx, dy, dz) then
            print(string.format("[LuaStatefulMoveTo:onStart] ERROR: Failed to set move direction for '%s'", entity_id))
            return "FAILURE"
        end

        -- 保存目标位置供 onRunning 使用
        moveStates[entity_id] = {
            targetX = target_x,
            targetY = target_y,
            targetZ = target_z,
            threshold = threshold
        }

        print(string.format("[LuaStatefulMoveTo:onStart] Distance to target: %.2f, movement started", dist))
        return "RUNNING"
    end,

    -- onRunning: 每帧调用，直到节点完成
    function(params)
        local entity_id = params.entity_id or ""

        -- 获取移动状态
        local state = moveStates[entity_id]
        if not state then
            print(string.format("[LuaStatefulMoveTo:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        -- 使用 get_entity_distance 检查是否到达
        local dist = sim.get_entity_distance(entity_id, state.targetX, state.targetY, state.targetZ)

        if dist <= state.threshold then
            print(string.format("[LuaStatefulMoveTo:onRunning] Entity '%s' arrived at destination (%.1f, %.1f, %.1f)",
                                entity_id, state.targetX, state.targetY, state.targetZ))

            -- 停止移动
            sim.set_entity_move_direction(entity_id, 0, 0, 0)

            -- 清理状态
            moveStates[entity_id] = nil
            return "SUCCESS"
        end

        -- 仍在移动中，继续返回 RUNNING
        return "RUNNING"
    end,

    -- onHalted: 节点被中断时调用
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulMoveTo:onHalted] Movement halted for entity '%s'", entity_id))

        -- 停止移动
        sim.set_entity_move_direction(entity_id, 0, 0, 0)

        -- 清理状态
        moveStates[entity_id] = nil
    end
)
print("  [OK] LuaStatefulMoveTo registered")

-- LuaStatefulPatrol: 有状态巡逻节点
bt.register_stateful_action("LuaStatefulPatrol",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""
        local waypoints_str = params.waypoints or "0,0;5,0;5,5;0,5"

        print(string.format("[LuaStatefulPatrol:onStart] Entity '%s' starting patrol", entity_id))

        -- 解析路径点
        local waypoints = {}
        for x, y in string.gmatch(waypoints_str, "([%d%.%-]+),([%d%.%-]+)") do
            table.insert(waypoints, {x = tonumber(x), y = tonumber(y), z = 0})
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
            local dist = math.sqrt((pos.x - wp.x)^2 + (pos.y - wp.y)^2)
            if dist < minDist then
                minDist = dist
                currentWaypoint = i
            end
        end

        -- 初始化巡逻状态
        local stateKey = entity_id .. "_patrol"
        moveStates[stateKey] = {
            waypoints = waypoints,
            currentIndex = currentWaypoint,
            cycles = tonumber(params.cycles) or 1,
            currentCycle = 1,
            threshold = 0.5
        }

        -- 设置初始方向指向第一个路径点
        local targetWp = waypoints[currentWaypoint]
        local dx = targetWp.x - pos.x
        local dy = targetWp.y - pos.y
        local dz = targetWp.z - pos.z
        sim.set_entity_move_direction(entity_id, dx, dy, dz)

        print(string.format("[LuaStatefulPatrol:onStart] Starting at waypoint %d/%d, cycles: %d",
                            currentWaypoint, #waypoints, moveStates[stateKey].cycles))
        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_patrol"
        local state = moveStates[stateKey]

        if not state then
            print(string.format("[LuaStatefulPatrol:onRunning] ERROR: No state for entity '%s'", entity_id))
            return "FAILURE"
        end

        -- 获取当前目标路径点
        local targetWp = state.waypoints[state.currentIndex]

        -- 使用 get_entity_distance 检查是否到达当前路径点
        local dist = sim.get_entity_distance(entity_id, targetWp.x, targetWp.y, targetWp.z)

        if dist <= state.threshold then
            print(string.format("[LuaStatefulPatrol:onRunning] Reached waypoint %d/%d",
                                state.currentIndex, #state.waypoints))

            -- 移动到下一个路径点
            state.currentIndex = state.currentIndex + 1

            -- 检查是否完成所有路径点
            if state.currentIndex > #state.waypoints then
                state.currentCycle = state.currentCycle + 1

                if state.currentCycle > state.cycles then
                    print(string.format("[LuaStatefulPatrol:onRunning] Patrol completed after %d cycles", state.cycles))

                    -- 停止移动
                    sim.set_entity_move_direction(entity_id, 0, 0, 0)

                    moveStates[stateKey] = nil
                    return "SUCCESS"
                end

                -- 开始下一轮
                state.currentIndex = 1
                print(string.format("[LuaStatefulPatrol:onRunning] Starting cycle %d/%d",
                                    state.currentCycle, state.cycles))
            end

            -- 更新方向指向下一个路径点
            targetWp = state.waypoints[state.currentIndex]
            local pos = sim.get_entity_position(entity_id)
            if pos then
                local dx = targetWp.x - pos.x
                local dy = targetWp.y - pos.y
                local dz = targetWp.z - pos.z
                sim.set_entity_move_direction(entity_id, dx, dy, dz)
            end
        end

        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        print(string.format("[LuaStatefulPatrol:onHalted] Patrol halted for entity '%s'", entity_id))

        -- 停止移动
        sim.set_entity_move_direction(entity_id, 0, 0, 0)

        moveStates[entity_id .. "_patrol"] = nil
    end
)
print("  [OK] LuaStatefulPatrol registered")

-- LuaStatefulWait: 有状态等待节点
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

-- ============================================
-- 实体行为相关节点
-- ============================================

-- 1. 检查传感器是否探测到威胁（最高优先级）
bt.register_condition("SensorDetectThreat", function(params)
    local entity_id = params.entity_id or ""
    local sensor_range = tonumber(params.sensor_range) or 20

    -- 获取实体位置
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return false
    end

    -- 获取范围内的所有实体
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range and (entity.type == "enemy" or entity.type == "threat") then
                -- 保存检测到的威胁ID到 blackboard
                bt.set_blackboard(entity_id, "detected_threat_id", entity.id)
                bt.set_blackboard(entity_id, "threat_position", {x = entity.x, y = entity.y, z = entity.z})
                print(string.format("[SensorDetectThreat] Entity '%s' detected threat '%s' at distance %.2f",
                                    entity_id, entity.id, dist))
                return true
            end
        end
    end

    return false
end)
print("  [OK] SensorDetectThreat registered")

-- 2. 检查前方是否有障碍物（中等优先级）
bt.register_condition("HasObstacle", function(params)
    local entity_id = params.entity_id or ""
    local check_distance = tonumber(params.check_distance) or 5

    -- 获取实体位置
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return false
    end

    -- 简化实现：检查前方是否有实体阻挡
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= entity_id then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= check_distance then
                -- 保存障碍物信息
                bt.set_blackboard(entity_id, "obstacle_id", entity.id)
                bt.set_blackboard(entity_id, "obstacle_position", {x = entity.x, y = entity.y, z = entity.z})
                print(string.format("[HasObstacle] Entity '%s' detected obstacle '%s' at distance %.2f",
                                    entity_id, entity.id, dist))
                return true
            end
        end
    end

    return false
end)
print("  [OK] HasObstacle registered")

-- 3. 检查是否有用户输入路线（最低优先级）
bt.register_condition("HasUserPath", function(params)
    local entity_id = params.entity_id or ""
    -- 检查 blackboard 中是否有用户路径
    local path = bt.get_blackboard(entity_id, "user_path")
    local has_path = path ~= nil and #path > 0
    print(string.format("[HasUserPath] Entity '%s' has user path: %s", entity_id, tostring(has_path)))
    return has_path
end)
print("  [OK] HasUserPath registered")

-- 4. 移动到威胁位置（有状态动作）
bt.register_stateful_action("MoveToThreat",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""

        -- 从 blackboard 获取威胁位置
        local threat_pos = bt.get_blackboard(entity_id, "threat_position")
        if not threat_pos then
            print(string.format("[MoveToThreat:onStart] ERROR: No threat position for entity '%s'", entity_id))
            return "FAILURE"
        end

        local target_x = threat_pos.x
        local target_y = threat_pos.y
        local target_z = threat_pos.z or 0
        local threshold = tonumber(params.threshold) or 1.0

        print(string.format("[MoveToThreat:onStart] Entity '%s' starting move to threat at (%.1f, %.1f, %.1f)",
                            entity_id, target_x, target_y, target_z))

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[MoveToThreat:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- 检查距离
        local dist = sim.get_entity_distance(entity_id, target_x, target_y, target_z)
        if dist <= threshold then
            print(string.format("[MoveToThreat:onStart] Already at threat location (distance: %.2f)", dist))
            return "SUCCESS"
        end

        -- 计算方向向量
        local dx = target_x - pos.x
        local dy = target_y - pos.y
        local dz = target_z - pos.z

        -- 设置移动方向
        if not sim.set_entity_move_direction(entity_id, dx, dy, dz) then
            print(string.format("[MoveToThreat:onStart] ERROR: Failed to set move direction for '%s'", entity_id))
            return "FAILURE"
        end

        -- 保存目标位置
        local stateKey = entity_id .. "_threat"
        moveStates[stateKey] = {
            targetX = target_x,
            targetY = target_y,
            targetZ = target_z,
            threshold = threshold
        }

        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_threat"
        local state = moveStates[stateKey]

        if not state then
            return "FAILURE"
        end

        -- 检查是否到达
        local dist = sim.get_entity_distance(entity_id, state.targetX, state.targetY, state.targetZ)

        if dist <= state.threshold then
            print(string.format("[MoveToThreat:onRunning] Entity '%s' arrived at threat location", entity_id))
            sim.set_entity_move_direction(entity_id, 0, 0, 0)
            moveStates[stateKey] = nil
            return "SUCCESS"
        end

        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_threat"
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        moveStates[stateKey] = nil
    end
)
print("  [OK] MoveToThreat registered")

-- 5. 执行打击
bt.register_action("ExecuteAttack", function(params)
    local entity_id = params.entity_id or ""
    local target_id = params.target_id or ""

    print(string.format("[ExecuteAttack] Entity '%s' attacking target '%s'", entity_id, target_id))
    -- 实际攻击逻辑
    return "SUCCESS"
end)
print("  [OK] ExecuteAttack registered")

-- 6. 绕行障碍物
bt.register_action("AvoidObstacle", function(params)
    local entity_id = params.entity_id or ""
    local avoid_distance = tonumber(params.avoid_distance) or 3

    -- 获取障碍物位置
    local obstacle_pos = bt.get_blackboard(entity_id, "obstacle_position")
    if not obstacle_pos then
        return "FAILURE"
    end

    -- 计算绕行方向（简化：向右侧绕行）
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        return "FAILURE"
    end

    -- 计算垂直方向（右侧）
    local dx = obstacle_pos.x - pos.x
    local dy = obstacle_pos.y - pos.y
    local dist = math.sqrt(dx * dx + dy * dy)

    if dist > 0 then
        -- 垂直向量（顺时针90度）
        local perp_x = dy / dist * avoid_distance
        local perp_y = -dx / dist * avoid_distance

        -- 移动到新位置
        local new_x = pos.x + perp_x
        local new_y = pos.y + perp_y
        sim.move_entity(entity_id, new_x, new_y, pos.z)

        print(string.format("[AvoidObstacle] Entity '%s' avoiding obstacle to (%.1f, %.1f)",
                            entity_id, new_x, new_y))
    end

    return "SUCCESS"
end)
print("  [OK] AvoidObstacle registered")

-- 7. 继续移动（恢复原有路径）
bt.register_action("ResumeMovement", function(params)
    local entity_id = params.entity_id or ""

    -- 清除障碍物信息
    bt.set_blackboard(entity_id, "obstacle_id", nil)
    bt.set_blackboard(entity_id, "obstacle_position", nil)

    print(string.format("[ResumeMovement] Entity '%s' resuming normal movement", entity_id))
    return "SUCCESS"
end)
print("  [OK] ResumeMovement registered")

-- 8. 执行用户路线（有状态动作）
bt.register_stateful_action("FollowUserPath",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""

        -- 从 blackboard 获取用户路径
        local waypoints_str = bt.get_blackboard(entity_id, "user_path")
        if not waypoints_str or waypoints_str == "" then
            print(string.format("[FollowUserPath:onStart] ERROR: No user path for entity '%s'", entity_id))
            return "FAILURE"
        end

        print(string.format("[FollowUserPath:onStart] Entity '%s' starting to follow user path", entity_id))

        -- 解析路径点
        local waypoints = {}
        for x, y in string.gmatch(waypoints_str, "([%d%.%-]+),([%d%.%-]+)") do
            table.insert(waypoints, {x = tonumber(x), y = tonumber(y), z = 0})
        end

        if #waypoints == 0 then
            print("[FollowUserPath:onStart] ERROR: No valid waypoints")
            return "FAILURE"
        end

        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            print(string.format("[FollowUserPath:onStart] ERROR: Entity '%s' not found", entity_id))
            return "FAILURE"
        end

        -- 找到最近的路径点作为起点
        local currentWaypoint = 1
        local minDist = math.huge
        for i, wp in ipairs(waypoints) do
            local dist = math.sqrt((pos.x - wp.x)^2 + (pos.y - wp.y)^2)
            if dist < minDist then
                minDist = dist
                currentWaypoint = i
            end
        end

        -- 初始化路径状态
        local stateKey = entity_id .. "_userpath"
        moveStates[stateKey] = {
            waypoints = waypoints,
            currentIndex = currentWaypoint,
            threshold = 0.5
        }

        -- 设置初始方向
        local targetWp = waypoints[currentWaypoint]
        local dx = targetWp.x - pos.x
        local dy = targetWp.y - pos.y
        local dz = targetWp.z - pos.z
        sim.set_entity_move_direction(entity_id, dx, dy, dz)

        print(string.format("[FollowUserPath:onStart] Starting at waypoint %d/%d",
                            currentWaypoint, #waypoints))
        return "RUNNING"
    end,

    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_userpath"
        local state = moveStates[stateKey]

        if not state then
            return "FAILURE"
        end

        -- 获取当前目标路径点
        local targetWp = state.waypoints[state.currentIndex]

        -- 检查是否到达当前路径点
        local dist = sim.get_entity_distance(entity_id, targetWp.x, targetWp.y, targetWp.z)

        if dist <= state.threshold then
            print(string.format("[FollowUserPath:onRunning] Reached waypoint %d/%d",
                                state.currentIndex, #state.waypoints))

            -- 移动到下一个路径点
            state.currentIndex = state.currentIndex + 1

            -- 检查是否完成所有路径点
            if state.currentIndex > #state.waypoints then
                print(string.format("[FollowUserPath:onRunning] User path completed"))
                sim.set_entity_move_direction(entity_id, 0, 0, 0)
                moveStates[stateKey] = nil

                -- 清除用户路径
                bt.set_blackboard(entity_id, "user_path", nil)
                return "SUCCESS"
            end

            -- 更新方向指向下一个路径点
            targetWp = state.waypoints[state.currentIndex]
            local pos = sim.get_entity_position(entity_id)
            if pos then
                local dx = targetWp.x - pos.x
                local dy = targetWp.y - pos.y
                local dz = targetWp.z - pos.z
                sim.set_entity_move_direction(entity_id, dx, dy, dz)
            end
        end

        return "RUNNING"
    end,

    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        local stateKey = entity_id .. "_userpath"
        print(string.format("[FollowUserPath:onHalted] Path following halted for entity '%s'", entity_id))
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        moveStates[stateKey] = nil
    end
)
print("  [OK] FollowUserPath registered")

print("[BT Nodes Registry] All nodes registered successfully!")
print("")
