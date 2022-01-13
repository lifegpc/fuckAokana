list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(ENABLE_FFMPEG "Use ffmpeg's library to merge CG." ON)
option(ENABLE_LZ4 "Use liblz4 to decompress data in UnityFS archive." ON)

if (ENABLE_FFMPEG)
    find_package(AVFORMAT 58)
    find_package(AVCODEC 58)
    find_package(AVUTIL 56)
    find_package(SWSCALE 3.9)
    find_package(AVFILTER 7)
    if (AVFORMAT_FOUND AND AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND AND AVFILTER_FOUND)
        set(HAVE_FFMPEG 1)
    endif()
endif()
if (ENABLE_LZ4)
    find_package(LZ4)
    if (LZ4_FOUND)
        set(HAVE_LZ4 1)
    endif()
endif()

if (NOT HAVE_FFMPEG)
    message(WARNING "ffmpeg not found. Extract and merge CG will not works.")
endif()

include(CheckSymbolExists)
include(GNUInstallDirs)

if (WIN32)
    check_symbol_exists(printf_s "stdio.h" HAVE_PRINTF_S)
endif()

configure_file(fuckaokana_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/fuckaokana_config.h")

set(
    FUCKAOKANA_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/fuckaokana_config.h"
    "${CMAKE_CURRENT_BINARY_DIR}/fuckaokana_version.h"
    archive.h
    archive.cpp
    archive_list.h
    archive_list.cpp
    embed_data.h
    embed_data.c
    extract.h
    extract.cpp
    extract_all.h
    extract_all.cpp
    main.cpp
    unityfs/asset.h
    unityfs/asset.cpp
    unityfs/assetref.h
    unityfs/assetref.cpp
    unityfs/block_storage.h
    unityfs/block_storage.cpp
    unityfs/decompress.h
    unityfs/decompress.cpp
    unityfs/dump.h
    unityfs/dump.cpp
    "unityfs/environment.h"
    "unityfs/environment.cpp"
    unityfs/object/boolean.h
    unityfs/object/boolean.cpp
    unityfs/object/number.h
    unityfs/object/number.cpp
    unityfs/object.h
    unityfs/object.cpp
    unityfs/object_info.h
    unityfs/object_info.cpp
    "unityfs/type.h"
    "unityfs/type.cpp"
    unityfs/unityfs.h
    unityfs/unityfs.cpp
    unityfs/unityfs_private.h
)

if (HAVE_FFMPEG)
    list(APPEND FUCKAOKANA_FILES
        extract_cgs.h
        extract_cgs.cpp
        merge_cg.h
        merge_cg.c
    )
endif()

if (WIN32)
    list(APPEND FUCKAOKANA_FILES "${CMAKE_CURRENT_BINARY_DIR}/fuckaokana.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fuckaokana.rc" PROPERTIES GENERATED TRUE)
endif()
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fuckaokana_version.h" PROPERTIES GENERATED TRUE)

if (MSVC)
    list(APPEND FUCKAOKANA_FILES "${CMAKE_CURRENT_BINARY_DIR}/data.c")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/data.c" PROPERTIES GENERATED TRUE)
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_BINARY_DIR}/utils")

add_custom_target(fuckaokana-version ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/FUCKAOKANA_VERSION.cmake")
if (MSVC)
    add_executable(incbin "../incbin/incbin.c")
    add_custom_target(fuckaokana-data "$<TARGET_FILE:incbin>" "${CMAKE_CURRENT_SOURCE_DIR}/embed_data.c" -o "${CMAKE_CURRENT_BINARY_DIR}/data.c" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_dependencies(fuckaokana-data incbin)
endif()
add_executable(fuckaokana "${FUCKAOKANA_FILES}")
add_dependencies(fuckaokana fuckaokana-version)
if (MSVC)
    add_dependencies(fuckaokana fuckaokana-data)
endif()
target_link_libraries(fuckaokana utils)
if (TARGET getopt)
    target_link_libraries(fuckaokana getopt)
endif()
install(TARGETS fuckaokana)
if (HAVE_FFMPEG)
    target_link_libraries(fuckaokana AVCODEC::AVCODEC)
    target_link_libraries(fuckaokana AVFILTER::AVFILTER)
    target_link_libraries(fuckaokana AVFORMAT::AVFORMAT)
    target_link_libraries(fuckaokana AVUTIL::AVUTIL)
    target_link_libraries(fuckaokana SWSCALE::SWSCALE)
endif()
if (HAVE_LZ4)
    target_link_libraries(fuckaokana LZ4::LZ4)
endif()
