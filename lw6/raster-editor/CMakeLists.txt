add_executable(
        raster-editor
        ImageFile.cpp
        ImageViewer.cpp
        MipMapGenerator.cpp
        main.cpp
)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

target_link_libraries(raster-editor
        ${Boost_LIBRARIES}
        sfml-system
        sfml-window
        sfml-graphics
)