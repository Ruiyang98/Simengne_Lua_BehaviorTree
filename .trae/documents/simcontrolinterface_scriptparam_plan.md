# SimControlInterface 暴露 ScriptParam 接口设计方案

## 问题背景

当前 `EntityScriptManager` 提供了修改 script parameters 的接口，但这些接口依赖 `sol::object`（sol2 Lua 绑定库的类型）：

```cpp
void setScriptParam(const std::string& scriptName, const std::string& key, sol::object value);
sol::optional<sol::object> getScriptParam(const std::string& scriptName, const std::string& key);
```

这导致任何想使用这些接口的外部代码都必须引入 sol 头文件，造成不必要的依赖耦合。

## 设计目标

在不引入 sol 头文件的前提下，向外部暴露修改 script parameters 的接口。

## 解决方案

### 方案：使用 std::variant 封装参数类型（推荐）

利用 C++17 的 `std::variant` 来封装 Lua 参数类型，避免直接暴露 sol 类型。

#### 1. 定义参数类型（在独立头文件中）

创建 `ScriptParamTypes.h`，不依赖 sol：

```cpp
#ifndef SCRIPT_PARAM_TYPES_H
#define SCRIPT_PARAM_TYPES_H

#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace scripting {

// 支持的参数类型（C++原生类型）
using ScriptParam = std::variant<
    bool,
    int,
    double,
    std::string,
    std::vector<double>,  // 用于位置、方向等
    std::vector<std::tuple<double, double, double>>  // 用于路径点
>;

// 路径点结构（可选，更清晰的语义）
struct Waypoint {
    double x, y, z;
    Waypoint(double x=0, double y=0, double z=0) : x(x), y(y), z(z) {}
};

} // namespace scripting

#endif
```

#### 2. 修改 EntityScriptManager

在 `EntityScriptManager.h` 中：

* 前向声明 `sol::object`

* 提供两个重载版本：

  * `setScriptParam()` - 接受 `ScriptParam`（variant）

  * 内部转换为 `sol::object`

```cpp
// EntityScriptManager.h
#include "scripting/ScriptParamTypes.h"

// 前向声明 sol 类型（不需要包含 sol.hpp）
namespace sol {
    class object;
    class state;
    class table;
    template<typename T> struct optional;
}

class EntityScriptManager {
public:
    // ... 现有接口 ...
    
    // ========== 不依赖 sol 的 C++ 接口 ==========
    
    // 设置脚本参数（使用 variant，不暴露 sol）
    void setScriptParam(const std::string& scriptName, 
                        const std::string& key, 
                        const ScriptParam& value);
    
    // 获取脚本参数（返回 optional<variant>）
    std::optional<ScriptParam> getScriptParam(
        const std::string& scriptName, 
        const std::string& key);
    
    // 设置路径点（专用接口）
    void setScriptWaypoints(const std::string& scriptName,
                            const std::vector<Waypoint>& waypoints);
    
    // 获取路径点
    std::vector<Waypoint> getScriptWaypoints(const std::string& scriptName);
    
    // 设置实体字段
    void setEntityField(const std::string& key, const ScriptParam& value);
    std::optional<ScriptParam> getEntityField(const std::string& key);
    
    // 批量设置参数（用于一次性设置多个参数）
    void setScriptParams(const std::string& scriptName,
                         const std::unordered_map<std::string, ScriptParam>& params);
    
private:
    // 内部转换函数（在 .cpp 中实现，可以包含 sol.hpp）
    sol::object paramToSolObject(const ScriptParam& param);
    std::optional<ScriptParam> solObjectToParam(const sol::object& obj);
    
    // ... 现有成员 ...
};
```

#### 3. 在 SimControlInterface 中暴露接口

在 `SimControlInterface.h` 中：

```cpp
#include "scripting/ScriptParamTypes.h"

class SimControlInterface {
public:
    // ... 现有接口 ...
    
    // ========== Script Parameter 管理接口 ==========
    
    // 设置指定实体脚本的参数
    virtual bool setScriptParam(const VehicleID& entityId,
                                const std::string& scriptName,
                                const std::string& key,
                                const scripting::ScriptParam& value) = 0;
    
    // 获取指定实体脚本的参数
    virtual std::optional<scripting::ScriptParam> getScriptParam(
                                const VehicleID& entityId,
                                const std::string& scriptName,
                                const std::string& key) = 0;
    
    // 设置脚本路径点
    virtual bool setScriptWaypoints(const VehicleID& entityId,
                                    const std::string& scriptName,
                                    const std::vector<scripting::Waypoint>& waypoints) = 0;
    
    // 获取脚本路径点
    virtual std::vector<scripting::Waypoint> getScriptWaypoints(
                                    const VehicleID& entityId,
                                    const std::string& scriptName) = 0;
    
    // 批量设置脚本参数
    virtual bool setScriptParams(const VehicleID& entityId,
                                 const std::string& scriptName,
                                 const std::unordered_map<std::string, scripting::ScriptParam>& params) = 0;
    
    // 获取指定实体的所有脚本名称
    virtual std::vector<std::string> getEntityScriptNames(const VehicleID& entityId) = 0;
};
```

#### 4. 在 MockSimController 中实现

```cpp
// MockSimController.h
class MockSimController : public SimControlInterface {
public:
    // ... 现有接口 ...
    
    // Script Parameter 接口实现
    bool setScriptParam(const VehicleID& entityId,
                        const std::string& scriptName,
                        const std::string& key,
                        const scripting::ScriptParam& value) override;
    
    std::optional<scripting::ScriptParam> getScriptParam(
                        const VehicleID& entityId,
                        const std::string& scriptName,
                        const std::string& key) override;
    
    bool setScriptWaypoints(const VehicleID& entityId,
                            const std::string& scriptName,
                            const std::vector<scripting::Waypoint>& waypoints) override;
    
    std::vector<scripting::Waypoint> getScriptWaypoints(
                            const VehicleID& entityId,
                            const std::string& scriptName) override;
    
    bool setScriptParams(const VehicleID& entityId,
                         const std::string& scriptName,
                         const std::unordered_map<std::string, scripting::ScriptParam>& params) override;
    
    std::vector<std::string> getEntityScriptNames(const VehicleID& entityId) override;
};
```

#### 5. 实现细节（EntityScriptManager.cpp）

```cpp
// 内部转换：ScriptParam -> sol::object
sol::object EntityScriptManager::paramToSolObject(const ScriptParam& param) {
    return std::visit([this](auto&& arg) -> sol::object {
        using T = std::decay_t<decltype(arg)>;
        
        if constexpr (std::is_same_v<T, bool>) {
            return sol::make_object(*luaState_, arg);
        } else if constexpr (std::is_same_v<T, int>) {
            return sol::make_object(*luaState_, arg);
        } else if constexpr (std::is_same_v<T, double>) {
            return sol::make_object(*luaState_, arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return sol::make_object(*luaState_, arg);
        } else if constexpr (std::is_same_v<T, std::vector<double>>) {
            sol::table tbl = luaState_->create_table();
            for (size_t i = 0; i < arg.size(); ++i) {
                tbl[i + 1] = arg[i];  // Lua 1-indexed
            }
            return tbl;
        } else if constexpr (std::is_same_v<T, std::vector<std::tuple<double, double, double>>>) {
            sol::table tbl = luaState_->create_table();
            for (size_t i = 0; i < arg.size(); ++i) {
                sol::table point = luaState_->create_table();
                point["x"] = std::get<0>(arg[i]);
                point["y"] = std::get<1>(arg[i]);
                point["z"] = std::get<2>(arg[i]);
                tbl[i + 1] = point;
            }
            return tbl;
        } else if constexpr (std::is_same_v<T, std::vector<Waypoint>>) {
            sol::table tbl = luaState_->create_table();
            for (size_t i = 0; i < arg.size(); ++i) {
                sol::table point = luaState_->create_table();
                point["x"] = arg[i].x;
                point["y"] = arg[i].y;
                point["z"] = arg[i].z;
                tbl[i + 1] = point;
            }
            return tbl;
        }
        return sol::nil;
    }, param);
}

// 设置参数（不依赖 sol 的公开接口）
void EntityScriptManager::setScriptParam(const std::string& scriptName, 
                                          const std::string& key, 
                                          const ScriptParam& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = scriptStates_.find(scriptName);
    if (it != scriptStates_.end()) {
        it->second[key] = paramToSolObject(value);
    }
}
```

## 实施步骤

1. **创建** **`ScriptParamTypes.h`**

   * 定义 `ScriptParam` variant 类型

   * 定义 `Waypoint` 结构体

   * 确保不依赖 sol

2. **修改** **`EntityScriptManager.h`**

   * 前向声明 sol 类型

   * 添加基于 `ScriptParam` 的新接口

   * 保留现有 sol 接口（向后兼容）

3. **修改** **`EntityScriptManager.cpp`**

   * 实现 `paramToSolObject()` 转换函数

   * 实现新的 `setScriptParam()` 等接口

4. **修改** **`SimControlInterface.h`**

   * 添加 script parameter 管理纯虚接口

   * 包含 `ScriptParamTypes.h`

5. **修改** **`MockSimController.h/cpp`**

   * 实现新的纯虚接口

   * 委托给对应的 `EntityScriptManager`

## 优势

1. **零 sol 依赖**：外部代码只需包含 `ScriptParamTypes.h`，无需 sol
2. **类型安全**：使用 `std::variant` 在编译期检查类型
3. **向后兼容**：保留现有 sol 接口，不影响内部实现
4. **清晰分离**：接口层与实现层分离，符合依赖倒置原则
5. **易于扩展**：需要新类型时只需扩展 variant

## 使用示例

```cpp
// 外部代码（不依赖 sol）
#include "simulation/SimControlInterface.h"
#include "scripting/ScriptParamTypes.h"

void configureEntity(SimControlInterface* sim) {
    VehicleID entityId = ...;
    
    // 设置简单参数
    sim->setScriptParam(entityId, "patrol", "speed", 5.0);
    sim->setScriptParam(entityId, "patrol", "loop", true);
    sim->setScriptParam(entityId, "patrol", "mode", std::string("aggressive"));
    
    // 设置路径点
    std::vector<scripting::Waypoint> waypoints = {
        {100.0, 0.0, 100.0},
        {200.0, 0.0, 100.0},
        {200.0, 0.0, 200.0}
    };
    sim->setScriptWaypoints(entityId, "patrol", waypoints);
    
    // 批量设置
    std::unordered_map<std::string, scripting::ScriptParam> params = {
        {"speed", 5.0},
        {"loop", true},
        {"range", 100.0}
    };
    sim->setScriptParams(entityId, "patrol", params);
}
```

