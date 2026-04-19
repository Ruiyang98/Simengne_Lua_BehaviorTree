# 多次加载脚本支持计划

## 目标
将程序改为纯交互式模式，启动后直接进入命令循环，用户可以在程序运行时连续输入脚本路径并执行，直到主动退出。不再支持命令行参数启动。

## 现状分析
当前实现：
- 通过命令行参数 `argv[1]` 指定单个脚本文件
- 执行完脚本后立即退出
- 不支持交互式输入

需要改进：
- 启动后直接进入交互式命令循环
- 支持用户输入脚本路径并执行
- 支持输入特殊命令（如 `quit`/`exit`）退出程序
- 可以连续多次加载不同的脚本

## 实现步骤

### 步骤1: 修改 main.cpp 添加交互式循环
**文件**: `src/main.cpp`

1. 移除命令行参数处理逻辑
2. 添加交互式命令循环函数 `runInteractiveMode()`
3. 修改主逻辑：
   - 初始化后直接启动交互式模式
4. 在交互式模式中：
   - 显示提示符（如 `LuaSim> `）
   - 读取用户输入
   - 处理特殊命令：`quit`、`exit`、`help`
   - 执行输入的脚本路径
   - 显示执行结果或错误信息
   - 循环等待下一次输入

### 步骤2: 更新帮助信息
**文件**: `src/main.cpp`

更新 `printUsage()` 函数，添加交互式模式的说明：
- 列出支持的命令
- 给出使用示例

### 步骤3: 添加输入处理辅助函数
**文件**: `src/main.cpp`

添加辅助函数：
- `trim()` - 去除字符串首尾空格
- `toLower()` - 字符串转小写（用于命令比较）

### 步骤4: 测试验证
1. 编译项目
2. 测试交互式模式：
   - 启动程序
   - 输入 `help` 查看帮助
   - 输入脚本路径执行
   - 连续执行多个脚本
   - 输入 `quit` 退出

## 交互式模式设计

### 命令列表
| 命令 | 功能 |
|------|------|
| `quit` / `exit` | 退出程序 |
| `help` | 显示帮助信息 |
| `clear` | 清屏 |
| `<脚本路径>` | 执行指定的 Lua 脚本 |

### 交互示例
```
========================================
    Lua Simulation Control Demo
========================================

OK: Lua environment initialized

Entering interactive mode. Type 'help' for usage, 'quit' to exit.

LuaSim> help
Available commands:
  help              - Show this help message
  quit/exit         - Exit the program
  clear             - Clear screen
  <script_path>     - Execute a Lua script file

Examples:
  LuaSim> scripts/example_control.lua
  LuaSim> scripts/entity_control_test.lua

LuaSim> scripts/entity_control_test.lua
Executing script: scripts/entity_control_test.lua
----------------------------------------
... (script output) ...
----------------------------------------
OK: Script executed successfully

LuaSim> scripts/example_control.lua
Executing script: scripts/example_control.lua
----------------------------------------
... (script output) ...
----------------------------------------
OK: Script executed successfully

LuaSim> quit
Goodbye!
```

## 代码变更概要

### main.cpp 变更点
1. 修改 `main()` 函数签名 - 移除 argc/argv 参数（或忽略）
2. 添加 `#include <algorithm>` 用于字符串处理
3. 添加字符串处理辅助函数
4. 添加 `runInteractiveMode()` 函数
5. 更新 `printUsage()` 函数

## 风险与注意事项
1. 错误处理 - 脚本执行失败时不应退出程序
2. 资源管理 - 多次执行脚本不应导致内存泄漏
3. 输入验证 - 处理空输入和无效路径
