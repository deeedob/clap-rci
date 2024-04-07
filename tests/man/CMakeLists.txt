add_library(testpluginone SHARED testpluginone.cpp)
add_sanitizers(testpluginone)
target_link_libraries(testpluginone clap::rci)
clap_symlink(testpluginone)

add_library(testplugintwo SHARED testplugintwo.cpp)
add_sanitizers(testplugintwo)
target_link_libraries(testplugintwo clap::rci)
clap_symlink(testplugintwo)

add_executable(testserver testserver.cpp)
target_link_libraries(testserver PRIVATE clap::rci)
