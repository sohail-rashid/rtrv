cmake_minimum_required(VERSION 3.15)

# Simple search example
add_executable(simple_search simple_search.cpp)
target_link_libraries(simple_search search_engine)

# Batch indexing example
add_executable(batch_indexing batch_indexing.cpp)
target_link_libraries(batch_indexing search_engine)

# Skip pointer demonstration
add_executable(skip_pointer_demo skip_pointer_demo.cpp)
target_link_libraries(skip_pointer_demo search_engine)
