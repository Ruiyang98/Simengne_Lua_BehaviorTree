# Lua 库配置
# 使用方法：
#   1. 在 CMakeLists.txt 中包含此文件：include(cmake/lua.cmake)
#   2. 调用 link_lua(你的目标名) 来链接 Lua

# 设置 Lua 路径（相对于项目根目录）
set(LUA_VERSION "5.1.5")
set(LUA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lua")
set(LUA_INCLUDE_DIR "${LUA_ROOT}/include")
set(SOL2_INCLUDE_DIR "${LUA_ROOT}/include")# sol2 头文件路径
set(LUA_LIB_DIR "${LUA_ROOT}/lib")# lua 库文件路径

# 根据构建类型选择库文件
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LUA_LIBRARY "${LUA_LIB_DIR}/luad.lib")
else()
    set(LUA_LIBRARY "${LUA_LIB_DIR}/lua.lib")
endif()

# 创建导入目标（如果尚未创建）
if(NOT TARGET lua::lua)
    add_library(lua::lua STATIC IMPORTED)
    set_target_properties(lua::lua PROPERTIES
        IMPORTED_LOCATION "${LUA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
    )
endif()

# 创建 sol2 接口目标（头文件-only）
if(NOT TARGET sol2::sol2)
    add_library(sol2::sol2 INTERFACE IMPORTED)
    set_target_properties(sol2::sol2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SOL2_INCLUDE_DIR}"
    )
endif()

# 打印 Lua 配置信息
message(STATUS "Lua ${LUA_VERSION} configuration:")
message(STATUS "  Include: ${LUA_INCLUDE_DIR}")
message(STATUS "  Library: ${LUA_LIBRARY}")
message(STATUS "  Sol2:    ${SOL2_INCLUDE_DIR}")

#
# 宏：链接 Lua 库到指定目标
#
# 用法：link_lua(<target> [VISIBILITY])
#
# 参数：
#   target     - 要链接的目标名称
#   VISIBILITY - 可选，链接可见性（PRIVATE/PUBLIC/INTERFACE），默认为 PRIVATE
#
# 示例：
#   link_lua(my_app)                    # 默认 PRIVATE 链接
#   link_lua(my_lib PUBLIC)             # PUBLIC 链接
#   link_lua(another_lib INTERFACE)     # INTERFACE 链接
#
macro(link_lua target)
    # 解析可选参数
    set(_visibility "PRIVATE")
    if(${ARGC} GREATER 1)
        set(_visibility "${ARGV1}")
    endif()

    # 确保目标存在
    if(NOT TARGET ${target})
        message(FATAL_ERROR "link_lua: target '${target}' does not exist")
    endif()

    # 添加包含目录
    target_include_directories(${target} ${_visibility}
        ${LUA_INCLUDE_DIR}
        ${SOL2_INCLUDE_DIR}
    )

    # 链接 Lua 库
    target_link_libraries(${target} ${_visibility}
        ${LUA_LIBRARY}
    )

    # 添加编译定义（Windows 下）
    if(MSVC)
        target_compile_definitions(${target} ${_visibility}
            _CRT_SECURE_NO_WARNINGS
        )
    endif()

    message(STATUS "Linked Lua to '${target}' (${_visibility})")
endmacro()

#
# 宏：仅链接 Lua C 库（不包含 sol2）
#
# 用法：link_lua_c(<target> [VISIBILITY])
#
macro(link_lua_c target)
    set(_visibility "PRIVATE")
    if(${ARGC} GREATER 1)
        set(_visibility "${ARGV1}")
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "link_lua_c: target '${target}' does not exist")
    endif()

    target_include_directories(${target} ${_visibility}
        ${LUA_INCLUDE_DIR}
    )

    target_link_libraries(${target} ${_visibility}
        ${LUA_LIBRARY}
    )

    if(MSVC)
        target_compile_definitions(${target} ${_visibility}
            _CRT_SECURE_NO_WARNINGS
        )
    endif()

    message(STATUS "Linked Lua C library to '${target}' (${_visibility})")
endmacro()

#
# 宏：仅添加 sol2 头文件路径
#
# 用法：link_sol2(<target> [VISIBILITY])
#
macro(link_sol2 target)
    set(_visibility "PRIVATE")
    if(${ARGC} GREATER 1)
        set(_visibility "${ARGV1}")
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "link_sol2: target '${target}' does not exist")
    endif()

    target_include_directories(${target} ${_visibility}
        ${SOL2_INCLUDE_DIR}
    )

    message(STATUS "Linked sol2 to '${target}' (${_visibility})")
endmacro()
