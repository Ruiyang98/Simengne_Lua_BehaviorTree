-- Tactical auto-attack script example
-- Each script has its own state table for isolation
-- Use entity.vars for sharing data between scripts

print("[TacticalAutoAttack] Script loaded for entity: " .. entity.id)

function execute(state)
    -- state is the script's own state table
    -- Initialize state variables with defaults
    state.attack_count = state.attack_count or 0
    state.sensor_range = state.sensor_range or 20
    state.attack_range = state.attack_range or 10
    
    -- Get entity position using global sim table
    local pos = sim.get_entity_position(tonumber(entity.id))
    if not pos then
        print("[TacticalAutoAttack] Failed to get position for entity: " .. entity.id)
        return
    end
    
    print(string.format("[TacticalAutoAttack] Entity %s at (%.1f, %.1f), attack_count=%d", 
        entity.id, pos.x, pos.y, state.attack_count))
    
    -- Detect enemies in range
    local entities = sim.get_all_entities()
    local enemy_found = false
    local nearest_enemy = nil
    local nearest_dist = state.sensor_range
    
    for _, e in ipairs(entities) do
        -- Check if it's an enemy (type is "enemy" or "threat")
        if tostring(e.id) ~= entity.id and 
           (e.type == "enemy" or e.type == "threat") then
            
            local dist = math.sqrt((pos.x - e.x)^2 + (pos.y - e.y)^2)
            
            if dist <= state.sensor_range then
                enemy_found = true
                print(string.format("[TacticalAutoAttack] Enemy detected: vehicle=%s at distance %.1f", 
                                    tostring(e.id), dist))
                
                -- Track nearest enemy
                if dist < nearest_dist then
                    nearest_dist = dist
                    nearest_enemy = e
                end
                
                if dist <= state.attack_range then
                    -- Within attack range, execute attack
                    print(string.format("[TacticalAutoAttack] ATTACKING enemy vehicle=%s at distance %.1f!", 
                                        tostring(e.id), dist))
                    -- Increment attack counter in script's own state
                    state.attack_count = state.attack_count + 1
                    
                    -- Share data with other scripts via entity.vars
                    entity.set_var("last_target", e.id)
                    entity.set_var("last_attack_time", sim.get_time())
                else
                    -- Enemy in sensor range but not attack range
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
    
    -- Store script-specific state
    state.last_check_time = sim.get_time()
    state.last_position = {x = pos.x, y = pos.y, z = pos.z}
end
