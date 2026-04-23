-- 脚本管理器使用示例
-- 演示如何使用 EntityScriptManager 和 entity.vars API

print("========================================")
print("    Script Manager Demo")
print("========================================")
print("")

-- 创建一个实体用于测试
local entity_id = "1"  -- 使用 vehicle ID 作为实体 ID

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

-- 添加纯 Lua 战术脚本
print("----------------------------------------")
print("Adding tactical script...")
print("----------------------------------------")

local tactical_script = [[
function execute()
    -- 使用 entity.vars 存储和获取变量
    local sensor_range = entity.get_var("sensor_range", 20)
    local attack_range = entity.get_var("attack_range", 10)
    local check_count = entity.get_var("check_count", 0)
    
    -- 增加检查计数
    check_count = check_count + 1
    entity.set_var("check_count", check_count)
    
    -- 获取实体位置
    local pos = sim.get_entity_position(tonumber(entity.id))
    if not pos then return end
    
    print(string.format("[Tactical] Entity %s at (%.1f, %.1f), check_count=%d", 
        entity.id, pos.x, pos.y, check_count))
    
    -- 检测敌人
    local entities = sim.get_all_entities()
    for _, e in ipairs(entities) do
        if tostring(e.id) ~= entity.id and e.type == "enemy" then
            local dist = math.sqrt((pos.x - e.x)^2 + (pos.y - e.y)^2)
            if dist <= sensor_range then
                if dist <= attack_range then
                    print(string.format("[Tactical] ATTACKING enemy %s! (dist=%.1f)", 
                        tostring(e.id), dist))
                else
                    print(string.format("[Tactical] Enemy %s detected (dist=%.1f)", 
                        tostring(e.id), dist))
                end
            end
        end
    end
    
    -- 演示其他变量操作
    entity.set_var("last_check_time", sim.get_time())
    
    -- 检查变量是否存在
    if entity.has_var("sensor_range") then
        print(string.format("[Tactical] Sensor range is set to: %.1f", 
            entity.get_var("sensor_range")))
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
print("Setting entity variables...")
print("----------------------------------------")

-- 通过脚本管理器获取 entity 表并设置变量
-- 注意：这里演示如何在 Lua 中操作 entity.vars
print("Variables can be set from within the script using entity.set_var()")
print("Or from C++ using manager->getVariables()")

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
print("Key features demonstrated:")
print("  - entity.id: Get entity ID")
print("  - entity.get_var(key, default): Get variable with default value")
print("  - entity.set_var(key, value): Set variable")
print("  - entity.has_var(key): Check if variable exists")
print("  - entity.remove_var(key): Remove variable")
print("  - entity.clear_vars(): Clear all variables")
print("")
print("To test:")
print("  1. Start simulation: sim.start()")
print("  2. Wait for script execution (every 500ms)")
print("  3. Check output for tactical detection messages")
print("")
