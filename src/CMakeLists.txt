cmake_minimum_required(VERSION 3.10)
project(tscclocksource)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR} src)
add_library(tscclocksource ${src})