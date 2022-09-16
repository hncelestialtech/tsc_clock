cmake_minimum_required(VERSION 3.12)
project(tsctest)

add_definitions("-O0 -g -march=native")

include_directories(${CMAKE_CURRENT_LIST_DIR}/../src)
link_directories(${CMAKE_CURRENT_LIST_DIR}/../src/build)

add_executable(latency_test ${CMAKE_CURRENT_SOURCE_DIR}/test_latency.cc)
target_link_libraries(latency_test tscns pthread)

add_executable(st_test ${CMAKE_CURRENT_SOURCE_DIR}/test_tsc_st.cc)
target_link_libraries(st_test tscns pthread)

add_executable(mp_test ${CMAKE_CURRENT_SOURCE_DIR}/test_tsc_mp.cc)
target_link_libraries(mp_test tscns pthread)