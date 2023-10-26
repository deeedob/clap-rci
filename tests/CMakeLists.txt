find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)

add_subdirectory(auto/core)
add_subdirectory(auto/server)
#add_subdirectory(auto/plugin)
add_subdirectory(benchmarks)
#add_subdirectory(ideas)
