# 3.25 is a modern baseline - has LINUX macro and other stuff
cmake_minimum_required(VERSION 3.25)

# Project definition
set(PROJECT_NAME rpgmtranslate)
set(PROJECT_VERSION "1.0.0")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

option(ENABLE_NUSPELL
    "Nuspell integration - allows using dictionaries for spell-checking."
    ON
)

option(ENABLE_LANGUAGETOOL
    "LanguageTool - allows using LanguageTool for text linting."
    ON
)

option(ENABLE_LIBGIT2
    "libgit2 - allows using built-in Git client for managing repositories."
    ON
)

option(ENABLE_ASSET_PLAYBACK
    "Allows playing video and audio assets directly in the application. Requires FFmpeg."
    ON
)

option(ENABLE_JSON_HIGHLIGHTING
    "Only works if ENABLE_SYNTAX_HIGHLIGTHING=ON. Enable syntax highlighting for JSON data."
    ON
)
option(ENABLE_JS_HIGHLIGTHING
    "Only works if ENABLE_SYNTAX_HIGHLIGTHING=ON. Enable syntax highlighting for JavaScript data."
    ON
)
option(ENABLE_RUBY_HIGHLIGHTING
    "Only works if ENABLE_SYNTAX_HIGHLIGTHING=ON. Enable syntax highlighting for Ruby-lang data."
    ON
)

option(ENABLE_ASAN
    "Enable AddressSanitizer (ASan). If using MSVC, this will override runtime to release version."
    OFF
)
option(ENABLE_UBSAN
    "Enable UndefinedBehaviorSanitizer (UBSan)."
    OFF
)
option(ENABLE_LSAN
    "Enable LeakSanitizer (LSan)."
    OFF
)

option(MSVC_STATIC_RUNTIME
    "Link MSVC runtime statically (MT/MTd vs MD/MDd)."
    ON
)
option(ENABLE_PIC
    "Enable Position Independent Code."
    ON
)
option(ENABLE_LTO
    "Enable link-time optimizations."
    OFF
)

# Set standards for C and C++
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory for the compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)

# Enable Qt6 automatic processing (UIC, MOC, RCC)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (ENABLE_PIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

set(_san_flags "")

if (ENABLE_ASAN)
    string(APPEND _san_flags ",address")
endif()

if (ENABLE_UBSAN)
    string(APPEND _san_flags ",undefined")
endif()

if (ENABLE_LSAN)
    string(APPEND _san_flags ",leak")
endif()

if (_san_flags)
    if (MSVC)
        # MSVC debug runtime is incompatible with sanitizers. Let sanitizers take priority.
        if (MSVC_STATIC_RUNTIME)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
        endif()
    endif()

    string(REGEX REPLACE "^," "" _san_flags "${_san_flags}")
    set(_san_flags "-fsanitize=${_san_flags}")

    set(CMAKE_CXX_FLAGS_DEBUG    "${CMAKE_CXX_FLAGS_DEBUG}    -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls ${_san_flags} -fsanitize-address-use-after-scope")
    set(CMAKE_C_FLAGS_DEBUG      "${CMAKE_C_FLAGS_DEBUG}      -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls ${_san_flags} -fsanitize-address-use-after-scope")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} ${_san_flags} -fsanitize-address-use-after-scope")
else()
    if (MSVC)
        if (MSVC_STATIC_RUNTIME)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
        endif()
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Gui Svg Network LinguistTools)
find_package(LibArchive REQUIRED)

include(FindPackageHandleStandardArgs)

if (ENABLE_ASSET_PLAYBACK)
    find_path(FFMPEG_INCLUDE_DIR
        NAMES libavcodec/avcodec.h
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES ffmpeg
    )

    foreach(_lib avfilter avformat avcodec avutil swresample swscale)
        string(TOUPPER ${_lib} _LIB_UPPER)

        find_library(${_LIB_UPPER}_LIBRARY
            NAMES ${_lib}
            HINTS ${FFMPEG_ROOT}
            PATHS /usr/lib64 /usr/lib
        )
    endforeach()

    find_package_handle_standard_args(FFmpeg
        REQUIRED_VARS
            FFMPEG_INCLUDE_DIR
            AVFILTER_LIBRARY
            AVFORMAT_LIBRARY
            AVCODEC_LIBRARY
            AVUTIL_LIBRARY
            SWRESAMPLE_LIBRARY
            SWSCALE_LIBRARY
        FAIL_MESSAGE "FFmpeg not found. Install it or set -DFFMPEG_ROOT=<path>"
    )

    if (NOT FFmpeg_FOUND)
        message(FATAL_ERROR "Aborting: FFmpeg is required for ENABLE_ASSET_PLAYBACK.")
    endif()
endif()

if (ENABLE_NUSPELL)
    find_package(Nuspell REQUIRED)
endif()

if (ENABLE_LIBGIT2)
    find_package(libgit2)
endif()

# Configure the project version header
configure_file(
    src/version/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Collect all source files recursively
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
    src/*.ui
    src/*.ts
)

# Collect translation source files
file(GLOB TRANSLATION_TS_FILES translations/*)

# Create the executable, but do not yet finalize it
set(WINDOWS_APPLICATION_ICON "resources/${PROJECT_NAME}-logo.rc")

# Build zmij, it cannot be used with only the header
add_library(zmij OBJECT src/3rdparty/zmij.cc)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${SOURCES}
    ${QM_FILES}
    ${WINDOWS_APPLICATION_ICON}
)

qt_add_translations(
    ${PROJECT_NAME}
    TS_FILES ${TRANSLATION_TS_FILES}
    SOURCES ${SOURCES}
    QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
)

qt_add_resources(resources.qrc)
target_sources(${PROJECT_NAME} PRIVATE resources.qrc)

# Build mode macros for usage in C++
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_BUILD>
        $<$<CONFIG:Release>:RELEASE_BUILD>

        $<$<BOOL:${ENABLE_NUSPELL}>:ENABLE_NUSPELL>
        $<$<BOOL:${ENABLE_LANGUAGETOOL}>:ENABLE_LANGUAGETOOL>
        $<$<BOOL:${ENABLE_LIBGIT2}>:ENABLE_LIBGIT2>
        $<$<BOOL:${ENABLE_ASSET_PLAYBACK}>:ENABLE_ASSET_PLAYBACK>
        $<$<BOOL:${ENABLE_JSON_HIGHLIGHTING}>:ENABLE_JSON_HIGHLIGHTING>
        $<$<BOOL:${ENABLE_JS_HIGHLIGHTING}>:ENABLE_JS_HIGHLIGHTING>
        $<$<BOOL:${ENABLE_RUBY_HIGHLIGHTING}>:ENABLE_RUBY_HIGHLIGHTING>
)

# Definitions for better code
add_definitions(
    -DQT_DISABLE_DEPRECATED_UP_TO=0x060802
    -DQT_ENABLE_STRICT_MODE_UP_TO=0x060802
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_QASCONST
    -DQT_NO_QEXCHANGE
    -DQT_NO_QSNPRINTF
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_FOREACH
    -DQT_NO_CONTEXTLESS_CONNECT
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    -DQT_USE_NODISCARD_FILE_OPEN
)

# Include file/line information in debug messages
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_MESSAGELOGCONTEXT)

# Set properties (e.g., no console window for Windows app)
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

# Include the directory for generated files (e.g., version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include additional source subdirectories
file(GLOB SUBDIRS LIST_DIRECTORIES true src/*)
file(GLOB ICONS icons/*)

# Include paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBDIRS}
    ${FFMPEG_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Svg
    LibArchive::LibArchive
    zmij
)

if (ENABLE_ASSET_PLAYBACK)
    # FFmpeg libs
    target_link_libraries(${PROJECT_NAME} PRIVATE
        avfilter
        avformat
        avcodec
        avutil
        swresample
        swscale
    )
endif()

if (ENABLE_NUSPELL)
    target_link_libraries(${PROJECT_NAME} PRIVATE Nuspell::nuspell)
endif()

if (ENABLE_LIBGIT2)
    target_link_libraries(${PROJECT_NAME} PRIVATE libgit2::libgit2package)
endif()

if (LINUX)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::WaylandClient
    )
endif()

# Build the Rust library
set(RUST_CRATE_DIR "${CMAKE_SOURCE_DIR}/rust")
set(GENERATED_DIR  "${CMAKE_CURRENT_BINARY_DIR}")
set(RPGMTRANSLATE_RUST_HEADER "${GENERATED_DIR}/rpgmtranslate.h")

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6.1
)
FetchContent_MakeAvailable(Corrosion)

set(RPGMTRANSLATE_RUST_FEATURES "")

if (ENABLE_LANGUAGETOOL)
    list(APPEND RPGMTRANSLATE_RUST_FEATURES "languagetool")
endif()

if (ENABLE_JSON_HIGHLIGHTING)
    list(APPEND RPGMTRANSLATE_RUST_FEATURES "json-highlighting")
endif()

if (ENABLE_JS_HIGHLIGHTING)
    list(APPEND RPGMTRANSLATE_RUST_FEATURES "js-highlighting")
endif()

if (ENABLE_RUBY_HIGHLIGHTING)
    list(APPEND RPGMTRANSLATE_RUST_FEATURES "ruby-highlighting")
endif()

corrosion_import_crate(
    MANIFEST_PATH "${RUST_CRATE_DIR}/Cargo.toml"
    FEATURES ${RPGMTRANSLATE_RUST_FEATURES}
)

if (LINUX)
    find_package(OpenSSL REQUIRED)
    corrosion_link_libraries(rpgmtranslate_rs OpenSSL::Crypto OpenSSL::SSL)
endif()

file(GLOB_RECURSE RUST_SOURCES
    CONFIGURE_DEPENDS
    "${RUST_CRATE_DIR}/src/*.rs"
    "${RUST_CRATE_DIR}/Cargo.toml"
    "${RUST_CRATE_DIR}/Cargo.lock"
    "${RUST_CRATE_DIR}/cbindgen.toml"
)

add_custom_command(
    OUTPUT "${RPGMTRANSLATE_RUST_HEADER}"
    COMMAND cbindgen
            --config "${RUST_CRATE_DIR}/cbindgen.toml"
            --output "${RPGMTRANSLATE_RUST_HEADER}"
    DEPENDS ${RUST_SOURCES}
    WORKING_DIRECTORY "${RUST_CRATE_DIR}"
    VERBATIM
)

add_custom_target(rpgmtranslate_rust_header
    DEPENDS "${RPGMTRANSLATE_RUST_HEADER}"
)

add_dependencies(${PROJECT_NAME} rpgmtranslate_rust_header)
target_include_directories(${PROJECT_NAME} PRIVATE "${GENERATED_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE rpgmtranslate_rs)

# Install the built executable
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
)

# Finalize the Qt executable setup
qt_finalize_executable(${PROJECT_NAME})