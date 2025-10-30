cmake_minimum_required(VERSION 3.27)
project(os)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(GTest REQUIRED)

add_subdirectory(lib)
add_subdirectory(lw1)
add_subdirectory(lw2)
add_subdirectory(lw3)
add_subdirectory(lw4)