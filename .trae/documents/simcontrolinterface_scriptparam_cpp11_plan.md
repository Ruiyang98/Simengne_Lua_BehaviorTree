# SimControlInterface 暴露 ScriptParam 接口设计方案 (C++11 版本)

## 当前限制

从 CMakeLists.txt 可以看到项目使用 **C++11**：

```cmake
set(CMAKE_CXX_STANDARD 11)
```

这意味着不能使用 C++17 的 `std::variant` 和 `std::optional`。

## 问题分析

### 如果引入 sol.hpp 会怎样？

**当前情况**：

* `SimControlInterface.h` 是核心接口头文件，被多处包含

* 目前 **没有** 直接包含 sol.hpp

* `EntityScriptManager.h` 包含了 sol.hpp

**如果 SimControlInterface.h 引入 sol.hpp**：

1. **编译时间增加**：sol2 是 heavy header-only 库，会显著增加编译时间
2. **依赖传播**：所有使用 SimControlInterface 的代码都会间接依赖 sol
3. **版本耦合**：外部代码必须使用相同的 sol 版本
4. **Lua 依赖**：外部代码必须链接 Lua 库
5. **头文件污染**：sol.hpp 会暴露大量模板代码

**示例影响**：

```cpp
// external_scheduler_example.cpp 目前只包含：
#include "simulation/MockSimController.h"

// 如果 SimControlInterface.h 引入 sol.hpp，
// 这个简单的示例也会编译 sol 的所有模板代码
```

## C++11 可行方案

### 方案一：使用 boost::variant / boost::optional（推荐如果有 boost）

如果项目已有 boost 依赖：

```cpp
#include <boost/variant.hpp>
#include <boost/optional.hpp>

using ScriptParam = boost::variant<bool, int, double, std::string, ...>;
template<typename T>
using Optional = boost::optional<T>;
```

### 方案二：自定义类型擦除容器（纯 C++11，无外部依赖）

创建一个不依赖 sol 的类型安全容器：

```cpp
// ScriptParamTypes.h - 纯 C++11，零依赖
#ifndef SCRIPT_PARAM_TYPES_H
#define SCRIPT_PARAM_TYPES_H

#include <string>
#include <vector>
#include <memory>

namespace scripting {

// 前向声明 - 实现隐藏在 cpp 中
class ScriptParamImpl;

// 支持的参数类型枚举
enum class ParamType {
    NIL,
    BOOL,
    INT,
    DOUBLE,
    STRING,
    VECTOR_DOUBLE,      // {1.0, 2.0, 3.0}
    WAYPOINT_LIST       // 路径点列表
};

// 路径点结构
struct Waypoint {
    double x, y, z;
    Waypoint(double x=0, double y=0, double z=0) : x(x), y(y), z(z) {}
};

// ScriptParam - 类型安全的参数容器（PIMPL 模式）
class ScriptParam {
public:
    // 构造函数
    ScriptParam();  // nil
    ScriptParam(bool value);
    ScriptParam(int value);
    ScriptParam(double value);
    ScriptParam(const std::string& value);
    ScriptParam(const char* value);
    ScriptParam(const std::vector<double>& value);
    ScriptParam(const std::vector<Waypoint>& value);
    
    // 拷贝/移动（C++11）
    ScriptParam(const ScriptParam& other);
    ScriptParam(ScriptParam&& other) noexcept;
    ScriptParam& operator=(const ScriptParam& other);
    ScriptParam& operator=(ScriptParam&& other) noexcept;
    ~ScriptParam();
    
    // 类型查询
    ParamType getType() const;
    bool isNil() const;
    
    // 获取值（返回 bool 表示是否成功）
    bool getBool(bool& out) const;
    bool getInt(int& out) const;
    bool getDouble(double& out) const;
    bool getString(std::string& out) const;
    bool getVectorDouble(std::vector<double>& out) const;
    bool getWaypoints(std::vector<Waypoint>& out) const;
    
    // 便捷获取（带默认值）
    bool asBool(bool defaultVal = false) const;
    int asInt(int defaultVal = 0) const;
    double asDouble(double defaultVal = 0.0) const;
    std::string asString(const std::string& defaultVal = "") const;
    
    // 工厂方法
    static ScriptParam makeNil();
    
private:
    std::unique_ptr<ScriptParamImpl> impl_;  // PIMPL
    
    // 内部使用 - 实现可以访问 impl
    friend class EntityScriptManager;
};

// 可选值模板（简化版 optional）
template<typename T>
class Optional {
public:
    Optional() : hasValue_(false) {}
    Optional(const T& value) : value_(value), hasValue_(true) {}
    Optional(T&& value) : value_(std::move(value)), hasValue_(true) {}
    
    bool hasValue() const { return hasValue_; }
    explicit operator bool() const { return hasValue_; }
    
    T& value() { return value_; }
    const T& value() const { return value_; }
    
    T valueOr(const T& defaultVal) const {
        return hasValue_ ? value_ : defaultVal;
    }
    
    void reset() { hasValue_ = false; }
    
private:
    T value_;
    bool hasValue_;
};

} // namespace scripting

#endif
```

**实现文件（ScriptParamTypes.cpp）**：

```cpp
#include "scripting/ScriptParamTypes.h"
#include <sol.hpp>  // 只在 cpp 中包含

namespace scripting {

// 内部实现使用 variant-like 结构
class ScriptParamImpl {
public:
    ParamType type;
    
    union Value {
        bool b;
        int i;
        double d;
        
        Value() {}
        ~Value() {}
    } value;
    
    std::string s;  // string 不能放在 union 中
    std::vector<double> vec;
    std::vector<Waypoint> waypoints;
    
    ScriptParamImpl() : type(ParamType::NIL) {}
};

// 构造函数实现
ScriptParam::ScriptParam() : impl_(new ScriptParamImpl()) {}
ScriptParam::ScriptParam(bool value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::BOOL;
    impl_->value.b = value;
}
ScriptParam::ScriptParam(int value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::INT;
    impl_->value.i = value;
}
ScriptParam::ScriptParam(double value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::DOUBLE;
    impl_->value.d = value;
}
ScriptParam::ScriptParam(const std::string& value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::STRING;
    impl_->s = value;
}
ScriptParam::ScriptParam(const char* value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::STRING;
    impl_->s = value;
}
ScriptParam::ScriptParam(const std::vector<double>& value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::VECTOR_DOUBLE;
    impl_->vec = value;
}
ScriptParam::ScriptParam(const std::vector<Waypoint>& value) : impl_(new ScriptParamImpl()) {
    impl_->type = ParamType::WAYPOINT_LIST;
    impl_->waypoints = value;
}

// 转换到 sol::object（内部使用）
sol::object ScriptParam::toSolObject(sol::state& lua) const {
    if (!impl_) return sol::nil;
    
    switch (impl_->type) {
        case ParamType::NIL: return sol::nil;
        case ParamType::BOOL: return sol::make_object(lua, impl_->value.b);
        case ParamType::INT: return sol::make_object(lua, impl_->value.i);
        case ParamType::DOUBLE: return sol::make_object(lua, impl_->value.d);
        case ParamType::STRING: return sol::make_object(lua, impl_->s);
        case ParamType::VECTOR_DOUBLE: {
            sol::table tbl = lua.create_table();
            for (size_t i = 0; i < impl_->vec.size(); ++i) {
                tbl[i + 1] = impl_->vec[i];
            }
            return tbl;
        }
        case ParamType::WAYPOINT_LIST: {
            sol::table tbl = lua.create_table();
            for (size_t i = 0; i < impl_->waypoints.size(); ++i) {
                sol::table point = lua.create_table();
                point["x"] = impl_->waypoints[i].x;
                point["y"] = impl_->waypoints[i].y;
                point["z"] = impl_->waypoints[i].z;
                tbl[i + 1] = point;
            }
            return tbl;
        }
    }
    return sol::nil;
}

// 从 sol::object 构造（内部使用）
ScriptParam ScriptParam::fromSolObject(const sol::object& obj) {
    if (!obj.valid() || obj.is<sol::nil_t>()) {
        return ScriptParam();
    }
    if (obj.is<bool>()) {
        return ScriptParam(obj.as<bool>());
    }
    if (obj.is<int>()) {
        return ScriptParam(obj.as<int>());
    }
    if (obj.is<double>()) {
        return ScriptParam(obj.as<double>());
    }
    if (obj.is<std::string>()) {
        return ScriptParam(obj.as<std::string>());
    }
    // table 处理...
    return ScriptParam();
}

} // namespace scripting
```

### 方案三：简单结构体 + 类型枚举（最轻量）

如果只需要有限类型，可以用简单结构体：

```cpp
// ScriptParamTypes.h - 极简版本
#ifndef SCRIPT_PARAM_TYPES_H
#define SCRIPT_PARAM_TYPES_H

#include <string>
#include <vector>

namespace scripting {

struct Waypoint {
    double x, y, z;
    Waypoint(double x=0, double y=0, double z=0) : x(x), y(y), z(z) {}
};

// 简单参数结构 - 所有字段直接暴露
struct ScriptParam {
    enum Type { NIL, BOOL, INT, DOUBLE, STRING, WAYPOINTS } type;
    
    bool boolVal;
    int intVal;
    double doubleVal;
    std::string stringVal;
    std::vector<Waypoint> waypoints;
    
    // 构造函数
    ScriptParam() : type(NIL) {}
    ScriptParam(bool v) : type(BOOL), boolVal(v) {}
    ScriptParam(int v) : type(INT), intVal(v) {}
    ScriptParam(double v) : type(DOUBLE), doubleVal(v) {}
    ScriptParam(const std::string& v) : type(STRING), stringVal(v) {}
    ScriptParam(const char* v) : type(STRING), stringVal(v) {}
    ScriptParam(const std::vector<Waypoint>& v) : type(WAYPOINTS), waypoints(v) {}
    
    // 便捷判断
    bool isNil() const { return type == NIL; }
};

// 简单 optional 模板
template<typename T>
class Optional {
    T val_;
    bool has_;
public:
    Optional() : has_(false) {}
    Optional(const T& v) : val_(v), has_(true) {}
    bool has() const { return has_; }
    const T& get() const { return val_; }
    T getOr(const T& d) const { return has_ ? val_ : d; }
};

} // namespace scripting

#endif
```

## 推荐的 C++11 方案

综合考虑，推荐 **方案二（PIMPL 模式）** 或 **方案三（简单结构体）**：

### 推荐方案对比

| 特性    | 方案二 PIMPL | 方案三 简单结构体 |
| ----- | --------- | --------- |
| 类型安全  | 高（编译期检查）  | 中（运行时检查）  |
| 内存占用  | 较小（动态分配）  | 较大（固定大小）  |
| 扩展性   | 好（易于添加类型） | 一般（需修改结构） |
| 实现复杂度 | 中等        | 简单        |
| 性能    | 有间接开销     | 直接访问      |

### 实施步骤（方案二 PIMPL）

1. **创建** **`ScriptParamTypes.h`**

   * 定义 `ScriptParam` 类（PIMPL 模式）

   * 定义 `Waypoint` 结构

   * 定义 `Optional<T>` 模板

   * **零外部依赖**

2. **创建** **`ScriptParamTypes.cpp`**

   * 实现 `ScriptParamImpl`

   * 实现到 sol::object 的转换函数

   * 包含 sol.hpp（仅在 cpp 中）

3. **修改** **`EntityScriptManager.h`**

   * 前向声明 sol 类型（不引入 sol.hpp）

   * 添加基于 `ScriptParam` 的新接口

   * 保留现有 sol 接口（向后兼容）

4. **修改** **`EntityScriptManager.cpp`**

   * 实现新的接口，内部转换为 sol::object

5. **修改** **`SimControlInterface.h`**

   * 包含 `ScriptParamTypes.h`

   * 添加纯虚接口

6. **修改** **`MockSimController.h/cpp`**

   * 实现新的纯虚接口

## 使用示例

```cpp
// 外部代码（不依赖 sol）
#include "simulation/SimControlInterface.h"

void configureEntity(SimControlInterface* sim) {
    VehicleID entityId = ...;
    
    // 设置简单参数
    sim->setScriptParam(entityId, "patrol", "speed", 5.0);
    sim->setScriptParam(entityId, "patrol", "loop", true);
    sim->setScriptParam(entityId, "patrol", "mode", std::string("aggressive"));
    
    // 设置路径点
    std::vector<scripting::Waypoint> waypoints;
    waypoints.push_back(scripting::Waypoint(100.0, 0.0, 100.0));
    waypoints.push_back(scripting::Waypoint(200.0, 0.0, 100.0));
    sim->setScriptWaypoints(entityId, "patrol", waypoints);
    
    // 获取参数
    scripting::Optional<scripting::ScriptParam> param = 
        sim->getScriptParam(entityId, "patrol", "speed");
    if (param.has()) {
        double speed = param.get().asDouble();
    }
}
```

## 总结

| 问题           | 答案                                   |
| ------------ | ------------------------------------ |
| C++11 可以实现吗？ | **可以**，使用 PIMPL 模式或简单结构体替代 variant   |
| 引入 sol 会怎样？  | 增加编译时间、传播依赖、耦合版本、暴露 Lua 依赖           |
| 推荐方案         | **PIMPL 模式**（方案二），平衡类型安全和实现复杂度       |
| 外部代码需要包含什么？  | 只需 `ScriptParamTypes.h`，**零 sol 依赖** |

