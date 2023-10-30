set(core_src
    target.cpp
    logging.h 
    global.h
    timestamp.h
    processhandle.h
    processhandle.cpp
    blkringqueue.h
)

message(STATUS "core_src: ${core_src}")
# TODO: Is it possible to create a dll on windows?
add_library(core STATIC ${core_src})

target_link_libraries(core PUBLIC spdlog::spdlog)
if(WIN32)
    target_link_libraries(core PRIVATE kernel32)
endif()
target_include_directories(core PUBLIC "${PROJECT_SOURCE_DIR}/src")
