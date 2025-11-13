add_executable(
        memory-manager
        MemoryManager.cpp
        main.cpp
)

add_library(
        memory-manager-lib
        MemoryManager.cpp
)

target_include_directories(memory-manager-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
