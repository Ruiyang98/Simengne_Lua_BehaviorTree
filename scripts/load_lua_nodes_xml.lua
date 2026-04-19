-- 单独加载 lua_custom_nodes_example.xml 的示例
-- 注意：需要先注册 Lua 节点，然后才能执行

print("========================================")
print("    单独加载 Lua Nodes XML 示例")
print("========================================")
print("")

-- 第 1 步：注册 Lua 节点（必须先注册！）
print("1. 注册 Lua 节点...")

bt.register_condition("LuaHasEntities", function()
    return sim.get_entity_count() > 0
end)

bt.register_condition("LuaHasNPC", function()
    local entities = sim.get_all_entities()
    for _, entity in ipairs(entities) do
        if entity.type == "npc" then return true end
    end
    return false
end)

bt.register_action("LuaCheckHealth", function()
    local count = sim.get_entity_count()
    print("   [LuaCheckHealth] Entities: " .. count)
    return count > 0 and "SUCCESS" or "FAILURE"
end)

bt.register_action("LuaPatrol", function()
    print("   [LuaPatrol] Patrolling...")
    -- 简单的巡逻逻辑
    return "SUCCESS"
end)

print("   OK: 4 个 Lua 节点已注册")
print("")

-- 第 2 步：加载 XML 文件
print("2. 加载 XML 文件...")
if bt.load_file("bt_xml/lua_custom_nodes_example.xml") then
    print("   OK: lua_custom_nodes_example.xml 加载成功")
else
    print("   ERROR: " .. bt.get_last_error())
    return
end
print("")

-- 第 3 步：创建测试实体
print("3. 创建测试实体...")
local npc_id = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Created NPC: " .. npc_id)
print("")

-- 第 4 步：执行不同的行为树
print("4. 执行 LuaTestTree...")
local tree1 = bt.execute("LuaTestTree")
print("   Status: " .. bt.get_status(tree1))
print("")

print("5. 执行 LuaTestTreeSimple...")
local tree2 = bt.execute("LuaTestTreeSimple")
print("   Status: " .. bt.get_status(tree2))
print("")

print("6. 执行 LuaMixedTree（混合 C++ 和 Lua 节点）...")
local tree3 = bt.execute("LuaMixedTree", npc_id)
print("   Status: " .. bt.get_status(tree3))
print("")

-- 清理
sim.remove_entity(npc_id)

print("========================================")
print("    完成！")
print("========================================")
