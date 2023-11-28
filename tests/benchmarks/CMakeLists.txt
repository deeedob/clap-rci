# (c) 2023 Dennis Oberst
# all rights reserved.

add_executable(bench_clap_rci bench_clap_rci.cpp)
target_link_libraries(bench_clap_rci PRIVATE clap-remote)

add_subdirectory(clients/)
add_dependencies(bench_clap_rci client-cpp)
