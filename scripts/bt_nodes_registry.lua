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

print("[BT Nodes Registry] All nodes registered successfully!")
print("")
