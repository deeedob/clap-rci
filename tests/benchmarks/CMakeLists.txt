# (c) 2023 Dennis Oberst
# all rights reserved.

add_executable(bench_clap-remote bench_clap-remote.cpp)
target_link_libraries(bench_clap-remote PRIVATE clap-remote)
