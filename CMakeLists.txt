cmake_minimum_required(VERSION 3.12)
project(tscns)

if (NOT TSC_GLOBAL)
    set(TSC_GLOBAL FALSE)
else ()
    set(TSC_GLOBAL TRUE)
endif ()

if (NOT LINK_INIT)
    set(LINK_INIT TRUE)
endif ()

if (NOT SYNC_CALIBRATE)
    set(SYNC_CALIBRATE TRUE)
else ()
    set(SYNC_CALIBRATE FALSE)
endif ()

add_subdirectory(src)