add_subdirectory(3rdparty/spdlog EXCLUDE_FROM_ALL)
target_compile_options(spdlog PRIVATE "-w")

set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(clap-remote_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "" FORCE)

add_subdirectory(core)
add_subdirectory(server) # create proto-objects target

set(server_src
    server/server.h server/server.cpp
    server/serverctrl.h server/serverctrl.cpp
    server/cqeventhandler.h server/cqeventhandler.cpp
    server/shareddata.h server/shareddata.cpp
    server/tags/eventtag.h server/tags/eventtag.cpp
    server/tags/clienteventcall.h server/tags/clienteventcall.cpp
    server/tags/clientparamcall.h server/tags/clientparamcall.cpp
    server/tags/servereventstream.h server/tags/servereventstream.cpp
)

set(plugin_src
    plugin/coreplugin.h plugin/coreplugin.cpp
    plugin/settings.h plugin/context.h
    plugin/modules/module.h plugin/modules/module.cpp
    plugin/parameter/parameter.h plugin/parameter/parameter.cpp
    plugin/parameter/decibel_valuetype.h plugin/parameter/decibel_valuetype.cpp
    plugin/parameter/valuetype.h
)

list(APPEND clap-remote_SRC ${server_src} ${plugin_src})

set(target ${PROJECT_NAME})
add_library(${target} STATIC ${clap-remote_SRC})
add_library(${target}::${target} ALIAS ${target})
target_link_libraries(${target} PUBLIC proto-objects core spdlog)

# Include all 3rdparty libraries and make them available to the project
target_include_directories(${target} SYSTEM
    PUBLIC 3rdparty/clap/include
    PUBLIC 3rdparty/clap-helpers/include
    PUBLIC 3rdparty/farbot/include
    PUBLIC 3rdparty/crill/include
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${target} PRIVATE /W4 /WX /wd4100 /wd4702)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${target} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
    target_compile_definitions(${target} PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()

