add_library(stb_image_lib STATIC
        stb_image.cpp
)

target_include_directories(stb_image_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})