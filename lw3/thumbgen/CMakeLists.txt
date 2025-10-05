add_executable(
        thumbgen
        ArgsParser.cpp
        FileProcessor.cpp
        ImageProcessor.cpp
        ThumbnailGenerator.cpp
        main.cpp
)

target_link_libraries(thumbgen PRIVATE thead_pool_lib stb_image_lib)