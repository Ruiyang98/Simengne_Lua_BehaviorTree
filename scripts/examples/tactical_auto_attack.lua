-- 战术自动攻击脚本示例
-- 使用 entity.vars 存储变量，实现实体间变量隔离

print("[TacticalAutoAttack] Script loaded for entity: " .. entity.id)

function execute()
    -- 从 entity.vars 获取变量，使用默认值
    local attack_count = entity.get_var("attack_count", 0)
    local sensor_range = entity.get_var("sensor_range", 20)
    local attack_range = entity.get_var("attack_range", 10)
    
    -- 获取实体位置
    local pos = sim.get_entity_position(tonumber(entity.id))
    if not pos then
        print("[TacticalAutoAttack] Failed to get position for entity: " .. entity.id)
        return
    end
    
    print(string.format("[TacticalAutoAttack] Entity %s at (%.1f, %.1f), attack_count=%d", 
        entity.id, pos.x, pos.y, attack_count))
    
    -- 检测范围内敌人
    local entities = sim.get_all_entities()
    local enemy_found = false
    local nearest_enemy = nil
    local nearest_dist = sensor_range
    
    for _, e in ipairs(entities) do
        -- 检查是否是敌人（类型为"enemy"或"threat"）
        if tostring(e.id) ~= entity.id and 
           (e.type == "enemy" or e.type == "threat") then
            
            local dist = math.sqrt((pos.x - e.x)^2 + (pos.y - e.y)^2)
            
            if dist <= sensor_range then
                enemy_found = true
                print(string.format("[TacticalAutoAttack] Enemy detected: vehicle=%s at distance %.1f", 
                                    tostring(e.id), dist))
                
                -- 记录最近的敌人
                if dist < nearest_dist then
                    nearest_dist = dist
                    nearest_enemy = e
                end
                
                if dist <= attack_range then
                    -- 在攻击范围内，执行打击
                    print(string.format("[TacticalAutoAttack] ATTACKING enemy vehicle=%s at distance %.1f!", 
                                        tostring(e.id), dist))
                    -- 增加攻击计数
                    attack_count = attack_count + 1
                    entity.set_var("attack_count", attack_count)
                    entity.set_var("last_target", e.id)
                else
                    -- 敌人在传感器范围内但不在攻击范围内
                    print(string.format("[TacticalAutoAttack] Enemy vehicle=%s in sensor range (%.1f), moving closer...", 
                                        tostring(e.id), dist))
                end
            end
        end
    end
    
    if not enemy_found then
        print("[TacticalAutoAttack] No enemies in sensor range")
    elseif nearest_enemy then
        print(string.format("[TacticalAutoAttack] Nearest enemy: vehicle=%s at distance %.1f", 
                            tostring(nearest_enemy.id), nearest_dist))
    end
    
    -- 演示其他变量操作方法
    entity.set_var("last_check_time", sim.get_time())
    entity.set_var("last_position", {x = pos.x, y = pos.y, z = pos.z})
end
