-- 动态路径点巡逻脚本示例
-- 演示如何从C++端动态修改路径点参数
-- 
-- C++端使用方式：
--   manager:setScriptWaypoints("patrol", {{0,0,0}, {10,0,0}, {10,10,0}})
--   manager:setScriptParam("patrol", "patrol_speed", 2.5)
--   manager:setEntityField("alert_level", 5)

print("[PatrolDynamic] Script loaded for entity: " .. entity.id)

function execute(state)
    -- 初始化状态变量
    state.current_index = state.current_index or 1
    state.status = state.status or "idle"
    state.patrol_count = state.patrol_count or 0
    
    -- 从state读取路径点（C++通过setScriptWaypoints设置）
    local waypoints = state.waypoints
    if not waypoints or #waypoints == 0 then
        if state.status ~= "waiting" then
            print("[PatrolDynamic] No waypoints set, waiting for C++ to set waypoints...")
            state.status = "waiting"
        end
        return
    end
    
    -- 从state读取其他参数（C++通过setScriptParam设置）
    local patrol_speed = state.patrol_speed or 1.0
    local loop_mode = state.loop_mode or "cycle"  -- "cycle" 或 "pingpong"
    
    -- 从entity表读取共享数据（C++通过setEntityField设置，所有脚本可见）
    local alert_level = entity.alert_level or 0
    
    -- 获取当前位置
    local pos = sim.get_entity_position(tonumber(entity.id))
    if not pos then
        print("[PatrolDynamic] Failed to get position")
        return
    end
    
    -- 确保索引在有效范围内
    if state.current_index < 1 then
        state.current_index = 1
    elseif state.current_index > #waypoints then
        if loop_mode == "cycle" then
            state.current_index = 1
            state.patrol_count = state.patrol_count + 1
            print(string.format("[PatrolDynamic] Completed patrol loop #%d", state.patrol_count))
        else
            state.current_index = #waypoints
        end
    end
    
    -- 获取当前目标点
    local target = waypoints[state.current_index]
    if not target then
        print("[PatrolDynamic] Invalid waypoint index: " .. tostring(state.current_index))
        state.current_index = 1
        return
    end
    
    -- 计算距离
    local dx = target.x - pos.x
    local dy = target.y - pos.y
    local dist = math.sqrt(dx*dx + dy*dy)
    
    -- 根据警戒等级调整速度
    local speed = patrol_speed * (1 + alert_level * 0.2)
    
    -- 调试输出（每30帧输出一次，避免刷屏）
    state.debug_counter = (state.debug_counter or 0) + 1
    if state.debug_counter % 30 == 1 then
        print(string.format("[PatrolDynamic] Moving to waypoint %d/%d: (%.1f, %.1f), dist=%.2f, speed=%.1f, alert=%d",
            state.current_index, #waypoints, target.x, target.y, dist, speed, alert_level))
    end
    
    -- 到达判定阈值
    local arrival_threshold = 0.5
    
    if dist < arrival_threshold then
        -- 到达目标点
        print(string.format("[PatrolDynamic] Reached waypoint %d/%d: (%.1f, %.1f)",
            state.current_index, #waypoints, target.x, target.y))
        
        -- 移动到下一个路径点
        state.current_index = state.current_index + 1
        state.status = "moving"
        
        -- 如果完成一轮巡逻
        if state.current_index > #waypoints then
            if loop_mode == "cycle" then
                state.current_index = 1
                state.patrol_count = state.patrol_count + 1
                print(string.format("[PatrolDynamic] Starting patrol loop #%d", state.patrol_count + 1))
            else
                state.current_index = #waypoints
                print("[PatrolDynamic] Reached final waypoint")
            end
        end
    else
        -- 继续向目标移动
        local len = math.sqrt(dx*dx + dy*dy)
        if len > 0 then
            -- 归一化方向向量并应用速度
            local move_x = (dx / len) * speed
            local move_y = (dy / len) * speed
            sim.set_entity_move_direction(tonumber(entity.id), move_x, move_y, 0)
        end
        state.status = "moving"
    end
    
    -- 将当前状态写回state，C++可以通过getScriptParam读取
    state.current_x = pos.x
    state.current_y = pos.y
    state.distance_to_target = dist
end
