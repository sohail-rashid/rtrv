cmake_minimum_required(VERSION 3.15)

# Google Test
add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest EXCLUDE_FROM_ALL)

# Test executable
add_executable(search_engine_tests
    tokenizer_test.cpp
    inverted_index_test.cpp
    ranker_test.cpp
    query_parser_test.cpp
    search_engine_test.cpp
    integration_test.cpp
)

target_link_libraries(search_engine_tests
    search_engine
    gtest
    gtest_main
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(search_engine_tests)
