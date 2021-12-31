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
    extract.h
    extract.cpp
    extract_all.h
    extract_all.cpp
    main.cpp
)

if (WIN32)
    list(APPEND FUCKAOKANA_FILES "${CMAKE_CURRENT_BINARY_DIR}/fuckaokana.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fuckaokana.rc" PROPERTIES GENERATED TRUE)
endif()
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fuckaokana_version.h" PROPERTIES GENERATED TRUE)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_BINARY_DIR}/utils")

add_custom_target(fuckaokana-version ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/FUCKAOKANA_VERSION.cmake")
add_executable(fuckaokana "${FUCKAOKANA_FILES}")
add_dependencies(fuckaokana fuckaokana-version)
target_link_libraries(fuckaokana utils)
if (TARGET getopt)
    target_link_libraries(fuckaokana getopt)
endif()
install(TARGETS fuckaokana)
