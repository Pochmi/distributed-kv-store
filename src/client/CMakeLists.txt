# 客户端库
add_library(client STATIC
    kv_client.cc
    router.cc
    cluster_config.cc
    connection.cc
)

target_link_libraries(client
    common
    pthread
)

target_include_directories(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
