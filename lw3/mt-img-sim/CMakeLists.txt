add_executable(
        mt-img-sim
        ArgsParser.cpp
        ImageProcessor.cpp
        FileProcessor.cpp
        main.cpp
)

target_link_libraries(mt-img-sim PRIVATE stb_image_lib thead_pool_lib)