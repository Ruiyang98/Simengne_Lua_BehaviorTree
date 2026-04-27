# 避障功能实现计划

## 需求分析

用户需要一个避障系统，逻辑如下：
1. **判断是否有障碍物** - 条件检查
2. **有障碍物时** - 朝90度方向规避一段距离
3. **无障碍物时** - 执行用户输入的路线移动
4. **无用户路线时** - 什么都不做（Idle）

## 实现方案

### 方案：使用Behavior Tree + Lua自定义节点

利用现有的LuaBehaviorTreeBridge机制，注册自定义Lua节点，通过XML行为树组织逻辑。

## 具体实现步骤

### 步骤1：创建避障Lua节点注册文件

**文件**: `scripts/obstacle_avoidance_nodes.lua`

需要注册的节点：

1. **HasObstacle** (Condition) - 检查前方是否有障碍物
   - 参数: entity_id, check_distance
   - 返回: true/false
   - 实现: 获取实体位置，检查前方一定距离内是否有其他实体

2. **HasUserPath** (Condition) - 检查是否有用户输入路线
   - 参数: entity_id
   - 返回: true/false
   - 实现: 检查blackboard中是否有user_path字段

3. **AvoidObstacle** (StatefulAction) - 朝90度方向规避
   - 参数: entity_id, avoid_distance
   - 实现: 
     - onStart: 计算垂直方向（90度），设置移动方向
     - onRunning: 检查是否移动了指定距离，到达后停止
     - onHalted: 停止移动

4. **FollowUserPath** (StatefulAction) - 执行用户路线
   - 参数: entity_id
   - 实现:
     - onStart: 从blackboard读取路径点，开始移动
     - onRunning: 按路径点移动，到达后返回SUCCESS
     - onHalted: 停止移动

### 步骤2：创建避障行为树XML

**文件**: `bt_xml/obstacle_avoidance.xml`

行为树结构（Fallback + Sequence组合）：

```xml
<BehaviorTree ID="ObstacleAvoidanceTree">
    <Fallback name="main_fallback">
        <!-- 优先级1: 如果有障碍物，执行规避 -->
        <Sequence name="avoid_obstacle_sequence">
            <LuaCondition lua_node_name="HasObstacle" 
                          entity_id="{entity_id}" 
                          check_distance="5.0"/>
            <LuaStatefulAction lua_node_name="AvoidObstacle" 
                               entity_id="{entity_id}" 
                               avoid_distance="3.0"/>
        </Sequence>
        
        <!-- 优先级2: 如果有用户路线，执行路线 -->
        <Sequence name="follow_path_sequence">
            <LuaCondition lua_node_name="HasUserPath" 
                          entity_id="{entity_id}"/>
            <LuaStatefulAction lua_node_name="FollowUserPath" 
                               entity_id="{entity_id}"/>
        </Sequence>
        
        <!-- 优先级3: 什么都没有，Idle -->
        <LuaAction lua_node_name="Idle" 
                   entity_id="{entity_id}"/>
    </Fallback>
</BehaviorTree>
```

### 步骤3：创建C++示例程序

**文件**: `examples/obstacle_avoidance_example.cpp`

演示如何：
1. 加载避障行为树
2. 创建实体
3. 设置用户路径（通过blackboard）
4. 执行行为树
5. 动态添加障碍物测试避障功能

## 关键设计决策

1. **使用StatefulAction**: 移动类动作需要持续多帧，使用有状态动作节点
2. **Fallback结构**: 优先级从高到低：避障 > 执行路线 > Idle
3. **Blackboard传参**: 用户路径通过blackboard传递，便于动态修改
4. **90度规避**: 计算当前移动方向的垂直向量作为规避方向

## 使用方式

### C++端设置用户路径

```cpp
// 获取行为树黑板
auto blackboard = BT::Blackboard::create();
blackboard->set("entity_id", entityId);

// 设置用户路径（格式: "x1,y1;x2,y2;x3,y3"）
blackboard->set("user_path", "0,0;10,0;10,10;0,10");

// 创建并执行行为树
auto tree = factory.createTree("ObstacleAvoidanceTree", blackboard);
```

### Lua节点中读取路径

```lua
-- 在FollowUserPath节点中
local waypoints_str = bt.get_blackboard(entity_id, "user_path")
-- 解析路径点并执行
```

## 文件清单

1. `scripts/obstacle_avoidance_nodes.lua` - Lua节点注册
2. `bt_xml/obstacle_avoidance.xml` - 行为树定义
3. `examples/obstacle_avoidance_example.cpp` - C++示例程序
