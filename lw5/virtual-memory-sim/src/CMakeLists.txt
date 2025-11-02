add_library(
        virtual-memory-sim-lib
        PhysicalMemory.cpp
        VirtualMemory.cpp
)

target_include_directories(virtual-memory-sim-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})