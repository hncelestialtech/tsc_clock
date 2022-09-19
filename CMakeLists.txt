cmake_minimum_required(VERSION 3.12)
project(tscns)

if (NOT TSC_GLOBAL)
    set(TSC_GLOBAL FALSE)
endif ()

if (NOT LINK_INIT)
    set(LINK_INIT TRUE)
endif ()

add_subdirectory(src)
