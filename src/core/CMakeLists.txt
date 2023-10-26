set(core_src
    logging.h logging.cpp
    global.h
    timestamp.h
    processhandle.h
    semaphores.h
    blkqueue.h
    reducequeue.h
)

message(STATUS "core_src: ${core_src}")
add_library(core SHARED ${core_src})

target_link_libraries(core PUBLIC spdlog::spdlog)

set(src_dir "${PROJECT_SOURCE_DIR}/src")
target_include_directories(core PUBLIC "$<BUILD_INTERFACE:${src_dir}>")
