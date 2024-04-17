cmake_minimum_required(VERSION 3.9)

project(
    clap-rci
    VERSION 0.1
    LANGUAGES C CXX
)

message(
    STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}, v${CMAKE_CXX_COMPILER_VERSION}"
)

# OPTIONS
option(clap-rci_BUILD_AUTO_TESTS "Build tests" OFF)
option(clap-rci_BUILD_MAN_TESTS "Build tests" OFF)
option(WARNINGS_ARE_ERRORS "Error on Warning" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ${WARNINGS_ARE_ERRORS})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(cmake/utils.cmake)

# Library target
add_library(
    clap-rci STATIC
    include/clap-rci/coreplugin.h
    include/clap-rci/descriptor.h
    include/clap-rci/global.h
    include/clap-rci/mpmcqueue.h
    include/clap-rci/registry.h
    include/clap-rci/server.h
    include/clap-rci/entry.h
    src/coreplugin.cpp
    src/coreplugin_p.hpp
    src/coreplugin_p.cpp
    src/descriptor.cpp
    src/queueworker.hpp
    src/queueworker.cpp
    src/registry.cpp
    src/transportwatcher.hpp
    src/transportwatcher.cpp
    src/server/eventstreamreactor.hpp
    src/server/pluginservice.hpp
    src/server/pluginservice.cpp
    src/server/server.cpp
)

add_library(clap::rci ALIAS clap-rci)

# find packages
find_package(clap QUIET)
if(NOT clap_FOUND)
    add_subdirectory(3rdparty/clap/)
endif()

# Protobuf target generation, creates the target clap-rci-proto
initialize_proto_target(clap-rci "api/rci.proto")

# Includes
target_include_directories(clap-rci PUBLIC include/)
target_link_libraries(clap-rci PUBLIC clap clap-rci-proto)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        clap-rci
        PRIVATE -Wall
                -Werror
                -Wshadow
                -Wnon-virtual-dtor
                -Wcast-align
                -Wunused
                -Wpedantic
                -Wsign-conversion
                -Wnull-dereference
                -Wdouble-promotion
                -Wformat=2
                -Wimplicit-fallthrough
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(clap-rci PRIVATE /W4 /WX /wd4100 /wd4702)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
                      ${CMAKE_MODULE_PATH}
)

find_package(Sanitizers REQUIRED)
add_sanitizers(clap-rci)
add_sanitizers(clap-rci-proto)

add_subdirectory(tests/)
