# BehaviorTree.CPP 库配置
# 使用方法：
#   1. 在 CMakeLists.txt 中包含此文件：include(cmake/BehaviorTree.cmake)
#   2. 调用 link_behaviortree(你的目标名) 来链接 BehaviorTree.CPP

# 设置 BehaviorTree.CPP 路径（相对于项目根目录）
set(BTCPP_VERSION "3.8")
set(BTCPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/BehaviorTree.CPP")
set(BTCPP_INCLUDE_DIR "${BTCPP_ROOT}/include")
set(BTCPP_LIB_DIR "${BTCPP_ROOT}/lib")

# 根据构建类型选择库文件
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BTCPP_LIBRARY "${BTCPP_LIB_DIR}/behaviortree_cpp_v3d.lib")
    set(BTCPP_SHARED_LIBARY "${BTCPP_LIB_DIR}/behaviortree_cpp_v3d.dll")
else()
    set(BTCPP_LIBRARY "${BTCPP_LIB_DIR}/behaviortree_cpp_v3.lib")
    set(BTCPP_SHARED_LIBARY "${BTCPP_LIB_DIR}/behaviortree_cpp_v3.dll")
endif()

# 创建导入目标（如果尚未创建）
if(NOT TARGET behaviortree::behaviortree_cpp_v3)
    add_library(behaviortree::behaviortree_cpp_v3 STATIC IMPORTED)
    set_target_properties(behaviortree::behaviortree_cpp_v3 PROPERTIES
        IMPORTED_LOCATION "${BTCPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BTCPP_INCLUDE_DIR}"
    )
endif()

# 打印 BehaviorTree.CPP 配置信息
message(STATUS "BehaviorTree.CPP ${BTCPP_VERSION} configuration:")
message(STATUS "  Include: ${BTCPP_INCLUDE_DIR}")
message(STATUS "  Library: ${BTCPP_LIBRARY}")

#
# 宏：链接 BehaviorTree.CPP 库到指定目标
#
# 用法：link_behaviortree(<target> [VISIBILITY])
#
# 参数：
#   target     - 要链接的目标名称
#   VISIBILITY - 可选，链接可见性（PRIVATE/PUBLIC/INTERFACE），默认为 PRIVATE
#
# 示例：
#   link_behaviortree(my_app)                    # 默认 PRIVATE 链接
#   link_behaviortree(my_lib PUBLIC)             # PUBLIC 链接
#   link_behaviortree(another_lib INTERFACE)     # INTERFACE 链接
#
macro(link_behaviortree target)
    # 解析可选参数
    set(_visibility "PRIVATE")
    if(${ARGC} GREATER 1)
        set(_visibility "${ARGV1}")
    endif()

    # 确保目标存在
    if(NOT TARGET ${target})
        message(FATAL_ERROR "link_behaviortree: target '${target}' does not exist")
    endif()

    # 添加包含目录
    #target_include_directories(${target} ${_visibility}
    #    ${BTCPP_INCLUDE_DIR}
    #)

    # 链接 BehaviorTree.CPP 库
    target_link_libraries(${target} ${_visibility}
        behaviortree::behaviortree_cpp_v3
    )

    # 拷贝 behaviortree_cpp_v3.dll 到目标输出目录
    install(FILES ${BTCPP_SHARED_LIBARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    message(STATUS "Linked BehaviorTree.CPP to '${target}' (${_visibility})")
endmacro()
