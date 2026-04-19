cmake_minimum_required(VERSION 3.5)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty)

add_executable(bt3_log_cat         bt_log_cat.cpp )
target_link_libraries(bt3_log_cat  ${BEHAVIOR_TREE_LIBRARY} )
set_target_properties(bt3_log_cat PROPERTIES FOLDER "Tools")
install(TARGETS bt3_log_cat
        DESTINATION ${BEHAVIOR_TREE_BIN_DESTINATION} )

if( ZMQ_FOUND )
    add_executable(bt3_recorder         bt_recorder.cpp )
    target_link_libraries(bt3_recorder  ${BEHAVIOR_TREE_LIBRARY} ${ZMQ_LIBRARIES})
    target_include_directories(bt3_recorder PRIVATE ${ZMQ_INCLUDE_DIRS})
    set_target_properties(bt3_recorder PROPERTIES FOLDER "Tools")
    install(TARGETS bt3_recorder
            DESTINATION ${BEHAVIOR_TREE_BIN_DESTINATION} )
endif()

add_executable(bt3_plugin_manifest         bt_plugin_manifest.cpp )
target_link_libraries(bt3_plugin_manifest  ${BEHAVIOR_TREE_LIBRARY} )
set_target_properties(bt3_plugin_manifest PROPERTIES FOLDER "Tools")
install(TARGETS bt3_plugin_manifest
        DESTINATION ${BEHAVIOR_TREE_BIN_DESTINATION} )


