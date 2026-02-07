# 查找Google Test
find_package(GTest REQUIRED)

# 单元测试
add_executable(test_unit
    unit/test_kv_store.cc
    unit/test_router.cc
    unit/test_protocol.cc
    unit/test_replication_log.cc
    unit/test_replica_manager.cc
)

target_link_libraries(test_unit
    common
    core
    client
    replication
    cluster
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 集成测试
add_executable(test_integration
    integration/test_replication_basic.cc
    integration/test_cluster_basic.cc
    integration/test_failover_scenarios.cc
)

target_link_libraries(test_integration
    common
    core
    network
    client
    replication
    cluster
    GTest::gtest
    GTest::gtest_main
    pthread
)

# 性能测试（如果启用）
if(BUILD_BENCHMARKS)
    add_executable(test_performance
        integration/test_performance.cc
    )
    
    target_link_libraries(test_performance
        common
        core
        network
        client
        replication
        cluster
        GTest::gtest
        GTest::gtest_main
        pthread
    )
endif()

# 添加测试
add_test(NAME unit_tests
    COMMAND test_unit
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME integration_tests
    COMMAND test_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(BUILD_BENCHMARKS)
    add_test(NAME performance_tests
        COMMAND test_performance
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# 复制测试数据文件
file(COPY test_data DESTINATION ${CMAKE_BINARY_DIR}/tests)
