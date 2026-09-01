cmake_minimum_required(VERSION 3.25)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are unsupported. You can of course patch this out of CMakeLists.txt and build in-source, but please specify a build directory using -B flag when invoking CMake.")
endif()

set(PROJECT_NAME rpgmtranslate)
set(PROJECT_VERSION "1.1.0")
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY target/bin CACHE PATH "" FORCE)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

option(ENABLE_LLM_CONNECTOR
    "LLM-based translation backends (OpenAI, Anthropic, DeepSeek, Gemini, Mistral, Ollama, and other OpenAI-compatible/local providers)."
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

set(RUST_PROFILE "" CACHE STRING "Rust profile to build the project.")

set(LUA_EXECUTABLE "" CACHE FILEPATH "Lua is used in the project for some pre-build scripts. Lua is shipped by default for Linux and Windows, but override it if you want your own Lua build.")

if (ENABLE_PIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Gui Svg Network LinguistTools)
find_package(LibArchive REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(qjs REQUIRED)

if (ENABLE_ASSET_PLAYBACK)
    find_package(FFmpeg REQUIRED)
endif()

if (ENABLE_NUSPELL)
    find_package(Nuspell REQUIRED)
    find_package(ICU REQUIRED COMPONENTS data)
endif()

if (ENABLE_LIBGIT2)
    find_package(Libgit2 REQUIRED)
endif()

if (ENABLE_MIMALLOC)
    find_package(mimalloc REQUIRED)
endif()

if (LINUX)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
    find_package(OpenSSL REQUIRED)
endif()

set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AboutWindow/AboutWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AboutWindow/AboutWindow.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/AssetMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/AssetPreviewWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/CodeViewer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/GraphicsAssetViewer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/MediaPlayer.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/AutoUpdater/AutoUpdater.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/BatchMenu/BatchMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BatchMenu/BatchMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/BookmarkMenu/BookmarkList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BookmarkMenu/BookmarkMenu.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/CBSLIWidget/CBSLIWidget.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/FFILogger/FFILogger.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Allocator.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/FileSelectMenu/FileSelectMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FileSelectMenu/FileSelectMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/GlossaryMenu/GlossaryMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GlossaryMenu/GlossaryMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintMenu/LintMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintMenu/LintTable.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintSelectMenu/LintSelectMenu.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindow.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindowCallbacks.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindowConnections.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindowFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindowSaveLoad.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PersistentMenu/PersistentMenu.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PopupInput/PopupInput.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PurgeMenu/PurgeMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PurgeMenu/PurgeMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadMenu/ReadMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadMenu/ReadMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchMenu/SearchMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchMenu/SearchMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchPanelDock/SearchPanelDock.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchPanelDock/SearchResultList.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SerdeMenu/SerdeMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SerdeMenu/SerdeMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/SettingsWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/SettingsWindow.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/GitCommitList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/GitFileList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/SourceControlDock.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabListDelegate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabListModel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabPanel.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TaskPanel/TaskPanel.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TaskWorker/TaskWorker.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/LabelInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/LintTooltip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationHighlighter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableDelegate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableHeader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableModel.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationsMenu/TranslationsMenu.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/WriteMenu/WriteMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/WriteMenu/WriteMenu.ui"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Notice.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/miniaudio.cpp"
)

# Not compiled or added to the target - headers need no source-list entry of their own, since the
# compiler finds them via target_include_directories() below. This list exists purely so
# qt_add_translations() (further down) has every file that can carry a tr()/NOTICE() call in view;
# without it, strings living only in a header (see e.g. MainWindow.hpp) would silently drop out of
# the translation catalog.
set(HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/fast_float.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/jeaiii_to_text.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/magic_enum.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/miniaudio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3rdparty/zmij.h"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/AboutWindow/AboutWindow.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/AssetMenu.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/AssetPreviewWidget.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/CodeViewer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/GraphicsAssetViewer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AssetMenu/MediaPlayer.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/AutoUpdater/AutoUpdater.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/BackupSelectDialog/BackupSelectDialog.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/BatchMenu/BatchMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/BookmarkMenu/BookmarkList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BookmarkMenu/BookmarkMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/CBSLIWidget/CBSLIWidget.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/FFILogger/FFILogger.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/FileSelectMenu/FileSelectMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/GlossaryMenu/GlossaryMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintMenu/LintMenu.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintMenu/LintTable.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/LintSelectMenu/LintSelectMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/Lints/PluginSequences.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow/MainWindow.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PersistentMenu/PersistentMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PopupInput/PopupInput.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/PurgeMenu/PurgeMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadMenu/ReadMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchMenu/SearchMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchPanelDock/SearchPanelDock.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SearchPanelDock/SearchResultList.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SerdeMenu/SerdeMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/Settings/ProjectSettings.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Settings/Settings.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/PasswordLineEdit.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/SettingsWindow.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/UnfocusLineEdit.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/GitCommitList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/GitFileList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SourceControlDock/SourceControlDock.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabList.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabListDelegate.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabListModel.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TabPanel/TabPanel.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TaskPanel/TaskPanel.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TaskWorker/Task.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TaskWorker/TaskWorker.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/LabelInput.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/LintTooltip.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationHighlighter.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationInput.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTable.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableDelegate.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableHeader.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationTable/TranslationTableModel.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationsMenu/ClickableLabel.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationsMenu/TranslationsMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/WriteMenu/WriteMenu.hpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Aliases.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Constants.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Enums.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/FWD.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Hasher.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Notice.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Types.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/Utils.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/ezgit2.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utilities/glazemeta.hpp"
)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
)

set(WINDOWS_APPLICATION_ICON "resources/${PROJECT_NAME}-logo.rc")

target_sources(${PROJECT_NAME} PRIVATE
    ${SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/TranslationsMenu/ClickableLabel.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/UnfocusLineEdit.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/BackupSelectDialog/BackupSelectDialog.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SettingsWindow/PasswordLineEdit.hpp"
    ${WINDOWS_APPLICATION_ICON}
    rpgmtranslate.manifest
)

set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow"
)

file(GLOB TRANSLATION_TS_FILES translations/*)

qt_add_translations(
    ${PROJECT_NAME}
    TS_FILES ${TRANSLATION_TS_FILES}
    SOURCES ${SOURCES} ${HEADERS}
    QM_FILES_OUTPUT_VARIABLE TRANSLATION_QM_FILES
    # NOTICE()/NOTICE_N()/NOTICE_IN() (src/utilities/Notice.hpp) wrap the message literal, and lupdate does not expand
    # macros. These aliases teach it to read the literal out of the call directly. NOTICE takes the text first so it
    # lines up with QT_TR_NOOP's single argument; NOTICE_IN maps to QT_TRANSLATE_NOOP, which only fires on a
    # two-argument call. NOTICE_N's call-site argument order (text, nullptr, n, ...) instead matches tr()'s own
    # (sourceText, disambiguation, n) shape, aliased here to `tr` rather than a NOOP macro - only that gets lupdate
    # to generate separate singular/plural entries for it, since numerus is recognized positionally, not via NOOP.
    LUPDATE_OPTIONS -tr-function-alias QT_TR_NOOP+=NOTICE,QT_TRANSLATE_NOOP+=NOTICE_IN,tr+=NOTICE_N
)

qt_add_resources(resources.qrc)
target_sources(${PROJECT_NAME} PRIVATE resources.qrc)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG_BUILD>
        $<$<CONFIG:Release>:RELEASE_BUILD>

        $<$<BOOL:${ENABLE_NUSPELL}>:ENABLE_NUSPELL>
        $<$<BOOL:${ENABLE_LANGUAGETOOL}>:ENABLE_LANGUAGETOOL>
        $<$<BOOL:${ENABLE_GOOGLE_TRANSLATE}>:ENABLE_GOOGLE_TRANSLATE>
        $<$<BOOL:${ENABLE_YANDEX_TRANSLATE}>:ENABLE_YANDEX_TRANSLATE>
        $<$<BOOL:${ENABLE_DEEPL}>:ENABLE_DEEPL>
        $<$<BOOL:${ENABLE_LLM_CONNECTOR}>:ENABLE_LLM_CONNECTOR>
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
)

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

if (ENABLE_LLM_CONNECTOR)
    list(APPEND RUST_FEATURES "llm-connector")
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

corrosion_import_crate(
    MANIFEST_PATH "${RUST_CRATE_DIR}/Cargo.toml"
    FEATURES ${RUST_FEATURES}
    PROFILE ${RUST_PROFILE}
)

file(GLOB_RECURSE RUST_SOURCES
    CONFIGURE_DEPENDS
    "${RUST_CRATE_DIR}/src/*.rs"
    "${RUST_CRATE_DIR}/Cargo.toml"
    "${RUST_CRATE_DIR}/Cargo.lock"
    "${RUST_CRATE_DIR}/cbindgen.toml"
)

add_custom_command(
    OUTPUT "${RUST_HEADER}"
    COMMAND cbindgen
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

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/headers/scripts.h"
    COMMAND "${LUA_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-headers.lua"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${LUA_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-headers.lua"
    COMMENT "Generating scripts.h"
)

add_custom_target(scripts_header
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/headers/scripts.h"
)

add_dependencies(${PROJECT_NAME} scripts_header)

configure_file(
    src/version/version.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/headers/version.h"
)

configure_file(
    src/version/nuspell-version.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/headers/nuspell-version.h"
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

add_library(zmij OBJECT src/3rdparty/zmij.cc)

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
    target_link_libraries(${PROJECT_NAME} PRIVATE mimalloc)
endif()

if (LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WaylandClient)
    corrosion_link_libraries(${PROJECT_NAME}_rs OpenSSL::Crypto OpenSSL::SSL)
endif()

qt_finalize_executable(${PROJECT_NAME})
