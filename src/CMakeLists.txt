set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../output/lib)

set(CMAKE_CXX_STANDARD 17)

add_definitions("-Wall")

if (${TSC_GLOBAL})
     add_compile_definitions(TSC_GLOBAL)
     set(tsc_src ${CMAKE_CURRENT_SOURCE_DIR}/tsc_clock_mt.cc)
else ()
     set(tsc_src ${CMAKE_CURRENT_SOURCE_DIR}/tsc_clock_st.cc)
endif ()

if (${LINK_INIT})
     add_compile_definitions(LINK_INIT)
endif ()

if (${SYNC_CALIBRATE})
     add_compile_definitions(SYNC_CALIBRATE)
endif ()

if (${DEBUG_PRINT})
     add_compile_definitions(DEBUG_PRINT)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "No CMAKE_BUILD_TYPE selected, config default value to ${CMAKE_BUILD_TYPE}")
endif ()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions("-g -O0")
else ()
     add_definitions("-O3")
     if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          #required by butil/crc32.cc to boost performance for 10x
          if ((CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)") AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4))
               add_definitions("-msse4 -msse4.2")
          elseif ((CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
               # segmentation fault in libcontext
                add_definitions("-fno-gcse")
          endif ()
          if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0))
               add_definitions("-Wno-aligned-new")
          endif ()
     endif ()
     if (FASTMATH)
          add_definitions(-ffast-math)
     endif ()
endif ()

add_library(tscns STATIC ${tsc_src})
# add_library(tscns_s SHARED ${CMAKE_CURRENT_SOURCE_DIR}/tsc_clock.cc)
target_link_libraries(tscns pthread)
# target_link_libraries(tscns_s pthread)

# set_target_properties(tscns_s PROPERTIES VERSION 0.0.0 SOVERSION 0)