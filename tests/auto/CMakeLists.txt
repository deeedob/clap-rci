# add_library(testplugin SHARED testplugin.cpp)
# target_link_libraries(testplugin clap::rci)
# set_target_properties(testplugin PROPERTIES PREFIX "")
# set_target_properties(testplugin PROPERTIES SUFFIX ".clap")

add_test_executable(tst_descriptor DEPENDENCIES clap::rci)
add_test_executable(tst_registry DEPENDENCIES clap::rci)
add_test_executable(tst_server DEPENDENCIES clap::rci)
add_test_executable(tst_mpmcqueue DEPENDENCIES clap::rci)
