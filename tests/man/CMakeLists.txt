add_library(testpluginone SHARED testpluginone.cpp)
target_link_libraries(testpluginone clap::rci)
clap_symlink(testpluginone)

add_library(testplugintwo SHARED testplugintwo.cpp)
target_link_libraries(testplugintwo clap::rci)
clap_symlink(testplugintwo)
