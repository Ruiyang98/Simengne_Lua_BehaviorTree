# Lua 文件整理合并计划

## 当前 Lua 文件分析

### 项目脚本目录 (`scripts/`)

| 文件 | 功能 | 状态 |
|------|------|------|
| `bt_nodes_registry.lua` | 全局行为树节点注册中心，包含移动、战斗、交互、健康、目标检测等节点 | **核心文件，保留** |
| `bt_stateful_action_example.lua` | 演示 LuaStatefulAction 连续移动，使用 sim 接口 | **示例文件，可合并** |
| `bt_advanced_example.lua` | 高级行为树示例，综合演示多种功能 | **示例文件，可合并** |
| `bt_lua_node_with_params.lua` | Lua 节点参数使用示例 | **示例文件，可合并** |
| `bt_custom_node_example.lua` | 自定义 Lua 节点示例（内联 XML） | **示例文件，可合并** |
| `bt_custom_node_from_file.lua` | 自定义 Lua 节点示例（从文件加载 XML） | **与 bt_custom_node_example.lua 重复度高，可合并** |
| `load_lua_nodes_xml.lua` | 单独加载 lua_custom_nodes_example.xml 的示例 | **示例文件，可合并** |
| `async_bt_example.lua` | 异步行为树执行示例 | **示例文件，可合并** |
| `bt_blackboard_example.lua` | 黑板操作示例 | **示例文件，可合并** |
| `bt_control_example.lua` | 行为树基础控制示例 | **示例文件，可合并** |
| `entity_control_test.lua` | 实体控制测试脚本 | **示例文件，可合并** |
| `example_control.lua` | 基础仿真控制示例 | **示例文件，可合并** |
| `advanced_control.lua` | 高级仿真控制示例 | **示例文件，可合并** |

### 第三方库目录 (`3rdparty/`)
- `sol2-2.17.5/examples/*.lua` - sol2 库示例，**保留**
- `lua-5.1.5/etc/*.lua` - Lua 标准库，**保留**
- `lua-5.1.5/test/*.lua` - Lua 测试文件，**保留**

---

## 整理方案

### 1. 保留的核心文件
- `bt_nodes_registry.lua` - 作为全局节点注册中心保留

### 2. 合并示例文件（采用 example_ 前缀命名）

#### 2.1 `example_bt_basic.lua`
合并以下基础示例：
- `bt_control_example.lua` - 行为树基础控制
- `bt_blackboard_example.lua` - 黑板操作
- `entity_control_test.lua` - 实体控制

#### 2.2 `example_bt_lua_nodes.lua`
合并以下 Lua 节点示例：
- `bt_custom_node_example.lua` - 自定义节点（内联 XML）
- `bt_custom_node_from_file.lua` - 自定义节点（文件加载）
- `load_lua_nodes_xml.lua` - 加载 XML 示例
- `bt_lua_node_with_params.lua` - 参数化节点

#### 2.3 `example_bt_advanced.lua`
合并以下高级示例：
- `bt_advanced_example.lua` - 高级综合示例
- `bt_stateful_action_example.lua` - 有状态动作
- `async_bt_example.lua` - 异步执行

#### 2.4 `example_sim_control.lua`
合并以下仿真控制示例：
- `example_control.lua` - 基础控制
- `advanced_control.lua` - 高级控制

### 3. 删除的冗余文件
以下文件在合并后删除：
- `bt_control_example.lua`
- `bt_blackboard_example.lua`
- `entity_control_test.lua`
- `bt_custom_node_example.lua`
- `bt_custom_node_from_file.lua`
- `load_lua_nodes_xml.lua`
- `bt_lua_node_with_params.lua`
- `bt_advanced_example.lua`
- `bt_stateful_action_example.lua`
- `async_bt_example.lua`
- `example_control.lua`
- `advanced_control.lua`

---

## 实施步骤

1. 创建合并后的示例文件（4个，使用 example_ 前缀）
2. 删除原始的冗余文件（12个）
3. 验证 `bt_nodes_registry.lua` 功能完整

---

## 预期结果

整理后文件结构：
```
scripts/
├── bt_nodes_registry.lua          # 核心节点注册中心
├── example_bt_basic.lua           # 基础示例合集
├── example_bt_lua_nodes.lua       # Lua节点示例合集
├── example_bt_advanced.lua        # 高级示例合集
└── example_sim_control.lua        # 仿真控制示例合集
```

文件数量：从 13 个减少到 5 个（减少 8 个文件）
