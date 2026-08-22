# FindLibgit2.cmake
#
# Result variables:
#   Libgit2_FOUND
#
# Imported targets:
#   libgit2::libgit2
#
# Search order:
#   1. Upstream CMake config package (vcpkg, system install, etc.)
#   2. pkg-config (Linux, or any platform where it's available)
#   3. Manual find_path/find_library fallback (Windows/macOS without pkg-config)

# 1. CMake config package
find_package(libgit2 CONFIG QUIET)

if(NOT TARGET libgit2::libgit2 AND TARGET libgit2::libgit2package)
    # Debian's libgit2Targets.cmake sets INTERFACE_INCLUDE_DIRECTORIES to
    # .../include/git2 instead of its parent, so <git2/*.h> can't be found and,
    # worse, any system header that happens to share a filename with something
    # inside git2/ (e.g. stdint.h, a bundled MSVC-only compat shim that's a
    # silent no-op under GCC/Clang) shadows the real one. Strip the stray
    # /git2 suffix on the upstream-provided target before wrapping it.
    get_target_property(_libgit2_pkg_includes libgit2::libgit2package INTERFACE_INCLUDE_DIRECTORIES)
    if(_libgit2_pkg_includes)
        set(_libgit2_fixed_includes "")
        foreach(_dir ${_libgit2_pkg_includes})
            get_filename_component(_dir_name "${_dir}" NAME)
            if(_dir_name STREQUAL "git2")
                get_filename_component(_dir "${_dir}" DIRECTORY)
            endif()
            get_filename_component(_dir "${_dir}" ABSOLUTE)
            list(APPEND _libgit2_fixed_includes "${_dir}")
        endforeach()
        set_target_properties(libgit2::libgit2package PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_libgit2_fixed_includes}"
        )
    endif()
    unset(_libgit2_pkg_includes)
    unset(_libgit2_fixed_includes)

    add_library(libgit2::libgit2 INTERFACE IMPORTED)
    target_link_libraries(libgit2::libgit2 INTERFACE libgit2::libgit2package)
endif()

# 2. pkg-config, wherever it exists (not just Linux)
if(NOT TARGET libgit2::libgit2)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
        if(TARGET PkgConfig::LIBGIT2)
            add_library(libgit2::libgit2 INTERFACE IMPORTED)
            target_link_libraries(libgit2::libgit2 INTERFACE PkgConfig::LIBGIT2)
        endif()
    endif()
endif()

# 3. Manual fallback (e.g. Windows/macOS without pkg-config, no CMake config installed)
if(NOT TARGET libgit2::libgit2)
    find_path(LIBGIT2_INCLUDE_DIR
        NAMES git2.h
        HINTS ${LIBGIT2_ROOT} ${LIBGIT2_ROOT}/include
        PATHS /usr/include /usr/local/include
    )

    find_library(LIBGIT2_LIBRARY
        NAMES git2 libgit2
        HINTS ${LIBGIT2_ROOT} ${LIBGIT2_ROOT}/lib ${LIBGIT2_ROOT}/bin
        PATHS /usr/lib64 /usr/lib
    )

    if(LIBGIT2_INCLUDE_DIR AND LIBGIT2_LIBRARY)
        add_library(libgit2::libgit2 UNKNOWN IMPORTED)
        set_target_properties(libgit2::libgit2 PROPERTIES
            IMPORTED_LOCATION "${LIBGIT2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBGIT2_INCLUDE_DIR}"
        )
    endif()
endif()

if(TARGET libgit2::libgit2)
    set(Libgit2_FOUND TRUE)
else()
    set(Libgit2_FOUND FALSE)
    if(Libgit2_FIND_REQUIRED)
        message(FATAL_ERROR "libgit2 not found via CMake config, pkg-config, or manual search. Set -DLIBGIT2_ROOT=<path>.")
    endif()
endif()
