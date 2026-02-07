# 基础使用示例
add_executable(example_basic
    basic_usage.cc
)

target_link_libraries(example_basic
    common
    client
)

# 复制示例
add_executable(example_replication
    replication_demo.cc
)

target_link_libraries(example_replication
    common
    client
)

# 集群管理示例
add_executable(example_cluster
    cluster_client.cc
)

target_link_libraries(example_cluster
    common
    client
    cluster
)

# 管理工具示例
add_executable(example_admin
    admin_tool.cc
)

target_link_libraries(example_admin
    common
    client
    cluster
)
