cmake_minimum_required(VERSION 3.25)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are unsupported. You can of course patch this out of CMakeLists.txt and build in-source, but please specify a build directory using -B flag when invoking CMake.")
endif()

set(PROJECT_NAME rpgmtranslate)
set(PROJECT_VERSION 1.2.0)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin)

option(ENABLE_GXHASH "Hash with gxhash (needs AES-NI / Armv8 crypto). OFF uses the portable rapidhash." ON)
option(ENABLE_PIC
    "Enable Position Independent Code."
    ON
)

option(ENABLE_NUSPELL
    "Nuspell integration - allows using dictionaries for spell-checking."
    ON
)

option(ENABLE_LANGUAGETOOL
    "LanguageTool - allows using LanguageTool for text linting."
    ON
)

option(ENABLE_GOOGLE_TRANSLATE
    "Google Translate backend. Used for machine translation."
    ON
)

option(ENABLE_YANDEX_TRANSLATE
    "Yandex Translate backend. Used for machine translation."
    ON
)

option(ENABLE_DEEPL
    "DeepL backend. Used for machine translation."
    ON
)

option(ENABLE_SERDE_CSV
    "Serde menu: allows exporting/importing translation files as CSV."
    ON
)

option(ENABLE_SERDE_XML
    "Serde menu: allows exporting/importing translation files as XML."
    ON
)

option(ENABLE_SERDE_XLSX
    "Serde menu: allows exporting/importing translation files as XLSX."
    ON
)

option(ENABLE_SERDE_YAML
    "Serde menu: allows exporting/importing translation files as YAML."
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
    "Enable syntax highlighting for JSON data."
    ON
)
option(ENABLE_JS_HIGHLIGHTING
    "Enable syntax highlighting for JavaScript code."
    ON
)
option(ENABLE_RUBY_HIGHLIGHTING
    "Enable syntax highlighting for Ruby-lang code."
    ON
)
option(ENABLE_JS_FORMATTING
    "Enable source code formatting for JavaScript code."
    ON
)
option(ENABLE_RUBY_FORMATTING
    "Enable source code formatting for Ruby-lang code."
    ON
)

option(ENABLE_MIMALLOC "Use mimalloc instead of a standard memory allocator." OFF)

option(ENABLE_UPDATER
    "Self-updater: checks GitHub for a newer release and installs it over the running executable."
    ON
)

option(ENABLE_TOKENIZER_CHINESE_ICU
    "Glossary matching: ICU-based word segmentation for Chinese."
    OFF
)
option(ENABLE_TOKENIZER_CHINESE_LINDERA
    "Glossary matching: Lindera-based word segmentation for Chinese."
    ON
)
option(ENABLE_TOKENIZER_JAPANESE_ICU
    "Glossary matching: ICU-based word segmentation for Japanese."
    OFF
)
option(ENABLE_TOKENIZER_JAPANESE_LINDERA
    "Glossary matching: Lindera-based word segmentation for Japanese."
    ON
)
option(ENABLE_TOKENIZER_KOREAN_LINDERA
    "Glossary matching: Lindera-based word segmentation for Korean."
    ON
)
option(ENABLE_TOKENIZER_SNOWBALL
    "Glossary matching: Snowball stemming for Latin-alphabet languages."
    ON
)
option(ENABLE_TOKENIZER_SOUTHEAST_ASIAN
    "Glossary matching: ICU-based word segmentation for Thai, Khmer, Lao and Burmese."
    ON
)
option(ENABLE_COUNTING_DICTIONARY
    "Word counting: dictionary-based word segmentation for Chinese, Japanese, Thai, Lao, Khmer and Burmese. Without it a run of such text counts as one word."
    ON
)
option(ENABLE_COUNTING_LSTM
    "Word counting: LSTM models for Thai, Lao, Khmer and Burmese."
    ON
)

set(MSVC_RUNTIME
    "MT"
    CACHE
    STRING
    "MT (static) / MD (dynamic) / MTd (static debug) / MDd (dynamic debug). Don't use debug versions if you use Clang with MSVC target with enabled ASan."
)

set(_valid_runtimes MT MD MTd MDd)

if(NOT MSVC_RUNTIME IN_LIST _valid_runtimes)
    message(FATAL_ERROR
        "MSVC_RUNTIME must be one of: ${_valid_runtimes}. "
        "Got: '${MSVC_RUNTIME}'")
endif()

set(_msvc_runtime_names MT MD MTd MDd)
set(_msvc_runtime_values MultiThreaded MultiThreadedDLL MultiThreadedDebug MultiThreadedDebugDLL)
list(FIND _msvc_runtime_names "${MSVC_RUNTIME}" _msvc_runtime_index)
list(GET _msvc_runtime_values ${_msvc_runtime_index} CMAKE_MSVC_RUNTIME_LIBRARY)

set(RUST_PROFILE "" CACHE STRING "Rust profile to build the project.")

set(LUA_EXECUTABLE "" CACHE FILEPATH "Lua is used in the project for some pre-build scripts. Lua is shipped by default for Linux and Windows, but override it if you want your own Lua build.")

if (ENABLE_PIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(_qt_components Widgets Concurrent Gui Svg Network LinguistTools)

if (LINUX)
    list(APPEND _qt_components WaylandClient)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${_qt_components})
qt_standard_project_setup(REQUIRES 6.8)
find_package(glaze REQUIRED)

if (glaze_VERSION VERSION_LESS 7.5.0)
    message(FATAL_ERROR "glaze >= 7.5.0 required, found ${glaze_VERSION}")
endif()

find_package(LibArchive REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(qjs 0.14 REQUIRED)
find_program(CBINDGEN_EXECUTABLE cbindgen REQUIRED)

if (ENABLE_ASSET_PLAYBACK)
    # The version FindFFmpeg reports is libavutil's: FFmpeg 6.1 (Ubuntu 24.04's 6.1.1) is libavutil 58.29
    find_package(FFmpeg 58.29 REQUIRED)
endif()

if (ENABLE_NUSPELL)
    find_package(Nuspell 5.1 REQUIRED)
    find_package(ICU REQUIRED COMPONENTS data)
endif()

if (ENABLE_LIBGIT2)
    find_package(Libgit2 1.9 REQUIRED)
endif()

if (ENABLE_MIMALLOC)
    find_package(mimalloc 3 REQUIRED)
endif()

if (LINUX)
    find_package(OpenSSL REQUIRED)
endif()

set(SOURCES
    src/AboutWindow/AboutWindow.cpp
    src/AboutWindow/AboutWindow.ui

    src/AssetMenu/AssetMenu.cpp
    src/AssetMenu/AssetPreviewWidget.cpp
    src/AssetMenu/CodeViewer.cpp
    src/AssetMenu/GraphicsAssetViewer.cpp
    src/AssetMenu/MediaPlayer.cpp

    src/BatchMenu/BatchMenu.cpp
    src/BatchMenu/ScriptEditor.cpp
    src/BatchMenu/BatchMenu.ui

    src/BookmarkMenu/BookmarkList.cpp
    src/BookmarkMenu/BookmarkMenu.cpp

    src/CBSLIWidget/CBSLIWidget.cpp

    src/FFILogger/FFILogger.cpp

    src/utilities/Allocator.cpp

    src/FileSelectMenu/FileSelectMenu.cpp
    src/FileSelectMenu/FileSelectMenu.ui

    src/GlossaryMenu/GlossaryMenu.cpp
    src/GlossaryMenu/GlossaryMenu.ui

    src/LintMenu/LintMenu.cpp
    src/LintMenu/LintTable.cpp

    src/LintSelectMenu/LintSelectMenu.cpp

    src/MainWindow/MainWindow.cpp
    src/MainWindow/MainWindow.ui
    src/MainWindow/MainWindowCallbacks.cpp
    src/MainWindow/MainWindowConnections.cpp
    src/MainWindow/MainWindowFunctions.cpp
    src/MainWindow/MainWindowSaveLoad.cpp

    src/PersistentMenu/PersistentMenu.cpp

    src/PopupInput/PopupInput.cpp

    src/PurgeMenu/PurgeMenu.cpp
    src/PurgeMenu/PurgeMenu.ui

    src/ReadMenu/ReadMenu.cpp
    src/ReadMenu/ReadMenu.ui

    src/SearchMenu/SearchMenu.cpp
    src/SearchMenu/SearchMenu.ui

    src/SearchPanelDock/SearchPanelDock.cpp
    src/SearchPanelDock/SearchResultList.cpp

    src/SerdeMenu/SerdeMenu.cpp
    src/SerdeMenu/SerdeMenu.ui

    src/SettingsWindow/SettingsWindow.cpp
    src/SettingsWindow/SettingsWindow.ui

    src/SourceControlDock/GitCommitList.cpp
    src/SourceControlDock/GitFileList.cpp
    src/SourceControlDock/SourceControlDock.cpp

    src/ScrollableMessageBox/ScrollableMessageBox.cpp
    src/StatisticsDialog/StatisticsDialog.cpp

    src/TabPanel/TabList.cpp
    src/TabPanel/TabListDelegate.cpp
    src/TabPanel/TabListModel.cpp
    src/TabPanel/TabPanel.cpp

    src/TaskPanel/TaskPanel.cpp

    src/TaskWorker/ScriptHost.cpp
    src/TaskWorker/TaskWorker.cpp

    src/TranslationTable/LabelInput.cpp
    src/TranslationTable/LintTooltip.cpp
    src/TranslationTable/TranslationHighlighter.cpp
    src/TranslationTable/TranslationInput.cpp
    src/TranslationTable/TranslationTable.cpp
    src/TranslationTable/TranslationTableDelegate.cpp
    src/TranslationTable/TranslationTableHeader.cpp
    src/TranslationTable/TranslationTableModel.cpp

    src/TranslationsMenu/TranslationsMenu.cpp

    src/WriteMenu/WriteMenu.cpp
    src/WriteMenu/WriteMenu.ui

    src/main.cpp

    src/utilities/Notice.cpp
    src/utilities/QtStrings.cpp
    src/utilities/Utils.cpp
    src/utilities/miniaudio.cpp
    src/utilities/tinysoundfont.cpp
    src/utilities/mimalloc.cpp
)

if (ENABLE_UPDATER)
    list(APPEND SOURCES src/AutoUpdater/AutoUpdater.cpp)
endif()

# Not compiled or added to the target - headers need no source-list entry of their own, since the
# compiler finds them via target_include_directories() below. This list exists purely so
# qt_add_translations() (further down) has every file that can carry a tr()/NOTICE() call in view;
# without it, strings living only in a header (see e.g. MainWindow.hpp) would silently drop out of
# the translation catalog.
set(HEADERS
    src/3rdparty/fast_float.h
    src/3rdparty/enchantum_single_header.hpp
    src/3rdparty/miniaudio.h
    src/3rdparty/tml.h
    src/3rdparty/tsf.h
    src/3rdparty/zmij.h

    src/AboutWindow/AboutWindow.hpp

    src/AssetMenu/AssetKind.hpp
    src/AssetMenu/AssetMenu.hpp
    src/AssetMenu/AssetPreviewWidget.hpp
    src/AssetMenu/CodeViewer.hpp
    src/AssetMenu/GraphicsAssetViewer.hpp
    src/AssetMenu/MediaPlayer.hpp

    src/AutoUpdater/AutoUpdater.hpp

    src/BackupSelectDialog/BackupSelectDialog.hpp

    src/BatchMenu/BatchMenu.hpp
    src/BatchMenu/ScriptEditor.hpp

    src/BookmarkMenu/BookmarkList.hpp
    src/BookmarkMenu/BookmarkMenu.hpp

    src/CBSLIWidget/CBSLIWidget.hpp

    src/FFILogger/FFILogger.hpp

    src/FileSelectMenu/FileSelectMenu.hpp

    src/GlossaryMenu/GlossaryMenu.hpp

    src/LintMenu/LintMenu.hpp
    src/LintMenu/LintTable.hpp

    src/LintSelectMenu/LintSelectMenu.hpp

    src/Lints/CommentTags.inl
    src/Lints/Notetags.inl
    src/Lints/PluginCommands.inl
    src/Lints/PluginSequences.hpp
    src/Lints/TextCodes.inl

    src/MainWindow/MainWindow.hpp

    src/PersistentMenu/PersistentMenu.hpp

    src/PopupInput/PopupInput.hpp

    src/PurgeMenu/PurgeMenu.hpp

    src/ReadMenu/ReadMenu.hpp

    src/SearchMenu/SearchMenu.hpp

    src/SearchPanelDock/SearchPanelDock.hpp
    src/SearchPanelDock/SearchResultList.hpp

    src/SerdeMenu/SerdeMenu.hpp

    src/Settings/ProjectSettings.hpp
    src/Settings/Settings.hpp

    src/SettingsWindow/PasswordLineEdit.hpp
    src/SettingsWindow/SettingsWindow.hpp
    src/SettingsWindow/UnfocusLineEdit.hpp

    src/SourceControlDock/GitCommitList.hpp
    src/SourceControlDock/GitFileList.hpp
    src/SourceControlDock/SourceControlDock.hpp

    src/StatisticsDialog/StatisticsDialog.hpp

    src/TabPanel/TabList.hpp
    src/TabPanel/TabListDelegate.hpp
    src/TabPanel/TabListModel.hpp
    src/TabPanel/TabPanel.hpp

    src/TaskPanel/TaskPanel.hpp

    src/TaskWorker/ScriptHost.hpp
    src/TaskWorker/Task.hpp
    src/TaskWorker/TaskWorker.hpp

    src/TranslationTable/LabelInput.hpp
    src/TranslationTable/LintTooltip.hpp
    src/TranslationTable/TranslationHighlighter.hpp
    src/TranslationTable/TranslationInput.hpp
    src/TranslationTable/TranslationTable.hpp
    src/TranslationTable/TranslationTableDelegate.hpp
    src/TranslationTable/TranslationTableHeader.hpp
    src/TranslationTable/TranslationTableModel.hpp

    src/TranslationsMenu/ClickableLabel.hpp
    src/TranslationsMenu/TranslationsMenu.hpp

    src/WriteMenu/WriteMenu.hpp

    src/utilities/Aliases.hpp
    src/utilities/Constants.hpp
    src/utilities/Enums.hpp
    src/utilities/FWD.hpp
    src/utilities/Hasher.hpp
    src/utilities/Notice.hpp
    src/utilities/Numbers.hpp
    src/utilities/QtAliases.hpp
    src/utilities/QtNumbers.hpp
    src/utilities/QtStrings.hpp
    src/utilities/Types.hpp
    src/utilities/Utils.hpp
    src/utilities/ezgit2.hpp
    src/utilities/glazemeta.hpp
    src/utilities/PackedBits.hpp
)

qt_add_executable(${PROJECT_NAME})

# The header-only QObject classes have no .cpp to be found by AUTOMOC through
target_sources(${PROJECT_NAME} PRIVATE
    ${SOURCES}
    src/TranslationsMenu/ClickableLabel.hpp
    src/SettingsWindow/UnfocusLineEdit.hpp
    src/BackupSelectDialog/BackupSelectDialog.hpp
    src/SettingsWindow/PasswordLineEdit.hpp
)

if (WIN32)
    set(MANIFEST_IDENTITY "com.rpg-maker-translation-tools.${PROJECT_NAME}")
    set(MANIFEST_DESCRIPTION "RPGMTranslate")
    configure_file(
        rpgmtranslate.manifest.in
        "${CMAKE_CURRENT_BINARY_DIR}/rpgmtranslate.manifest"
        @ONLY
    )

    target_sources(${PROJECT_NAME} PRIVATE
        resources/${PROJECT_NAME}-logo.rc
        "${CMAKE_CURRENT_BINARY_DIR}/rpgmtranslate.manifest"
    )
endif()

set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow"
)

file(GLOB TRANSLATION_TS_FILES CONFIGURE_DEPENDS translations/*.ts)

qt_add_translations(
    ${PROJECT_NAME}
    TS_FILES ${TRANSLATION_TS_FILES}
    SOURCES ${SOURCES} ${HEADERS}
    QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
    IMMEDIATE_CALL
    # NOTICE()/NOTICE_N()/NOTICE_IN() (src/utilities/Notice.hpp) wrap the message literal, and lupdate does not expand
    # macros. These aliases teach it to read the literal out of the call directly. NOTICE takes the text first so it
    # lines up with QT_TR_NOOP's single argument; NOTICE_IN maps to QT_TRANSLATE_NOOP, which only fires on a
    # two-argument call. NOTICE_N's call-site argument order (text, nullptr, n, ...) instead matches tr()'s own
    # (sourceText, disambiguation, n) shape, aliased here to `tr` rather than a NOOP macro - only that gets lupdate
    # to generate separate singular/plural entries for it, since numerus is recognized positionally, not via NOOP.
    LUPDATE_OPTIONS -tr-function-alias QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N
)

# The generated catalogs are embedded from wherever the build directory is
foreach(qmFile IN LISTS TRANSLATION_QM_FILES)
    get_filename_component(qmName "${qmFile}" NAME)
    string(REGEX REPLACE "^${PROJECT_NAME}\\.([A-Za-z]+)(-[A-Za-z]+)?\\.qm$" "\\1.qm" qmAlias "${qmName}")
    set_source_files_properties("${qmFile}" PROPERTIES QT_RESOURCE_ALIAS "${qmAlias}")
endforeach()

qt_add_resources(${PROJECT_NAME} translations
    PREFIX "/"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${TRANSLATION_QM_FILES}
)

# Relative to the source directory, so each is embedded as `:/icons/<name>.svg`
file(GLOB ICON_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS icons/*.svg)

qt_add_resources(${PROJECT_NAME} icons
    PREFIX "/"
    FILES ${ICON_FILES} icons/${PROJECT_NAME}-logo.png
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_BUILD>
        $<$<CONFIG:Release>:RELEASE_BUILD>

        $<$<BOOL:${ENABLE_NUSPELL}>:ENABLE_NUSPELL>
        $<$<BOOL:${ENABLE_LANGUAGETOOL}>:ENABLE_LANGUAGETOOL>
        $<$<BOOL:${ENABLE_GOOGLE_TRANSLATE}>:ENABLE_GOOGLE_TRANSLATE>
        $<$<BOOL:${ENABLE_YANDEX_TRANSLATE}>:ENABLE_YANDEX_TRANSLATE>
        $<$<BOOL:${ENABLE_DEEPL}>:ENABLE_DEEPL>
        $<$<BOOL:${ENABLE_SERDE_CSV}>:ENABLE_SERDE_CSV>
        $<$<BOOL:${ENABLE_SERDE_XML}>:ENABLE_SERDE_XML>
        $<$<BOOL:${ENABLE_SERDE_XLSX}>:ENABLE_SERDE_XLSX>
        $<$<BOOL:${ENABLE_SERDE_YAML}>:ENABLE_SERDE_YAML>
        $<$<BOOL:${ENABLE_LIBGIT2}>:ENABLE_LIBGIT2>
        $<$<BOOL:${ENABLE_ASSET_PLAYBACK}>:ENABLE_ASSET_PLAYBACK>
        $<$<BOOL:${ENABLE_JSON_HIGHLIGHTING}>:ENABLE_JSON_HIGHLIGHTING>
        $<$<BOOL:${ENABLE_JS_HIGHLIGHTING}>:ENABLE_JS_HIGHLIGHTING>
        $<$<BOOL:${ENABLE_RUBY_HIGHLIGHTING}>:ENABLE_RUBY_HIGHLIGHTING>
        $<$<BOOL:${ENABLE_JS_FORMATTING}>:ENABLE_JS_FORMATTING>
        $<$<BOOL:${ENABLE_RUBY_FORMATTING}>:ENABLE_RUBY_FORMATTING>
        $<$<BOOL:${ENABLE_MIMALLOC}>:ENABLE_MIMALLOC>
        $<$<BOOL:${ENABLE_UPDATER}>:ENABLE_UPDATER>

        QT_DISABLE_DEPRECATED_UP_TO=0x060800
        QT_ENABLE_STRICT_MODE_UP_TO=0x060800
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_QASCONST
        QT_NO_QEXCHANGE
        QT_NO_QSNPRINTF
        QT_NO_URL_CAST_FROM_STRING
        QT_NO_FOREACH
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
        QT_USE_NODISCARD_FILE_OPEN

        # Include file/line information in debug messages
        QT_MESSAGELOGCONTEXT
)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

set(RUST_CRATE_DIR "${CMAKE_SOURCE_DIR}/rust")
set(GENERATED_DIR  "${CMAKE_CURRENT_BINARY_DIR}")
set(RUST_HEADER "${GENERATED_DIR}/${PROJECT_NAME}_rs.h")

include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6.1
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Corrosion)

set(RUST_FEATURES "")

if (ENABLE_LANGUAGETOOL)
    list(APPEND RUST_FEATURES "languagetool")
endif()

if (ENABLE_GOOGLE_TRANSLATE)
    list(APPEND RUST_FEATURES "google-translate")
endif()

if (ENABLE_YANDEX_TRANSLATE)
    list(APPEND RUST_FEATURES "yandex-translate")
endif()

if (ENABLE_DEEPL)
    list(APPEND RUST_FEATURES "deepl")
endif()

if (ENABLE_SERDE_CSV)
    list(APPEND RUST_FEATURES "serde-csv")
endif()

if (ENABLE_SERDE_XML)
    list(APPEND RUST_FEATURES "serde-xml")
endif()

if (ENABLE_SERDE_XLSX)
    list(APPEND RUST_FEATURES "serde-xlsx")
endif()

if (ENABLE_SERDE_YAML)
    list(APPEND RUST_FEATURES "serde-yaml")
endif()

if (ENABLE_JSON_HIGHLIGHTING)
    list(APPEND RUST_FEATURES "json-highlighting")
endif()

if (ENABLE_JS_HIGHLIGHTING)
    list(APPEND RUST_FEATURES "js-highlighting")
endif()

if (ENABLE_RUBY_HIGHLIGHTING)
    list(APPEND RUST_FEATURES "ruby-highlighting")
endif()

if (ENABLE_JS_FORMATTING)
    list(APPEND RUST_FEATURES "js-formatting")
endif()

if (ENABLE_RUBY_FORMATTING)
    list(APPEND RUST_FEATURES "ruby-formatting")
endif()

if (ENABLE_TOKENIZER_CHINESE_ICU)
    list(APPEND RUST_FEATURES "chinese-icu")
endif()

if (ENABLE_TOKENIZER_CHINESE_LINDERA)
    list(APPEND RUST_FEATURES "chinese-lindera")
endif()

if (ENABLE_TOKENIZER_JAPANESE_ICU)
    list(APPEND RUST_FEATURES "japanese-icu")
endif()

if (ENABLE_TOKENIZER_JAPANESE_LINDERA)
    list(APPEND RUST_FEATURES "japanese-lindera")
endif()

if (ENABLE_TOKENIZER_KOREAN_LINDERA)
    list(APPEND RUST_FEATURES "korean-lindera")
endif()

if (ENABLE_TOKENIZER_SNOWBALL)
    list(APPEND RUST_FEATURES "snowball")
endif()

if (ENABLE_TOKENIZER_SOUTHEAST_ASIAN)
    list(APPEND RUST_FEATURES "southeast-asian")
endif()

if (ENABLE_COUNTING_DICTIONARY)
    list(APPEND RUST_FEATURES "count-dictionary")
endif()

if (ENABLE_COUNTING_LSTM)
    list(APPEND RUST_FEATURES "count-lstm")
endif()

if (NOT ENABLE_GXHASH)
    list(APPEND RUST_FEATURES rapidhash)
    set(RUST_NO_DEFAULT NO_DEFAULT_FEATURES)
endif()

corrosion_import_crate(
    MANIFEST_PATH rust/Cargo.toml
    ${RUST_NO_DEFAULT}
    FEATURES ${RUST_FEATURES}
    PROFILE ${RUST_PROFILE}
)

file(GLOB_RECURSE RUST_SOURCES
    CONFIGURE_DEPENDS
    rust/src/*.rs
    rust/Cargo.toml
    rust/Cargo.lock
    rust/cbindgen.toml
)

add_custom_command(
    OUTPUT "${RUST_HEADER}"
    COMMAND "${CBINDGEN_EXECUTABLE}"
            --config "${RUST_CRATE_DIR}/cbindgen.toml"
            --output "${RUST_HEADER}"
    DEPENDS ${RUST_SOURCES}
    WORKING_DIRECTORY "${RUST_CRATE_DIR}"
    VERBATIM
)

add_custom_target(rust_header
    DEPENDS "${RUST_HEADER}"
)

add_dependencies(${PROJECT_NAME} rust_header)

if (NOT LUA_EXECUTABLE)
    if (WIN32)
        set(LUA_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/lua.exe")
    elseif (LINUX)
        set(LUA_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/lua")
    else()
        message(FATAL_ERROR "Lua executable is not provided for your platform. It is required for a configure step. Supply the path to your system's executable using -DLUA_EXECUTABLE=...")
    endif()
endif()

file(GLOB BUILTIN_SCRIPTS CONFIGURE_DEPENDS scripts/*.js)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/headers/scripts.h"
    COMMAND "${LUA_EXECUTABLE}" prepare-headers.lua "${CMAKE_CURRENT_BINARY_DIR}/headers"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${LUA_EXECUTABLE}" prepare-headers.lua ${BUILTIN_SCRIPTS}
    COMMENT "Generating scripts.h"
)

add_custom_target(scripts_header
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/headers/scripts.h"
)

add_dependencies(${PROJECT_NAME} scripts_header)

configure_file(
    src/config/config.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/headers/config.h"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/headers"
)

file(GLOB SUBDIRS LIST_DIRECTORIES true src/*)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SUBDIRS}
    "${FFMPEG_INCLUDE_DIR}"
    "${GENERATED_DIR}"
)

add_library(zmij OBJECT src/3rdparty/zmij.cc src/3rdparty/zmij-int.cc)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Svg
    LibArchive::LibArchive
    LibLZMA::LibLZMA
    qjs
    zmij
    ${PROJECT_NAME}_rs
)

if (ENABLE_ASSET_PLAYBACK)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::avfilter
        FFmpeg::swresample
        FFmpeg::swscale
    )
endif()

if (ENABLE_NUSPELL)
    target_link_libraries(${PROJECT_NAME} PRIVATE Nuspell::nuspell ICU::data)
endif()

if (ENABLE_LIBGIT2)
    target_link_libraries(${PROJECT_NAME} PRIVATE libgit2::libgit2)
endif()

if (ENABLE_MIMALLOC)
    if (TARGET mimalloc)
        target_link_libraries(${PROJECT_NAME} PRIVATE mimalloc)
    elseif (TARGET mimalloc-static)
        target_link_libraries(${PROJECT_NAME} PRIVATE mimalloc-static)
    else()
        message(FATAL_ERROR "mimalloc package found, but it exports neither `mimalloc` nor `mimalloc-static` target")
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE "${MIMALLOC_INCLUDE_DIR}")
endif()

if (LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WaylandClient)
    corrosion_link_libraries(${PROJECT_NAME}_rs OpenSSL::Crypto OpenSSL::SSL)
endif()

function(summary_row label value)
    string(LENGTH "${label}" _length)
    math(EXPR _padding "40 - ${_length}")
    if (_padding LESS 1)
        set(_padding 1)
    endif()
    string(REPEAT " " ${_padding} _spaces)
    message(STATUS "${label}${_spaces}${value}")
endfunction()

function(summary_options)
    foreach (_option IN LISTS ARGN)
        if (${_option})
            summary_row("${_option}" "ON")
        else()
            summary_row("${_option}" "OFF")
        endif()
    endforeach()
endfunction()

message(STATUS "================= ${PROJECT_NAME} ${PROJECT_VERSION} BUILD SUMMARY =================")
get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (_multi_config)
    summary_row("Build type" "multi-config (${CMAKE_CONFIGURATION_TYPES})")
else()
    summary_row("Build type" "${CMAKE_BUILD_TYPE}")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        summary_row("Debug mode" "ON (DEBUG_BUILD defined)")
    else()
        summary_row("Debug mode" "OFF")
    endif()
endif()
summary_row("Compiler" "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
summary_row("Install prefix" "${CMAKE_INSTALL_PREFIX}")
if (WIN32)
    summary_row("MSVC runtime" "${MSVC_RUNTIME} (${CMAKE_MSVC_RUNTIME_LIBRARY})")
endif()
if (RUST_PROFILE)
    summary_row("Rust profile" "${RUST_PROFILE}")
else()
    summary_row("Rust profile" "default")
endif()
summary_row("Lua" "${LUA_EXECUTABLE}")
summary_row("Qt" "${Qt6_VERSION}")
if (ENABLE_ASSET_PLAYBACK)
    summary_row("FFmpeg" "${FFmpeg_VERSION} (libavutil)")
endif()
summary_options(
    ENABLE_PIC
    ENABLE_NUSPELL
    ENABLE_LANGUAGETOOL
    ENABLE_GOOGLE_TRANSLATE
    ENABLE_YANDEX_TRANSLATE
    ENABLE_DEEPL
    ENABLE_SERDE_CSV
    ENABLE_SERDE_XML
    ENABLE_SERDE_XLSX
    ENABLE_SERDE_YAML
    ENABLE_LIBGIT2
    ENABLE_ASSET_PLAYBACK
    ENABLE_JSON_HIGHLIGHTING
    ENABLE_JS_HIGHLIGHTING
    ENABLE_RUBY_HIGHLIGHTING
    ENABLE_JS_FORMATTING
    ENABLE_RUBY_FORMATTING
    ENABLE_MIMALLOC
    ENABLE_UPDATER
    ENABLE_TOKENIZER_CHINESE_ICU
    ENABLE_TOKENIZER_CHINESE_LINDERA
    ENABLE_TOKENIZER_JAPANESE_ICU
    ENABLE_TOKENIZER_JAPANESE_LINDERA
    ENABLE_TOKENIZER_KOREAN_LINDERA
    ENABLE_TOKENIZER_SNOWBALL
    ENABLE_TOKENIZER_SOUTHEAST_ASIAN
    ENABLE_COUNTING_DICTIONARY
    ENABLE_COUNTING_LSTM
)
message(STATUS "=======================================================")
