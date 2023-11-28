# (c) 2023 Dennis Oberst
# all rights reserved.

include(CTest)
include(Catch)

add_test_executable(tst_server DEPENDENCIES clap-rci)
add_test_executable(tst_serverctrl DEPENDENCIES clap-rci)
add_test_executable(tst_cqeventhandler DEPENDENCIES clap-rci)
