# (c) 2023 Dennis Oberst
# all rights reserved.

include(CTest)
include(Catch)

add_test_executable(tst_server DEPENDENCIES clap-remote)
add_test_executable(tst_serverctrl DEPENDENCIES clap-remote)
add_test_executable(tst_cqeventhandler DEPENDENCIES clap-remote)
