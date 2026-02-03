cmake_minimum_required(VERSION 3.10)
project(kv_store)

# 使用C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# 包含目录
include_directories(src)

# 服务器可执行文件（阶段一已有的）
add_executable(kv_server
    src/main_server.cc
    src/common/logger.cc
    src/common/protocol.cc
    src/common/utils.cc
    src/core/memory_store.cc
    src/network/simple_server.cc
)

# 客户端可执行文件（阶段二新增）
add_executable(kv_client
    src/main_client.cc
    src/common/logger.cc
    src/common/protocol.cc
    src/common/utils.cc
    # 阶段二客户端文件
    src/client/kv_client.cc
    src/client/router.cc
    src/client/cluster_config.cc
    src/client/connection.cc
)

# 链接pthread库
target_link_libraries(kv_server pthread)
target_link_libraries(kv_client pthread)
