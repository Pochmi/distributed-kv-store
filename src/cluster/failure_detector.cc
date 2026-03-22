add_library(cluster STATIC
    heartbeat.cc
    failure_detector.cc
    failover_manager.cc
    admin_command.cc
    admin_client.cc
)

target_link_libraries(cluster
    common
)

target_include_directories(cluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 禁用未使用参数警告
target_compile_options(cluster PRIVATE -Wno-unused-parameter)