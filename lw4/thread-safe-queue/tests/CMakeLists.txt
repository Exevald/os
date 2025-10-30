include(GoogleTest)

add_executable(
        thread-safe-queue-test
        ThreadSafeQueue_test.cpp
)

target_link_libraries(thread-safe-queue-test PRIVATE GTest::GTest GTest::gtest_main thread-safe-queue_lib)
gtest_discover_tests(thread-safe-queue-test)