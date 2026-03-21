# 核心存储库
add_library(core STATIC
    memory_store.cc
)

# 如果有持久化存储
# add_library(persistent_store STATIC
#     persistent_store.cc
# )

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
