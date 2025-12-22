cmake_minimum_required(VERSION 3.15)
project(SearchEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
add_compile_options(-Wall -Wextra -Wpedantic -O3)

# Fetch nlohmann/json for JSON parsing
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Main library
add_library(search_engine
    src/document.cpp
    src/document_loader.cpp
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/ranker.cpp
    src/query_parser.cpp
    src/search_engine.cpp
    src/persistence.cpp
)

target_include_directories(search_engine PUBLIC include)

# Dependencies
find_package(Threads REQUIRED)
target_link_libraries(search_engine 
    PUBLIC Threads::Threads
    PUBLIC nlohmann_json::nlohmann_json
)

# Tests
enable_testing()
add_subdirectory(tests)

# Benchmarks
add_subdirectory(benchmarks)

# Examples
add_subdirectory(server)
