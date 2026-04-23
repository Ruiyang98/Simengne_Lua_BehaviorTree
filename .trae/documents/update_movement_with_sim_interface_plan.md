# 使用仿真接口改写移动逻辑计划

## 新增接口说明

用户新增了两个仿真接口：

### 1. setEntityMoveDirection
```cpp
virtual bool setEntityMoveDirection(const std::string& entityId, double dx, double dy, double dz) = 0;
```
**功能**：设置实体的移动方向，让实体朝指定方向持续移动
**参数**：
- entityId: 实体ID
- dx, dy, dz: 移动方向向量（不需要归一化）

### 2. getEntityDistance
```cpp
virtual double getEntityDistance(const std::string& entityId, double x, double y, double z) = 0;
```
**功能**：获取实体到目标点的距离
**参数**：
- entityId: 实体ID
- x, y, z: 目标点坐标
**返回**：距离值

## 需要修改的文件

### 1. C++ 文件

#### AsyncMoveToPoint.cpp
**当前逻辑**：
- onStart: 获取目标位置，检查是否已到达
- onRunning: 计算新位置，调用 moveEntity 直接设置位置

**新逻辑**：
- onStart: 
  - 获取目标位置
  - 使用 getEntityDistance 检查距离
  - 如果未到达，使用 setEntityMoveDirection 设置移动方向
  - 返回 RUNNING
- onRunning:
  - 使用 getEntityDistance 检查是否到达
  - 如果到达，停止移动（调用 setEntityMoveDirection 设置 0,0,0）
  - 返回 SUCCESS
  - 如果未到达，继续返回 RUNNING（方向已在 onStart 设置）
- onHalted:
  - 停止移动（调用 setEntityMoveDirection 设置 0,0,0）

### 2. Lua 文件

#### bt_stateful_action_example.lua
**当前逻辑**：
- 使用 sim.get_entity_position 获取位置
- 计算新位置后调用 sim.move_entity 直接设置

**新逻辑**：
- onStart:
  - 获取当前位置
  - 使用 sim.get_entity_distance 检查距离
  - 计算方向向量
  - 调用 sim.set_entity_move_direction 设置移动方向
  - 返回 RUNNING
- onRunning:
  - 使用 sim.get_entity_distance 检查距离
  - 如果到达，调用 sim.set_entity_move_direction(0,0,0) 停止
  - 返回 SUCCESS
  - 如果未到达，返回 RUNNING
- onHalted:
  - 调用 sim.set_entity_move_direction(0,0,0) 停止

## 实施步骤

### 步骤 1: 更新 AsyncMoveToPoint.cpp
- [ ] 修改 onStart 方法，使用 setEntityMoveDirection 设置方向
- [ ] 修改 onRunning 方法，使用 getEntityDistance 检查距离
- [ ] 修改 onHalted 方法，停止移动
- [ ] 添加方向向量成员变量

### 步骤 2: 更新 Lua 绑定（如果需要）
- [ ] 检查是否需要添加 set_entity_move_direction 和 get_entity_distance 到 LuaSimBinding

### 步骤 3: 更新 bt_stateful_action_example.lua
- [ ] 修改 LuaStatefulMoveTo 使用新接口
- [ ] 修改 LuaStatefulPatrol 使用新接口

### 步骤 4: 验证编译和运行
- [ ] 编译项目
- [ ] 运行测试

## 关键代码示例

### C++ AsyncMoveToPoint 新实现

```cpp
BT::NodeStatus AsyncMoveToPoint::onStart() {
    // ... 获取参数和实体ID ...
    
    // 使用 getEntityDistance 检查距离
    double distance = simController->getEntityDistance(entityId_, targetX_, targetY_, targetZ_);
    
    if (distance <= arrivalThreshold_) {
        return BT::NodeStatus::SUCCESS;
    }
    
    // 计算方向向量
    double dx = targetX_ - currentX;
    double dy = targetY_ - currentY;
    double dz = targetZ_ - currentZ;
    
    // 设置移动方向
    simController->setEntityMoveDirection(entityId_, dx, dy, dz);
    
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus AsyncMoveToPoint::onRunning() {
    // 检查是否到达
    double distance = simController->getEntityDistance(entityId_, targetX_, targetY_, targetZ_);
    
    if (distance <= arrivalThreshold_) {
        // 停止移动
        simController->setEntityMoveDirection(entityId_, 0, 0, 0);
        return BT::NodeStatus::SUCCESS;
    }
    
    return BT::NodeStatus::RUNNING;
}

void AsyncMoveToPoint::onHalted() {
    simulation::SimControlInterface* simController = getSimController();
    if (simController) {
        simController->setEntityMoveDirection(entityId_, 0, 0, 0);
    }
}
```

### Lua 新实现

```lua
bt.register_stateful_action("LuaStatefulMoveTo",
    -- onStart
    function(params)
        local entity_id = params.entity_id or ""
        local target_x = tonumber(params.x) or 0
        local target_y = tonumber(params.y) or 0
        
        -- 获取当前位置
        local pos = sim.get_entity_position(entity_id)
        if not pos then
            return "FAILURE"
        end
        
        -- 使用 get_entity_distance 检查距离
        local dist = sim.get_entity_distance(entity_id, target_x, target_y, 0)
        if dist <= 0.5 then
            return "SUCCESS"
        end
        
        -- 计算方向并设置
        local dx = target_x - pos.x
        local dy = target_y - pos.y
        sim.set_entity_move_direction(entity_id, dx, dy, 0)
        
        -- 保存目标位置供 onRunning 使用
        moveStates[entity_id] = {
            targetX = target_x,
            targetY = target_y
        }
        
        return "RUNNING"
    end,
    
    -- onRunning
    function(params)
        local entity_id = params.entity_id or ""
        local state = moveStates[entity_id]
        
        if not state then
            return "FAILURE"
        end
        
        -- 检查距离
        local dist = sim.get_entity_distance(entity_id, state.targetX, state.targetY, 0)
        
        if dist <= 0.5 then
            -- 停止移动
            sim.set_entity_move_direction(entity_id, 0, 0, 0)
            moveStates[entity_id] = nil
            return "SUCCESS"
        end
        
        return "RUNNING"
    end,
    
    -- onHalted
    function(params)
        local entity_id = params.entity_id or ""
        sim.set_entity_move_direction(entity_id, 0, 0, 0)
        moveStates[entity_id] = nil
    end
)
```

## 优势

使用新接口的优势：
1. **更符合物理仿真**：setEntityMoveDirection 让仿真系统控制实际移动，而不是直接设置位置
2. **更简洁**：不需要在每次 tick 计算新位置
3. **更灵活**：仿真系统可以处理速度、碰撞等物理效果
4. **距离检查更方便**：getEntityDistance 直接提供距离查询
