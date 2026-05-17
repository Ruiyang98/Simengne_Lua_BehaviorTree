# EntityScript 多次加载 Lua 脚本的缓冲区溢出风险分析计划

## 问题概述

用户询问：EntityScript 多次加载 Lua 脚本后，是否会出现缓冲区溢出现象？

## 代码分析结果

### 1. 当前脚本加载机制

EntityScriptManager 使用 **sol2** 库绑定 Lua，核心机制如下：

* **全局 Lua 状态**：所有 EntityScriptManager 共享一个全局 `sol::state`（来自 `LuaSimBinding` 单例）

* **脚本隔离**：每个脚本通过 `luaState_->create_table()` 创建独立的 state table 实现隔离

* **脚本存储**：使用 `std::unordered_map<std::string, sol::table> scriptStates_` 保存脚本状态

### 2. 关键代码路径

#### 2.1 脚本添加流程（`addTacticalScript` / `addBTScript`）

```cpp
// EntityScriptManager.cpp:50-54
sol::table scriptState = luaState_->create_table();
scriptState["_script_name"] = scriptName;
scriptStates_[scriptName] = scriptState;
```

#### 2.2 脚本执行流程（`TacticalScript::initializeScript`）

```cpp
// TacticalScript.cpp:29
auto result = luaState_->script(scriptCode);

// TacticalScript.cpp:38
executeFunc_ = (*luaState_)["execute"];
```

**关键问题**：`luaState_->script(scriptCode)` 在全局环境中执行脚本，会：

1. 将脚本中定义的所有函数/变量注册到全局表 `_G`
2. 如果多个脚本定义了同名的全局函数（如 `execute`），**后面的脚本会覆盖前面的**

### 3. 已识别的风险点

| 风险点             | 严重程度 | 说明                                                                             |
| --------------- | ---- | ------------------------------------------------------------------------------ |
| **全局函数覆盖**      | 高    | 多个脚本的 `execute` 函数在全局命名空间冲突，导致行为不可预测                                           |
| **Lua 全局表膨胀**   | 中    | 每次加载脚本都会在全局表中创建新条目，长期运行可能导致内存增长                                                |
| sol::table 引用累积 | 中    | `scriptStates_` 和 `scripts_` 中的条目在 `removeScript` 时才会释放，频繁 add/remove 可能导致内存碎片 |
| 缓冲区溢出（传统意义）     | 低    | sol2 和 Lua 内部使用动态内存分配，不存在传统 C 风格的缓冲区溢出风险                                       |

### 4. 关于"缓冲区溢出"的澄清

**传统意义上的缓冲区溢出（Buffer Overflow）**：

* 指向固定大小缓冲区写入超出其容量的数据

* **在当前代码中不存在**，因为：

  * `std::string` 和 `std::stringstream` 自动管理内存

  * sol2/Lua 使用动态内存分配

  * 没有使用固定大小的 C 风格数组

**但存在以下"类溢出"风险**：

1. **Lua 全局表无限增长**：如果频繁加载不同名称的脚本，全局表 `_G` 会持续累积变量
2. **内存泄漏风险**：`sol::table` 持有 Lua 对象的引用，如果 C++ 端的 map 未正确清理，Lua GC 无法回收

### 5. 修复建议

#### 5.1 立即修复：脚本沙箱化（高优先级）

修改 `TacticalScript::initializeScript`，使用独立环境加载脚本：

```cpp
bool TacticalScript::initializeScript(const std::string& scriptCode) {
    try {
        // 为脚本创建独立环境，避免全局命名空间污染
        sol::table env = luaState_->create_table();
        env[sol::metatable_key] = luaState_->create_table_with(
            sol::meta_function::index, luaState_->globals()
        );
        
        // 在独立环境中执行脚本
        sol::load_result loaded = luaState_->load(scriptCode);
        if (!loaded.valid()) {
            sol::error err = loaded;
            std::cerr << "[TacticalScript] Syntax error in script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        sol::protected_function scriptFunc = loaded;
        scriptFunc.environment = env;
        auto result = scriptFunc();
        
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[TacticalScript] Error executing script '" << name_ << "': " << err.what() << std::endl;
            return false;
        }
        
        // 从独立环境获取 execute 函数
        executeFunc_ = env["execute"];
        
        if (!executeFunc_.valid()) {
            std::cerr << "[TacticalScript] No 'execute' function found in script: " << name_ << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TacticalScript] Error initializing script '" << name_ << "': " << e.what() << std::endl;
        return false;
    }
}
```

#### 5.2 添加脚本重载保护（中优先级）

在 `EntityScriptManager::addTacticalScript` 中，如果脚本已存在，先移除旧脚本：

```cpp
bool EntityScriptManager::addTacticalScript(const std::string& scriptName, const std::string& scriptCode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果脚本已存在，先移除旧脚本以释放资源
    if (scripts_.find(scriptName) != scripts_.end()) {
        removeScriptInternal(scriptName);  // 需要添加内部无锁版本
    }
    
    // ... 原有逻辑
}
```

#### 5.3 添加 Lua GC 调优（低优先级）

在 `EntityScriptManager::removeScript` 中主动触发 Lua GC：

```cpp
bool EntityScriptManager::removeScript(const std::string& scriptName) {
    // ... 原有移除逻辑 ...
    
    // 建议 Lua 进行垃圾回收
    luaState_->collect_garbage();
    
    return true;
}
```

#### 5.4 添加脚本加载计数/限制（可选）

为防止恶意或错误的频繁加载，可添加加载频率限制。

## 实施步骤

1. **步骤 1**：修改 `TacticalScript::initializeScript` 实现脚本沙箱化
2. **步骤 2**：修改 `BTScript::initializeScript` 实现同样的沙箱化
3. **步骤 3**：添加 `removeScriptInternal` 无锁方法，支持安全重载
4. **步骤 4**：修改 `addTacticalScript` 和 `addBTScript` 支持重载已有脚本
5. **步骤 5**：添加 Lua GC 触发逻辑
6. **步骤 6**：运行测试验证修复效果

## 结论

**传统意义上的缓冲区溢出不会发生**，但存在：

* **全局命名空间污染**导致的函数覆盖问题

* **Lua 全局表膨胀**导致的内存增长问题

建议按照上述步骤实施修复，确保脚本隔离性和内存稳定性。
