add_library(cluster STATIC
    heartbeat.cc
    failover_manager.cc
    admin_command.cc
    admin_client.cc
)

target_link_libraries(cluster
    common
)

target_include_directories(cluster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 禁用未使用参数警告（避免编译错误）
target_compile_options(cluster PRIVATE -Wno-unused-parameter)