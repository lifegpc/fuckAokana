cmake_minimum_required(VERSION 3.18)

project(fuckAokana)

if (MSVC)
    add_compile_options(/utf-8)
endif()

include(CheckIncludeFiles)

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    add_subdirectory(getopt)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/getopt")
endif()

set(ENABLE_ICONV OFF CACHE BOOL "Libiconv is not needed.")
add_subdirectory(utils)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/utils")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/incbin")

add_subdirectory(src)
