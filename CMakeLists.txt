# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)

# Project definition
set(PROJECT_NAME rpgmtranslate)
set(PROJECT_VERSION "1.0.0")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address,undefined,leak -fsanitize-address-use-after-scope")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address,undefined,leak -fsanitize-address-use-after-scope")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined,leak -fsanitize-address-use-after-scope")

# Set standards for C and C++
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory for the compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)

# Enable Qt6 automatic processing (UIC, MOC, RCC)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Configure MSVC runtime linkage (static runtime, Debug/Release aware)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (LINUX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Gui Network LinguistTools)
find_package(Nuspell)

find_path(RAPIDHASH_INCLUDE_DIRS "rapidhash.h")
if(NOT RAPIDHASH_INCLUDE_DIRS)
    message(FATAL_ERROR "rapidhash.h not found, specify path to it using `-DRAPIDHASH_INCLUDE_DIRS=...`")
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
)

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
    ${RAPIDHASH_INCLUDE_DIRS}
    ${MINIAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Nuspell::nuspell
)

if (WIN32)
    find_library(BIT7Z_LIBRARY bit7z)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${BIT7Z_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        bit7z
    )
endif()

# Libraries to link on Linux
if (LINUX)
    find_package(Qt6 REQUIRED COMPONENTS Svg WaylandClient)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Svg
        Qt6::WaylandClient
        -lssl
        -lcrypto
    )
endif()

# Install the built executable
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
)

# Rust shit
# TODO: Remove cringe comments
# ---- Rust build integration (Ninja Multi-Config safe) ----

set(RUST_CRATE_DIR     "${CMAKE_SOURCE_DIR}/rust")
set(CARGO_TARGET_DIR   "${CMAKE_BINARY_DIR}/cargo-target")
set(GENERATED_DIR      "${CMAKE_CURRENT_BINARY_DIR}") # where rpgmtranslate.h will be generated
set(RPGMTRANSLATE_RUST_HEADER "${GENERATED_DIR}/rpgmtranslate.h")

# Rust staticlib name differs by toolchain on Windows
if(MSVC)
  set(RUST_LIB_NAME "rpgmtranslate.lib")
else()
  set(RUST_LIB_NAME "librpgmtranslate.a")
endif()

# Cargo puts outputs in:
#   ${CARGO_TARGET_DIR}/debug/...
#   ${CARGO_TARGET_DIR}/release/...
set(RUST_LIB_PATH_DEBUG   "${CARGO_TARGET_DIR}/debug/${RUST_LIB_NAME}")
set(RUST_LIB_PATH_RELEASE "${CARGO_TARGET_DIR}/release/${RUST_LIB_NAME}")

# Track Rust inputs so CMake knows when it must re-run cbindgen / cargo build target
file(GLOB_RECURSE RUST_SOURCES
  CONFIGURE_DEPENDS
  "${RUST_CRATE_DIR}/src/*.rs"
  "${RUST_CRATE_DIR}/Cargo.toml"
  "${RUST_CRATE_DIR}/Cargo.lock"
  "${RUST_CRATE_DIR}/cbindgen.toml"
)

# --- Header generation (config-independent => safe as an OUTPUT) ---
add_custom_command(
  OUTPUT  "${RPGMTRANSLATE_RUST_HEADER}"
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

# --- Rust build (DO NOT use OUTPUT with $<CONFIG> for Ninja Multi-Config) ---
# Build Debug => cargo build
# Build Release/RelWithDebInfo/MinSizeRel => cargo build --release
add_custom_target(rpgmtranslate_rust_build
  COMMAND "${CMAKE_COMMAND}" -E env
          "CARGO_TARGET_DIR=${CARGO_TARGET_DIR}"
          cargo build $<$<NOT:$<CONFIG:Debug>>:--release>
  DEPENDS ${RUST_SOURCES}
  WORKING_DIRECTORY "${RUST_CRATE_DIR}"
  VERBATIM
  COMMAND_EXPAND_LISTS
)

# --- Imported library target with per-config locations ---
add_library(rust_rpgmtranslate STATIC IMPORTED GLOBAL)

# Tell CMake exactly where the library will be for each config.
# RelWithDebInfo and MinSizeRel are mapped to Cargo's "release" directory.
set_target_properties(rust_rpgmtranslate PROPERTIES
  IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
  IMPORTED_LOCATION_DEBUG        "${RUST_LIB_PATH_DEBUG}"
  IMPORTED_LOCATION_RELEASE      "${RUST_LIB_PATH_RELEASE}"
  IMPORTED_LOCATION_RELWITHDEBINFO "${RUST_LIB_PATH_RELEASE}"
  IMPORTED_LOCATION_MINSIZEREL   "${RUST_LIB_PATH_RELEASE}"
)

# Make sure the Rust build runs before anything links against the imported lib.
add_dependencies(rust_rpgmtranslate
  rpgmtranslate_rust_build
  rpgmtranslate_rust_header
)

# Expose the generated header directory to consumers
target_include_directories(rust_rpgmtranslate INTERFACE "${GENERATED_DIR}")

# Link into your main target and ensure build order
target_link_libraries(${PROJECT_NAME} PRIVATE rust_rpgmtranslate)
target_include_directories(${PROJECT_NAME} PRIVATE "${GENERATED_DIR}")
add_dependencies(${PROJECT_NAME}
  rpgmtranslate_rust_build
  rpgmtranslate_rust_header
)


# Finalize the Qt executable setup
qt_finalize_executable(${PROJECT_NAME})

