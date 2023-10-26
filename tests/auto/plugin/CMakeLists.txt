# (c) 2023 Dennis Oberst
# all rights reserved.

find_package(Qt6 COMPONENTS Core Gui)

if(NOT Qt6_FOUND)
    message("Qt6 not found, skipping plugin tests")
    return()
endif()

add_library(basichost STATIC basichost.cpp)
target_link_libraries(basichost PUBLIC clap-remote Qt6::Core Qt6::Gui)

add_test_executable(tst_plugin DEPENDENCIES basichost)
