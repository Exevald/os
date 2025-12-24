add_executable(
        mt-search-engine
        Tokenizer.cpp
        InvertedIndex.cpp
        SearchEngine.cpp
        main.cpp
)

target_link_libraries(mt-search-engine PRIVATE thread_pool_lib)