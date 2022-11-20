cmake_minimum_required(VERSION 3.10)
project(master)
add_compile_options(-g)
include_directories(${CMAKE_CURRENT_LIST_DIR}/..)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR} src)

add_executable(master ${src})