# simulation 库 CMake 配置

# 收集源文件
set(SIMULATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SimControlInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MockSimController.cpp
)

# 创建静态库
add_library(simulation STATIC ${SIMULATION_SOURCES})

# 设置包含目录
target_include_directories(simulation PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 设置 C++ 标准
target_compile_features(simulation PUBLIC cxx_std_11)

# Windows 特定配置
if(MSVC)
    target_compile_definitions(simulation PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

message(STATUS "simulation library configured")
