-- 脚本管理器使用示例
-- 演示如何使用EntityScriptManager

print("========================================")
print("    Script Manager Demo")
print("========================================")
print("")

-- 创建一个实体用于测试
local entity_id = "1"  -- 使用vehicle ID作为实体ID

-- 检查实体是否存在，如果不存在则创建一个
if sim.get_entity_count() == 0 then
    print("Creating test entity...")
    local vid = sim.add_entity("guard", 0, 0, 0)
    entity_id = tostring(vid.vehicle)
    print(string.format("Created entity with vehicle ID: %s", entity_id))
else
    -- 使用第一个实体
    local entities = sim.get_all_entities()
    entity_id = tostring(entities[1].id.vehicle)
    print(string.format("Using existing entity with vehicle ID: %s", entity_id))
end

-- 创建一个敌人实体用于测试
print("Creating enemy entity...")
local enemy_vid = sim.add_entity("enemy", 5, 5, 0)
print(string.format("Created enemy with vehicle ID: %d", enemy_vid.vehicle))

print("")
print("----------------------------------------")
print("Creating script manager for entity " .. entity_id)
print("----------------------------------------")

-- 创建脚本管理器
local manager = sim.create_script_manager(entity_id)
if not manager then
    print("ERROR: Failed to create script manager")
    return
end

print("Script manager created successfully!")
print("")

-- 添加纯Lua战术脚本
print("----------------------------------------")
print("Adding tactical script...")
print("----------------------------------------")

local tactical_script = [[
function execute(entity_id, sim, bt)
    local sensor_range = 20
    local attack_range = 10
    
    local pos = sim.get_entity_position(entity_id)
    if not pos then return end
    
    print(string.format("[Tactical] Entity %s at (%.1f, %.1f)", entity_id, pos.x, pos.y))
    
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.id ~= tonumber(entity_id) and entity.type == "enemy" then
            local dist = math.sqrt((pos.x - entity.x)^2 + (pos.y - entity.y)^2)
            if dist <= sensor_range then
                if dist <= attack_range then
                    print(string.format("[Tactical] ATTACKING enemy %d! (dist=%.1f)", entity.id, dist))
                else
                    print(string.format("[Tactical] Enemy %d detected (dist=%.1f)", entity.id, dist))
                end
            end
        end
    end
end
]]

local success = manager:add_tactical_script("auto_attack", tactical_script)
if success then
    print("Tactical script 'auto_attack' added successfully!")
else
    print("ERROR: Failed to add tactical script")
end

print("")
print("----------------------------------------")
print("Script list:")
print("----------------------------------------")

local scripts = manager:get_scripts()
for i, name in ipairs(scripts) do
    print(string.format("  %d. %s", i, name))
end

print("")
print("----------------------------------------")
print("Script count: " .. manager:get_script_count())
print("Has 'auto_attack' script: " .. tostring(manager:has_script("auto_attack")))
print("----------------------------------------")

print("")
print("========================================")
print("    Demo completed!")
print("========================================")
print("")
print("The script manager will automatically execute")
print("all scripts every 500ms when simulation is running.")
print("")
print("To test:")
print("  1. Start simulation: sim.start()")
print("  2. Wait for script execution (every 500ms)")
print("  3. Check output for tactical detection messages")
print("")
