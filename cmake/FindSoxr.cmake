# FindSoxr.cmake — locate the libsoxr audio resampling library.
#
# Imported target: Soxr::Soxr
# Result variables: Soxr_FOUND, SOXR_INCLUDE_DIR, SOXR_LIBRARY

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(_SOXR QUIET soxr)
endif()

find_path(SOXR_INCLUDE_DIR
    NAMES soxr.h
    HINTS ${_SOXR_INCLUDE_DIRS}
)

find_library(SOXR_LIBRARY
    NAMES soxr
    HINTS ${_SOXR_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Soxr
    REQUIRED_VARS SOXR_LIBRARY SOXR_INCLUDE_DIR
)

if(Soxr_FOUND AND NOT TARGET Soxr::Soxr)
    add_library(Soxr::Soxr UNKNOWN IMPORTED)
    set_target_properties(Soxr::Soxr PROPERTIES
        IMPORTED_LOCATION             "${SOXR_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SOXR_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SOXR_INCLUDE_DIR SOXR_LIBRARY)
