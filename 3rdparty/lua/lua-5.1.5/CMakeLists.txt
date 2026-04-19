cmake_minimum_required(VERSION 3.10)
project(lua C)

set(LUA_VERSION "5.1.5")
set(LUA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_C_STANDARD 99)

file(GLOB LUA_LIB_SOURCES ${LUA_SRC_DIR}/*.c)
list(REMOVE_ITEM LUA_LIB_SOURCES 
    ${LUA_SRC_DIR}/lua.c #解释器
    ${LUA_SRC_DIR}/luac.c #编译器
)

add_library(lua_static STATIC ${LUA_LIB_SOURCES})

target_include_directories(lua_static PUBLIC
    ${LUA_SRC_DIR}
)

# Linux/macOS 需要链接数学库和动态加载库
if(UNIX)
    target_link_libraries(lua_static PUBLIC m ${CMAKE_DL_LIBS})
	target_compile_definitions(lua_static PUBLIC LUA_USE_LINUX)
endif()

if(MSVC)
    target_compile_definitions(lua_static PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(lua_static PROPERTIES
    OUTPUT_NAME "lua"
    POSITION_INDEPENDENT_CODE ON
    DEBUG_POSTFIX "d"
)

# ========== 2. 编译 lua 解释器 ==========
add_executable(lua ${LUA_SRC_DIR}/lua.c)
target_include_directories(lua PRIVATE ${LUA_SRC_DIR})
target_link_libraries(lua lua_static)
set_target_properties(lua PROPERTIES OUTPUT_NAME "lua")


# ========== 3. 编译 luac 编译器 ==========
add_executable(luac ${LUA_SRC_DIR}/luac.c ${LUA_SRC_DIR}/print.c)
target_include_directories(luac PRIVATE ${LUA_SRC_DIR})
target_link_libraries(luac lua_static)
set_target_properties(luac PROPERTIES OUTPUT_NAME "luac")

set(LUA_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../)

install(TARGETS lua_static
    ARCHIVE DESTINATION ${LUA_INSTALL_PREFIX}/lib
    LIBRARY DESTINATION ${LUA_INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${LUA_INSTALL_PREFIX}/bin
)

install(TARGETS lua luac
    RUNTIME DESTINATION ${LUA_INSTALL_PREFIX}/bin
)

install(DIRECTORY ${LUA_SRC_DIR}/
    DESTINATION ${LUA_INSTALL_PREFIX}/include
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "Lua ${LUA_VERSION} will be installed to: ${LUA_INSTALL_PREFIX}")
