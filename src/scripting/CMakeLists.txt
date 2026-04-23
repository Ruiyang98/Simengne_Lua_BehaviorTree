# script_system 库 CMake 配置

# 收集源文件
set(SCRIPTING_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/LuaSimBinding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LuaBehaviorTreeBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LuaBehaviorTreeNodes.cpp
)

# 创建静态库
add_library(script_system STATIC ${SCRIPTING_SOURCES})

# 设置包含目录
target_include_directories(script_system PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 设置 C++ 标准
target_compile_features(script_system PUBLIC cxx_std_11)

# 链接 Lua 库
target_link_libraries(script_system PUBLIC
    lua::lua
    simulation
    bt_nodes
)

# Windows 特定配置
if(MSVC)
    target_compile_definitions(script_system PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

message(STATUS "script_system library configured")
