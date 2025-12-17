cmake_minimum_required(VERSION 3.15)


# REST server example
add_executable(rest_server rest_server.cpp)
target_link_libraries(rest_server search_engine)

# Interactive CLI server
add_executable(interactive_server rest_server_interactive.cpp)
target_link_libraries(interactive_server search_engine)

# REST server with Drogon (optional - requires Drogon framework)
find_package(Drogon CONFIG QUIET)
if(Drogon_FOUND)
    add_executable(rest_server_drogon rest_server_drogon.cpp)
    target_link_libraries(rest_server_drogon search_engine Drogon::Drogon)
    message(STATUS "Drogon found - building rest_server_drogon")
else()
    message(STATUS "Drogon not found - skipping rest_server_drogon (install with: brew install drogon or vcpkg install drogon)")
endif()

# REST server with Crow (optional - requires Crow framework)
find_package(Crow CONFIG QUIET)
if(Crow_FOUND)
    add_executable(rest_server_crow rest_server_crow.cpp)
    target_link_libraries(rest_server_crow search_engine Crow::Crow)
    message(STATUS "Crow found - building rest_server_crow")
else()
    message(STATUS "Crow not found - skipping rest_server_crow (install with: brew install crow)")
endif()

