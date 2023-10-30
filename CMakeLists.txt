cmake_minimum_required(VERSION 3.9)

project(clap-remote
    VERSION 0.0.1
    LANGUAGES C CXX
)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}, v${CMAKE_CXX_COMPILER_VERSION}")

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

option(clap-remote_BUILD_TESTS "Build tests" OFF)

set(clap-remote_PROTO "${CMAKE_CURRENT_LIST_DIR}/api/v0/api.proto" CACHE STRING "proto server api" FORCE)
mark_as_advanced(clap-remote_PROTO)
set(clap-remote_PROTO_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/api/v0" CACHE STRING "proto include path" FORCE)
mark_as_advanced(clap-remote_INCLUDE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Add sanitizers support
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
include(cmake/utils.cmake)

add_subdirectory(src)

if (clap-remote_BUILD_TESTS)
    add_subdirectory(tests)
endif()

