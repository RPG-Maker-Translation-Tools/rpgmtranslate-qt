# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)

# Project definition
set(PROJECT_NAME rpgmtranslate)
set(PROJECT_VERSION "1.0.0")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

option(ENABLE_NUSPELL       "Enable Nuspell integration"       OFF)
option(ENABLE_LANGUAGETOOL  "Enable LanguageTool integration"  OFF)
option(ENABLE_LIBGIT2       "Enable libgit2 integration"       OFF)

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

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Gui Svg Network LinguistTools)

if (ENABLE_NUSPELL)
    find_package(Nuspell REQUIRED)
endif()

if (ENABLE_LIBGIT2)
    find_package(libgit2 CONFIG QUIET)

    if (NOT libgit2_FOUND)
        find_package(PkgConfig QUIET)

        if (PKG_CONFIG_FOUND)
            pkg_check_modules(LIBGIT2 REQUIRED libgit2)
        else()
            message(FATAL_ERROR "ENABLE_LIBGIT2=ON but neither libgit2 Config package nor pkg-config is available.")
        endif()
    endif()
endif()

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

        $<$<BOOL:${ENABLE_NUSPELL}>:ENABLE_NUSPELL>
        $<$<BOOL:${ENABLE_LANGUAGETOOL}>:ENABLE_LANGUAGETOOL>
        $<$<BOOL:${ENABLE_LIBGIT2}>:ENABLE_LIBGIT2>
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
    ${RAPIDHASH_INCLUDE_DIRS}
    ${MINIAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Svg
)

if(ENABLE_NUSPELL)
    if(TARGET Nuspell::nuspell)
        target_link_libraries(${PROJECT_NAME} PRIVATE Nuspell::nuspell)
    else()
        message(FATAL_ERROR "Nuspell found, but Nuspell::nuspell target not provided by your package config.")
    endif()
endif()

if (ENABLE_LIBGIT2)
    if (TARGET libgit2::libgit2)
        target_link_libraries(${PROJECT_NAME} PRIVATE libgit2::libgit2)
    elseif (DEFINED LIBGIT2_LIBRARIES)
        target_include_directories(${PROJECT_NAME} PRIVATE ${LIBGIT2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBGIT2_LIBRARIES})
    else()
        message(FATAL_ERROR "ENABLE_LIBGIT2=ON but libgit2 could not be linked (no target and no pkg-config variables).")
    endif()
endif()

if (WIN32)
    find_library(BIT7Z_LIBRARY bit7z)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${BIT7Z_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
        bit7z
    )
endif()

if (LINUX)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::WaylandClient
        -lssl
        -lcrypto
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
if(ENABLE_LANGUAGETOOL)
    list(APPEND RPGMTRANSLATE_RUST_FEATURES "languagetool")
endif()

corrosion_import_crate(
    MANIFEST_PATH "${RUST_CRATE_DIR}/Cargo.toml"
    FEATURES ${RPGMTRANSLATE_RUST_FEATURES}
)

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