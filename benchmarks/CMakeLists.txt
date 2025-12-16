cmake_minimum_required(VERSION 3.15)

# Google Benchmark
if(EXISTS ${CMAKE_SOURCE_DIR}/external/benchmark)
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/benchmark ${CMAKE_BINARY_DIR}/benchmark EXCLUDE_FROM_ALL)
else()
    message(WARNING "Google Benchmark not found. Skipping benchmarks. To enable, run: git submodule add https://github.com/google/benchmark.git external/benchmark")
    return()
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
