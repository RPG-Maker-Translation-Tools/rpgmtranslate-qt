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
