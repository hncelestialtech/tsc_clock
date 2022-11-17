cmake_minimum_required(VERSION 3.10)
project(compiler)
add_compile_options(-g)
add_executable(tsc_compile cpuflags.c main.c)