add_library(thread_pool_lib STATIC
        thread_pool.cpp
)

target_include_directories(thread_pool_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})