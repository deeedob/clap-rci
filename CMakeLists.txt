cmake_minimum_required(VERSION 3.9)

project(clap-remote
    VERSION 0.0.1
    LANGUAGES C CXX
)

find_package(Protobuf 3.21 CONFIG REQUIRED)
find_package(gRPC 1.50 REQUIRED)

option(ADD_TESTS "Add tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(proto_file "${CMAKE_CURRENT_LIST_DIR}/api/v0/api.proto" CACHE INTERNAL "Full proto file path")
set(proto_include "${CMAKE_CURRENT_LIST_DIR}/api/v0" CACHE INTERNAL "Proto include path")

# Add sanitizers support
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
include(cmake/utils.cmake)

add_subdirectory(src)

if (ADD_TESTS)
    add_subdirectory(tests)
endif()

