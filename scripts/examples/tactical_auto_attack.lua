-- 战术自动攻击脚本示例
-- 纯Lua战术规则：检测范围内敌人并自动打击

print("[TacticalAutoAttack] Script loaded")

function execute(entity_id, sim, bt)
    local sensor_range = 20
    local attack_range = 10
    
    -- 获取实体位置
    local pos = sim.get_entity_position(entity_id)
    if not pos then
        print("[TacticalAutoAttack] Failed to get position for entity: " .. entity_id)
        return
    end
    
    print(string.format("[TacticalAutoAttack] Entity %s at (%.1f, %.1f)", entity_id, pos.x, pos.y))
    
    -- 检测范围内敌人
    local entities = sim.get_all_entities()
    local enemy_found = false
    local nearest_enemy = nil
    local nearest_dist = sensor_range
    
    for _, entity in ipairs(entities) do
        -- 检查是否是敌人（类型为"enemy"或"threat"）
        if entity.id ~= tonumber(entity_id) and 
           (entity.type == "enemy" or entity.type == "threat") then
            
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            
            if dist <= sensor_range then
                enemy_found = true
                print(string.format("[TacticalAutoAttack] Enemy detected: vehicle=%d at distance %.1f", 
                                    entity.id, dist))
                
                -- 记录最近的敌人
                if dist < nearest_dist then
                    nearest_dist = dist
                    nearest_enemy = entity
                end
                
                if dist <= attack_range then
                    -- 在攻击范围内，执行打击
                    print(string.format("[TacticalAutoAttack] ATTACKING enemy vehicle=%d at distance %.1f!", 
                                        entity.id, dist))
                    -- 这里可以调用实际的攻击逻辑
                else
                    -- 敌人在传感器范围内但不在攻击范围内
                    print(string.format("[TacticalAutoAttack] Enemy vehicle=%d in sensor range (%.1f), moving closer...", 
                                        entity.id, dist))
                end
            end
        end
    end
    
    if not enemy_found then
        print("[TacticalAutoAttack] No enemies in sensor range")
    elseif nearest_enemy then
        print(string.format("[TacticalAutoAttack] Nearest enemy: vehicle=%d at distance %.1f", 
                            nearest_enemy.id, nearest_dist))
    end
end
