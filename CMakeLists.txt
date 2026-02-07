cmake_minimum_required(VERSION 3.10)
project(distributed_kv_store VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 项目描述
set(PROJECT_DESCRIPTION "A distributed key-value store with master-slave replication")
set(PROJECT_URL "https://github.com/yourusername/distributed-kv-store")

# 构建选项
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(WITH_ASAN "Enable Address Sanitizer" OFF)
option(WITH_TSAN "Enable Thread Sanitizer" OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(src)
include_directories(src/common)
include_directories(src/core)
include_directories(src/network)
include_directories(src/client)
include_directories(src/replication)
include_directories(src/cluster)

# 编译器选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    # 地址消毒和线程消毒
    if(WITH_ASAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    endif()
    
    if(WITH_TSAN)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
    endif()
endif()

# 添加子目录（源码）
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/network)
add_subdirectory(src/client)
add_subdirectory(src/replication)
add_subdirectory(src/cluster)

# 可执行文件：服务器（支持主从复制）
add_executable(kv_server
    src/main_server.cc
)

target_link_libraries(kv_server
    common
    core
    network
    client
    replication
    cluster
    pthread
)

# 可执行文件：客户端
add_executable(kv_client
    src/main_client.cc
)

target_link_libraries(kv_client
    common
    client
    pthread
)

# 可执行文件：管理工具（可选）
add_executable(kv_admin
    src/main_admin.cc
)

target_link_libraries(kv_admin
    common
    client
    cluster
    pthread
)

# 安装目标
install(TARGETS kv_server kv_client kv_admin
    RUNTIME DESTINATION bin
)

install(DIRECTORY configs/
    DESTINATION etc/distributed-kv-store
)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 生成编译命令数据库（用于clangd等）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 打印配置摘要
message(STATUS "==============================================")
message(STATUS "Distributed KV Store Configuration")
message(STATUS "==============================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "Address Sanitizer: ${WITH_ASAN}")
message(STATUS "Thread Sanitizer: ${WITH_TSAN}")
message(STATUS "==============================================")
