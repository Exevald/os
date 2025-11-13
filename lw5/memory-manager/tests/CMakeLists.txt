include(GoogleTest)

add_executable(
        memory-manager-test
        MemoryManager_test.cpp
)

target_link_libraries(memory-manager-test PRIVATE GTest::GTest GTest::gtest_main memory-manager-lib)
gtest_discover_tests(memory-manager-test)