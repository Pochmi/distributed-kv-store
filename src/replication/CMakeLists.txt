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