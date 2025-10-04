add_library(thead_pool_lib STATIC
        thread_pool.cpp
)

target_include_directories(thead_pool_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})