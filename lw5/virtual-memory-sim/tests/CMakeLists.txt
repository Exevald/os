include(GoogleTest)

add_executable(
        virtual-memory-sim-test
        PTE_test.cpp
        PhysicalMemory_test.cpp
        VirtualMemorySimulator_test.cpp
)

target_link_libraries(virtual-memory-sim-test PRIVATE GTest::GTest GTest::gtest_main virtual-memory-sim-lib)
gtest_discover_tests(virtual-memory-sim-test)