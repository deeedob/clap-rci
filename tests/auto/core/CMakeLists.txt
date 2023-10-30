# (c) 2023 Dennis Oberst
# all rights reserved.

add_test_executable(tst_blkqueue DEPENDENCIES core)
add_test_executable(tst_timestamp DEPENDENCIES core)

add_test_executable(tst_processhandle DEPENDENCIES core)
add_executable(executable executable.cpp)
add_dependencies(tst_processhandle executable)
