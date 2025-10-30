find_package(benchmark REQUIRED)

add_executable(tread-safe-queue-benchmark benchmark.cpp)

target_include_directories(tread-safe-queue-benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

target_link_libraries(tread-safe-queue-benchmark
        PRIVATE
        thread-safe-queue_lib
        benchmark::benchmark
)