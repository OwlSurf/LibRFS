cmake_minimum_required(VERSION 3.16)
project(RingFileSystem C)

add_library(rfs STATIC Src/ring_file_system.c)
target_include_directories(rfs PUBLIC Inc)

add_executable(host_demo examples/host_demo.c)
target_link_libraries(host_demo PRIVATE rfs)
