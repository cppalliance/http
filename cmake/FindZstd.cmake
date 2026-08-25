#
# Copyright (c) 2026 Mohammad Nejati
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/cppalliance/http
#

# Provides imported targets:
#   Zstd::Zstd

find_path(Zstd_INCLUDE_DIR NAMES "zstd.h")
find_library(Zstd_LIBRARY NAMES zstd libzstd zstd_static)

if(Zstd_INCLUDE_DIR AND EXISTS "${Zstd_INCLUDE_DIR}/zstd.h")
    file(STRINGS "${Zstd_INCLUDE_DIR}/zstd.h" Zstd_VERSION_LINES
        REGEX "^#define[ \t]+ZSTD_VERSION_(MAJOR|MINOR|RELEASE)[ \t]+[0-9]+")
    string(REGEX REPLACE ".*ZSTD_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" Zstd_VERSION_MAJOR "${Zstd_VERSION_LINES}")
    string(REGEX REPLACE ".*ZSTD_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" Zstd_VERSION_MINOR "${Zstd_VERSION_LINES}")
    string(REGEX REPLACE ".*ZSTD_VERSION_RELEASE[ \t]+([0-9]+).*" "\\1" Zstd_VERSION_RELEASE "${Zstd_VERSION_LINES}")
    set(Zstd_VERSION "${Zstd_VERSION_MAJOR}.${Zstd_VERSION_MINOR}.${Zstd_VERSION_RELEASE}")
    unset(Zstd_VERSION_LINES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zstd
    REQUIRED_VARS
        Zstd_INCLUDE_DIR
        Zstd_LIBRARY
    VERSION_VAR
        Zstd_VERSION
)

if(Zstd_FOUND)
    add_library(Zstd::Zstd UNKNOWN IMPORTED)
    set_target_properties(Zstd::Zstd PROPERTIES
        IMPORTED_LOCATION "${Zstd_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Zstd_INCLUDE_DIR}")
endif()

mark_as_advanced(
    Zstd_INCLUDE_DIR
    Zstd_LIBRARY)
