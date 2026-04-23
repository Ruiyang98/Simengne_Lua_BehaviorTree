-- Lua+行为树混合脚本示例
-- 每次执行时先执行Lua逻辑更新blackboard，然后tick行为树

print("[BTHybridPatrol] Script loaded")

-- 传感器范围
local SENSOR_RANGE = 15

function execute(entity_id, sim, bt)
    print(string.format("[BTHybridPatrol] Executing for entity %s", entity_id))
    
    -- 1. Lua逻辑：更新blackboard数据
    local pos = sim.get_entity_position(entity_id)
    if pos then
        -- 设置当前位置到blackboard
        bt.set_blackboard(entity_id, "current_x", pos.x)
        bt.set_blackboard(entity_id, "current_y", pos.y)
        print(string.format("[BTHybridPatrol] Updated position: (%.1f, %.1f)", pos.x, pos.y))
    else
        print("[BTHybridPatrol] Failed to get position")
    end
    
    -- 2. 检测威胁并更新blackboard
    local has_threat, threat_info = check_for_threats(entity_id, sim, bt)
    bt.set_blackboard(entity_id, "has_threat", has_threat)
    
    if has_threat and threat_info then
        bt.set_blackboard(entity_id, "threat_id", threat_info.id)
        bt.set_blackboard(entity_id, "threat_x", threat_info.x)
        bt.set_blackboard(entity_id, "threat_y", threat_info.y)
        bt.set_blackboard(entity_id, "threat_dist", threat_info.dist)
        print(string.format("[BTHybridPatrol] Threat detected: vehicle=%d at (%.1f, %.1f), dist=%.1f", 
                            threat_info.id, threat_info.x, threat_info.y, threat_info.dist))
    else
        print("[BTHybridPatrol] No threats detected")
    end
    
    -- 3. 设置巡逻路径点（如果没有设置）
    local waypoints = bt.get_blackboard(entity_id, "waypoints")
    if not waypoints then
        -- 默认巡逻路径：正方形
        waypoints = "0,0;10,0;10,10;0,10"
        bt.set_blackboard(entity_id, "waypoints", waypoints)
        print("[BTHybridPatrol] Set default patrol waypoints: " .. waypoints)
    end
    
    print(string.format("[BTHybridPatrol] Blackboard updated: has_threat=%s", tostring(has_threat)))
end

-- 检测威胁的辅助函数
function check_for_threats(entity_id, sim, bt)
    local pos = sim.get_entity_position(entity_id)
    if not pos then 
        return false, nil 
    end
    
    local entities = sim.get_all_entities()
    local nearest_threat = nil
    local min_dist = SENSOR_RANGE
    
    for _, entity in ipairs(entities) do
        -- 检查是否是威胁
        if entity.id ~= tonumber(entity_id) and 
           (entity.type == "enemy" or entity.type == "threat") then
            
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            
            if dist <= SENSOR_RANGE and dist < min_dist then
                min_dist = dist
                nearest_threat = {
                    id = entity.id,
                    x = entity.x,
                    y = entity.y,
                    dist = dist
                }
            end
        end
    end
    
    if nearest_threat then
        return true, nearest_threat
    end
    
    return false, nil
end
