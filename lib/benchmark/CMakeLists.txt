add_library(benchmark_lib STATIC
    benchmark.cpp
)

target_include_directories(benchmark_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})