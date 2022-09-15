cmake_minimum_required(VERSION 3.12)
project(tscns C)

add_definitions("-Ofast -march=native")

add_library(tscns STATIC ${CMAKE_CURRENT_SOURCE_DIR}/tsc_clock.c)
target_link_libraries(tscns pthread)