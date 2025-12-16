cmake_minimum_required(VERSION 3.15)

# Simple search example
add_executable(simple_search simple_search.cpp)
target_link_libraries(simple_search search_engine)

# Batch indexing example
add_executable(batch_indexing batch_indexing.cpp)
target_link_libraries(batch_indexing search_engine)

# REST server example
add_executable(rest_server rest_server.cpp)
target_link_libraries(rest_server search_engine)
