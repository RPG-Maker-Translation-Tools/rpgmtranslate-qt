# Findqjs.cmake
#
# Result variables:
#   qjs_FOUND
#
# Imported targets:
#   qjs
#
# Search order:
#   1. Upstream CMake config package (exports the `qjs` target directly, e.g. Fedora, Arch)
#   2. pkg-config - quickjs-ng.pc (Alpine) or qjs.pc, wherever pkg-config finds it
#   3. Manual find_path/find_library fallback (distros shipping only the header/lib)

# 1. CMake config package
find_package(qjs CONFIG QUIET)

# 2. pkg-config
if(NOT TARGET qjs)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(QJS QUIET IMPORTED_TARGET quickjs-ng)
        if(NOT TARGET PkgConfig::QJS)
            pkg_check_modules(QJS QUIET IMPORTED_TARGET qjs)
        endif()
        if(TARGET PkgConfig::QJS)
            add_library(qjs INTERFACE IMPORTED)
            target_link_libraries(qjs INTERFACE PkgConfig::QJS)
        endif()
    endif()
endif()

# 3. Manual fallback (distros whose package ships only the header/lib, no config or .pc)
if(NOT TARGET qjs)
    find_path(QJS_INCLUDE_DIR
        NAMES quickjs.h
        HINTS ${QJS_ROOT} ${QJS_ROOT}/include
        PATHS /usr/include /usr/local/include
    )

    find_library(QJS_LIBRARY
        NAMES qjs libqjs
        HINTS ${QJS_ROOT} ${QJS_ROOT}/lib ${QJS_ROOT}/bin
        PATHS /usr/lib64 /usr/lib
    )

    if(QJS_INCLUDE_DIR AND QJS_LIBRARY)
        add_library(qjs UNKNOWN IMPORTED)
        set_target_properties(qjs PROPERTIES
            IMPORTED_LOCATION "${QJS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${QJS_INCLUDE_DIR}"
        )
    endif()
endif()

if(TARGET qjs)
    set(qjs_FOUND TRUE)
else()
    set(qjs_FOUND FALSE)
    if(qjs_FIND_REQUIRED)
        message(FATAL_ERROR "qjs (quickjs-ng) not found via CMake config, pkg-config, or manual search. Set -DQJS_ROOT=<path>.")
    endif()
endif()
