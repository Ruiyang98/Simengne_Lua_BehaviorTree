# 行为树节点模块

set(BT_NODES_SOURCES
    EntityMovement.cpp
    MoveToPoint.cpp
    FollowPath.cpp
    CheckEntityExists.cpp
    BehaviorTreeExecutor.cpp
)

add_library(bt_nodes STATIC ${BT_NODES_SOURCES})

target_include_directories(bt_nodes PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    #${CMAKE_SOURCE_DIR}/3rdparty/BehaviorTree.CPP/BehaviorTree.CPP-3.8/include
)

target_link_libraries(bt_nodes PUBLIC
    simulation
    behaviortree::behaviortree_cpp_v3
)
