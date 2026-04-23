cmake_minimum_required(VERSION 3.10)
project(MyProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含 Lua 配置
include(cmake/lua.cmake)

# 包含 BehaviorTree.CPP 配置
include(cmake/BehaviorTree.cmake)

# 添加子目录
add_subdirectory(src/simulation)
add_subdirectory(src/scripting)
add_subdirectory(src/behaviortree)

# 创建可执行文件
add_executable(my_app src/main.cpp)

# 链接库
target_link_libraries(my_app PRIVATE
    simulation
    script_system
    bt_nodes
)

# 创建外部调度示例程序
add_executable(external_scheduler_example examples/external_scheduler_example.cpp)
target_link_libraries(external_scheduler_example PRIVATE
    simulation
    script_system
    bt_nodes
)

# 链接 Lua
# link_lua(my_app)

# 复制脚本文件到构建目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scripts 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
