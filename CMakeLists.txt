cmake_minimum_required(VERSION 3.16)
project(A3 C)

set(CMAKE_C_STANDARD 11)
add_executable(A3 A3.c)
target_link_libraries(A3 m)