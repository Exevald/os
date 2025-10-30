add_library(
        thread-safe-queue_lib
        ThreadSafeQueue.cpp
)

add_executable(
        thread-safe-queue
        ThreadSafeQueue.cpp
        main.cpp
)

target_include_directories(thread-safe-queue_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})