cmake_minimum_required(VERSION 3.15)

# Google Benchmark
if(EXISTS ${CMAKE_SOURCE_DIR}/external/benchmark)
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/benchmark ${CMAKE_BINARY_DIR}/benchmark EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()

# Benchmark executables
add_executable(indexing_benchmark indexing_benchmark.cpp)
target_link_libraries(indexing_benchmark search_engine benchmark::benchmark)

add_executable(search_benchmark search_benchmark.cpp)
target_link_libraries(search_benchmark search_engine benchmark::benchmark)

add_executable(concurrent_benchmark concurrent_benchmark.cpp)
target_link_libraries(concurrent_benchmark search_engine benchmark::benchmark)

add_executable(memory_benchmark memory_benchmark.cpp)
target_link_libraries(memory_benchmark search_engine benchmark::benchmark)
add_executable(tokenizer_simd_benchmark tokenizer_simd_benchmark.cpp)
target_link_libraries(tokenizer_simd_benchmark search_engine benchmark::benchmark)