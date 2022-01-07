find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_LZ4 QUIET IMPORTED_TARGET GLOBAL liblz4)
endif()

if (PC_LZ4_FOUND)
    set(LZ4_FOUND TRUE)
    set(LZ4_VERSION ${PC_LZ4_VERSION})
    set(LZ4_VERSION_STRING ${PC_LZ4_STRING})
    set(LZ4_LIBRARYS ${PC_LZ4_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(LZ4_INCLUDE_DIRS ${PC_LZ4_STATIC_INCLUDE_DIRS})
    else()
        set(LZ4_INCLUDE_DIRS ${PC_LZ4_INCLUDE_DIRS})
    endif()
    if (NOT TARGET LZ4::LZ4)
        add_library(LZ4::LZ4 ALIAS PkgConfig::PC_LZ4)
    endif()
else()
    message(FATAL_ERROR "failed.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4
    FOUND_VAR LZ4_FOUND
    REQUIRED_VARS
        LZ4_LIBRARYS
    VERSION_VAR LZ4_VERSION
)
