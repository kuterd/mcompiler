cmake_minimum_required(VERSION 3.2.0)

project(bad_compiler)

add_subdirectory(tests tests)     
add_executable(bad_compiler main.c list.c utils.c)

