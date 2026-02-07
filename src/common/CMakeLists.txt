# 公共库
add_library(common STATIC
    logger.cc
    config.cc
    protocol.cc
    utils.cc
)

# 如果使用JSON库，可以在这里添加
# find_package(nlohmann_json REQUIRED)
# target_link_libraries(common nlohmann_json::nlohmann_json)

# 编译选项
target_compile_options(common PRIVATE -Wall -Wextra)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
