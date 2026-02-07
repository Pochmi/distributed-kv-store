# ============================================
# Cluster Management Module
# Phase 3: Master-Slave Replication Support
# ============================================

# 定义集群管理库
add_library(cluster STATIC
    # 心跳检测和健康监控
    heartbeat.cc
    
    # 故障检测器
    failure_detector.cc
    
    # 故障切换管理器
    failover_manager.cc
    
    # 管理命令接口
    admin_command.cc
)

# 设置目标属性
set_target_properties(cluster PROPERTIES
    VERSION 3.0.0
    SOVERSION 3
    OUTPUT_NAME "distributed_kv_cluster"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# 包含目录
target_include_directories(cluster
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}            # 当前目录
        ${CMAKE_SOURCE_DIR}/src/common         # 公共头文件
        ${CMAKE_SOURCE_DIR}/src/replication    # 复制模块头文件
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}            # 私有生成文件
)

# 链接依赖库
target_link_libraries(cluster
    PUBLIC
        common          # 公共基础库（logger, config, utils等）
    PRIVATE
        pthread         # 线程支持
)

# 编译选项
target_compile_options(cluster
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused-parameter
        $<$<CONFIG:DEBUG>:-g -O0 -DCLUSTER_DEBUG=1>
        $<$<CONFIG:RELEASE>:-O3 -DNDEBUG>
)

# 预处理器定义
target_compile_definitions(cluster
    PRIVATE
        WITH_CLUSTER=1                         # 启用集群功能
        HEARTBEAT_INTERVAL_MS=1000             # 心跳间隔1秒
        HEARTBEAT_TIMEOUT_MS=3000              # 心跳超时3秒
        FAILOVER_RETRY_COUNT=3                 # 故障切换重试次数
        ELECTION_TIMEOUT_MS=5000               # 选举超时5秒
    PUBLIC
        CLUSTER_API_EXPORT                     # 导出API符号
)

# 安装配置
install(TARGETS cluster
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/cluster
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

# 测试目标（如果启用了测试）
if(BUILD_TESTS)
    # 这里可以添加集群模块的单元测试
    # add_executable(test_cluster ...)
    # target_link_libraries(test_cluster cluster gtest gtest_main)
endif()

# 打印构建信息
message(STATUS "Cluster module configured:")
message(STATUS "  Sources: heartbeat.cc, failure_detector.cc, failover_manager.cc, admin_command.cc")
message(STATUS "  Dependencies: common, pthread")
message(STATUS "  Compile definitions: WITH_CLUSTER=1, HEARTBEAT_INTERVAL_MS=1000")
