# 网络库
add_library(network STATIC
    simple_server.cc
    # connection.cc  # 如果有连接管理
    # handler.cc     # 如果有请求处理器
)

target_link_libraries(network
    common
    core
    pthread
)

target_include_directories(network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
