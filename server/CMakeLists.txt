cmake_minimum_required(VERSION 3.15)


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

