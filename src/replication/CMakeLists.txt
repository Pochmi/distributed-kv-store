# 复制模块
add_library(replication STATIC
    replica_manager.cc
    replication_log.cc
    master.cc
    slave.cc
)

target_link_libraries(replication
    common
    core
    network
    pthread
)

target_include_directories(replication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 添加复制模块的编译定义
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(replication PRIVATE REPLICATION_DEBUG=1)
endif()
