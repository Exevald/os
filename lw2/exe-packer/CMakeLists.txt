find_package(ZLIB REQUIRED)

add_executable(
        exe-packer
        Packer.cpp
        Unpacker.cpp
        FileDesc.cpp
        main.cpp
)

target_link_libraries(exe-packer PRIVATE ZLIB::ZLIB)