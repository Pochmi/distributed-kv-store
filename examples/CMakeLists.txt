# 示例程序

# 基本用法示例
add_executable(example_basic basic_usage.cc)
target_link_libraries(example_basic 
    client
    common
)

# 集群客户端示例
add_executable(example_cluster_client cluster_client.cc)
target_link_libraries(example_cluster_client 
    client
    common
)

# 复制演示示例
add_executable(example_replication replication_demo.cc)
target_link_libraries(example_replication 
    client
    common
)

# 管理工具示例
add_executable(example_admin admin_tool.cc)
target_link_libraries(example_admin 
    common
    client  # ClusterConfig 在 client 库中
)

# 设置输出目录
set_target_properties(example_basic example_cluster_client example_replication example_admin
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
