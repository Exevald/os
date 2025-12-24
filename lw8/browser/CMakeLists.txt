add_executable(
        browser
        InvertedIndex.cpp
        Tokenizer.cpp
        SearchEngine.cpp
        main.cpp
        SnippetExtractor.cpp
        DocumentStorage.cpp
        PersistentStorage.cpp
        HttpServer.cpp
)

target_link_libraries(browser PRIVATE thread_pool_lib)
target_include_directories(browser PRIVATE ${Boost_INCLUDE_DIRS})