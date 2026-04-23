# VehicleID 集成计划文档

## 概述

将 `VehicleID` 结构体作为所有实体的唯一标识符集成到仿真接口中，替换现有的 `std::string` 类型实体ID。

## 当前状态分析

### 现有的 VehicleID 结构体
```cpp
struct SimAddress {
    int site;
    int host;
};

struct VehicleID {
    SimAddress address;
    int vehicle;
};
```

### 现有的 Entity 结构体
```cpp
struct Entity {
    VehicleID id;  // 已存在但未使用
    std::string type;
    double x;
    double y;
    double z;
    
    Entity() : x(0.0), y(0.0), z(0.0) {}
    Entity(const VehicleID& entityId, const std::string& entityType, double px, double py, double pz)
        : id(entityId), type(entityType), x(px), y(py), z(pz) {}
};
```

### 当前问题
- `Entity` 结构体已经有 `VehicleID id` 字段，但接口仍使用 `std::string` 作为实体ID
- `MockSimController` 使用 `std::map<std::string, Entity>` 存储实体
- 所有接口方法使用 `const std::string& entityId` 参数

---

## 修改计划

### 第一阶段：核心接口修改

#### 1. SimControlInterface.h
**文件**: `include/simulation/SimControlInterface.h`

**修改内容**:
- 为 `VehicleID` 添加比较运算符（用于 map 键值）
- 修改所有实体管理接口，将 `std::string` 替换为 `VehicleID`

```cpp
// 添加比较运算符
inline bool operator==(const VehicleID& lhs, const VehicleID& rhs) {
    return lhs.address.site == rhs.address.site && 
           lhs.address.host == rhs.address.host && 
           lhs.vehicle == rhs.vehicle;
}

inline bool operator<(const VehicleID& lhs, const VehicleID& rhs) {
    if (lhs.address.site != rhs.address.site) return lhs.address.site < rhs.address.site;
    if (lhs.address.host != rhs.address.host) return lhs.address.host < rhs.address.host;
    return lhs.vehicle < rhs.vehicle;
}

inline bool operator!=(const VehicleID& lhs, const VehicleID& rhs) {
    return !(lhs == rhs);
}
```

**接口变更**:
```cpp
// 修改前
virtual std::string addEntity(const std::string& type, double x, double y, double z) = 0;
virtual bool removeEntity(const std::string& entityId) = 0;
virtual bool moveEntity(const std::string& entityId, double x, double y, double z) = 0;
virtual bool getEntityPosition(const std::string& entityId, double& x, double& y, double& z) = 0;
virtual bool setEntityMoveDirection(const std::string& entityId, double dx, double dy, double dz) = 0;
virtual double getEntityDistance(const std::string& entityId, double x, double y, double z) = 0;

// 修改后
virtual VehicleID addEntity(const std::string& type, double x, double y, double z) = 0;
virtual bool removeEntity(const VehicleID& entityId) = 0;
virtual bool moveEntity(const VehicleID& entityId, double x, double y, double z) = 0;
virtual bool getEntityPosition(const VehicleID& entityId, double& x, double& y, double& z) = 0;
virtual bool setEntityMoveDirection(const VehicleID& entityId, double dx, double dy, double dz) = 0;
virtual double getEntityDistance(const VehicleID& entityId, double x, double y, double z) = 0;
```

---

#### 2. MockSimController.h
**文件**: `include/simulation/MockSimController.h`

**修改内容**:
- 修改实体存储类型
- 修改接口方法签名

```cpp
// 修改前
std::map<std::string, Entity> entities_;
std::atomic<uint64_t> nextEntityId_;
std::string generateEntityId();

// 修改后
std::map<VehicleID, Entity> entities_;
std::atomic<int> nextVehicleId_;
VehicleID generateVehicleId();
```

**方法签名变更**:
```cpp
// 修改前
std::string addEntity(const std::string& type, double x, double y, double z);
bool removeEntity(const std::string& entityId);
bool moveEntity(const std::string& entityId, double x, double y, double z);
bool getEntityPosition(const std::string& entityId, double& x, double& y, double& z);
bool setEntityMoveDirection(const std::string& entityId, double dx, double dy, double dz);
double getEntityDistance(const std::string& entityId, double x, double y, double z);

// 修改后
VehicleID addEntity(const std::string& type, double x, double y, double z);
bool removeEntity(const VehicleID& entityId);
bool moveEntity(const VehicleID& entityId, double x, double y, double z);
bool getEntityPosition(const VehicleID& entityId, double& x, double& y, double& z);
bool setEntityMoveDirection(const VehicleID& entityId, double dx, double dy, double dz);
double getEntityDistance(const VehicleID& entityId, double x, double y, double z);
```

---

#### 3. MockSimController.cpp
**文件**: `src/simulation/MockSimController.cpp`

**修改内容**:
- 修改构造函数初始化列表
- 实现新的 `generateVehicleId()` 方法
- 修改所有实体管理方法的实现

```cpp
// 构造函数修改
MockSimController::MockSimController()
    : state_(0)
    , simTime_(0.0)
    , timeScale_(1.0)
    , timeStep_(0.016)
    , autoUpdate_(false)
    , running_(false)
    , verbose_(true)
    , nextVehicleId_(1) {
}

// 新的 ID 生成方法
VehicleID MockSimController::generateVehicleId() {
    VehicleID id;
    id.address.site = 0;
    id.address.host = 0;
    id.vehicle = nextVehicleId_.fetch_add(1);
    return id;
}

// addEntity 修改
VehicleID MockSimController::addEntity(const std::string& type, double x, double y, double z) {
    VehicleID vehicleId = generateVehicleId();
    entities_[vehicleId] = Entity(vehicleId, type, x, y, z);
    
    if (verbose_) {
        std::cout << "[MockSim] Entity added: " << vehicleId.vehicle 
                  << " (type: " << type << ") at (" 
                  << x << ", " << y << ", " << z << ")" << std::endl;
    }
    
    return vehicleId;
}
```

---

### 第二阶段：Behavior Tree 节点修改

#### 4. BlackboardKeys.h
**文件**: `include/behaviortree/BlackboardKeys.h`

**修改内容**:
- 添加新的 blackboard key 类型支持

```cpp
// 添加 VehicleID 相关的 blackboard key（可选，用于类型安全）
constexpr const char* VEHICLE_ID = "vehicle_id";
```

---

#### 5. AsyncMoveToPoint.h
**文件**: `include/behaviortree/AsyncMoveToPoint.h`

**修改内容**:
```cpp
// 修改前
std::string entityId_;

// 修改后
simulation::VehicleID vehicleId_;
```

---

#### 6. AsyncMoveToPoint.cpp
**文件**: `src/behaviortree/AsyncMoveToPoint.cpp`

**修改内容**:
- 从 blackboard 获取 VehicleID 而不是 string
- 修改所有使用 entityId_ 的地方

```cpp
// onStart 修改
BT::NodeStatus AsyncMoveToPoint::onStart() {
    // ... 获取目标坐标 ...
    
    // 获取 VehicleID 从 blackboard
    auto blackboard = config().blackboard;
    if (!blackboard) {
        std::cerr << "[AsyncMoveToPoint] No blackboard available" << std::endl;
        return BT::NodeStatus::FAILURE;
    }

    try {
        vehicleId_ = blackboard->get<simulation::VehicleID>(BlackboardKeys::VEHICLE_ID);
    } catch (const std::exception& e) {
        std::cerr << "[AsyncMoveToPoint] Failed to get vehicle_id from blackboard: " << e.what() << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // ... 其余逻辑保持不变，使用 vehicleId_ ...
}
```

---

#### 7. CheckEntityExists.h/cpp
**文件**: `include/behaviortree/CheckEntityExists.h`, `src/behaviortree/CheckEntityExists.cpp`

**修改内容**:
- 修改端口类型从 `std::string` 到 `VehicleID`

```cpp
// providedPorts 修改
BT::PortsList CheckEntityExists::providedPorts() {
    return {
        BT::InputPort<simulation::VehicleID>("vehicle_id", "Vehicle ID to check")
    };
}

// tick 修改
BT::NodeStatus CheckEntityExists::tick() {
    auto vehicle_id = getInput<simulation::VehicleID>("vehicle_id");
    
    if (!vehicle_id) {
        std::cerr << "[CheckEntityExists] Missing required input: vehicle_id" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
    
    // ... 其余逻辑 ...
}
```

---

### 第三阶段：Lua 绑定修改

#### 8. LuaSimBinding.cpp
**文件**: `src/scripting/LuaSimBinding.cpp`

**修改内容**:
- 注册 VehicleID 类型到 Lua
- 修改所有实体相关的函数绑定

```cpp
void LuaSimBinding::registerSimAPI() {
    // 注册 VehicleID 类型
    luaState_->new_usertype<simulation::SimAddress>("SimAddress",
        "site", &simulation::SimAddress::site,
        "host", &simulation::SimAddress::host
    );
    
    luaState_->new_usertype<simulation::VehicleID>("VehicleID",
        "address", &simulation::VehicleID::address,
        "vehicle", &simulation::VehicleID::vehicle,
        "__eq", [](const simulation::VehicleID& a, const simulation::VehicleID& b) {
            return a == b;
        }
    );
    
    // 修改实体管理函数
    simTable.set_function("add_entity", [this](const std::string& type, double x, double y, double z) -> simulation::VehicleID {
        if (simInterface_) {
            return simInterface_->addEntity(type, x, y, z);
        }
        return simulation::VehicleID{};
    });
    
    simTable.set_function("remove_entity", [this](const simulation::VehicleID& vehicleId) -> bool {
        if (simInterface_) {
            return simInterface_->removeEntity(vehicleId);
        }
        return false;
    });
    
    // ... 其他函数类似修改 ...
    
    // get_all_entities 修改
    simTable.set_function("get_all_entities", [this]() -> sol::table {
        sol::table entities = luaState_->create_table();
        
        if (simInterface_) {
            auto entityList = simInterface_->getAllEntities();
            for (size_t i = 0; i < entityList.size(); ++i) {
                sol::table entity = luaState_->create_table();
                entity["id"] = entityList[i].id;  // VehicleID 对象
                entity["type"] = entityList[i].type;
                entity["x"] = entityList[i].x;
                entity["y"] = entityList[i].y;
                entity["z"] = entityList[i].z;
                entities[i + 1] = entity;
            }
        }
        
        return entities;
    });
}
```

---

### 第四阶段：XML 文件修改

#### 9. async_square_path.xml
**文件**: `bt_xml/async_square_path.xml`

**修改内容**:
- 修改 blackboard key 从 `entity_id` 到 `vehicle_id`

```xml
<!-- 修改前 -->
<CheckEntityExists entity_id="{entity_id}" />

<!-- 修改后 -->
<CheckEntityExists vehicle_id="{vehicle_id}" />
```

---

### 第五阶段：Lua 脚本修改

#### 10. example_sim_control.lua
**文件**: `scripts/example_sim_control.lua`

**修改内容**:
- 修改实体管理部分，使用 VehicleID 对象

```lua
-- 修改前
local npc1 = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Added NPC 1: " .. npc1)
sim.move_entity(npc1, 5.0, 5.0, 0.0)

-- 修改后
local npc1 = sim.add_entity("npc", 0.0, 0.0, 0.0)
print("   Added NPC 1: vehicle=" .. npc1.vehicle)
sim.move_entity(npc1, 5.0, 5.0, 0.0)

-- 访问 VehicleID 字段
print("   Vehicle ID: " .. npc1.vehicle)
print("   Site: " .. npc1.address.site)
print("   Host: " .. npc1.address.host)
```

---

## 文件修改清单

### C++ 头文件 (.h)
| 文件路径 | 修改类型 |
|---------|---------|
| `include/simulation/SimControlInterface.h` | 添加比较运算符，修改接口签名 |
| `include/simulation/MockSimController.h` | 修改成员变量和方法签名 |
| `include/behaviortree/BlackboardKeys.h` | 添加 VEHICLE_ID key |
| `include/behaviortree/AsyncMoveToPoint.h` | 修改成员变量类型 |
| `include/behaviortree/CheckEntityExists.h` | 修改端口类型（如需要） |

### C++ 源文件 (.cpp)
| 文件路径 | 修改类型 |
|---------|---------|
| `src/simulation/MockSimController.cpp` | 实现新的 ID 生成和实体管理 |
| `src/behaviortree/AsyncMoveToPoint.cpp` | 使用 VehicleID |
| `src/behaviortree/CheckEntityExists.cpp` | 使用 VehicleID |
| `src/scripting/LuaSimBinding.cpp` | 注册 VehicleID 类型，修改绑定 |

### XML 文件
| 文件路径 | 修改类型 |
|---------|---------|
| `bt_xml/async_square_path.xml` | 修改 blackboard key |
| `bt_xml/lua_custom_nodes_example.xml` | 检查并修改 |
| `bt_xml/lua_stateful_nodes.xml` | 检查并修改 |
| `bt_xml/lua_nodes_with_params.xml` | 检查并修改 |

### Lua 脚本
| 文件路径 | 修改类型 |
|---------|---------|
| `scripts/example_sim_control.lua` | 使用 VehicleID 对象 |
| `scripts/example_bt_advanced.lua` | 检查并修改 |
| `scripts/example_bt_lua_nodes.lua` | 检查并修改 |
| `scripts/example_bt_basic.lua` | 检查并修改 |
| `scripts/bt_nodes_registry.lua` | 检查并修改 |

---

## 实施步骤

1. **步骤 1**: 修改 `SimControlInterface.h` - 添加比较运算符和修改接口
2. **步骤 2**: 修改 `MockSimController.h/cpp` - 实现新的实体管理
3. **步骤 3**: 修改 Behavior Tree 节点 - 使用 VehicleID
4. **步骤 4**: 修改 Lua 绑定 - 注册 VehicleID 类型
5. **步骤 5**: 修改 XML 文件 - 更新 blackboard key
6. **步骤 6**: 修改 Lua 脚本 - 使用新的 API
7. **步骤 7**: 编译测试

---

## 注意事项

1. **向后兼容性**: 这是一个破坏性变更，所有使用实体ID的地方都需要修改
2. **序列化**: VehicleID 需要支持序列化（用于 XML/Lua 传递）
3. **调试输出**: 需要为 VehicleID 提供友好的字符串表示
4. **Blackboard 类型**: BehaviorTree.CPP 需要能够识别 VehicleID 类型

---

## 测试计划

1. 编译项目，确保无编译错误
2. 运行 example_sim_control.lua，验证实体管理功能
3. 加载 async_square_path.xml，验证 Behavior Tree 执行
4. 检查所有实体操作（添加、删除、移动、查询）
