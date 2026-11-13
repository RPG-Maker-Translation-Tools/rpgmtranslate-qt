# Tests are opt-in (-DENABLE_TESTS=ON) and run with `ctest`. They borrow the include directories, definitions and
# libraries of the application target instead of repeating them, so this directory has to be added after that target is
# fully set up.

set(APP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

# `cmake --build <dir> --target coverage` runs the tests and reports how much of the application they execute
option(ENABLE_COVERAGE "Instrument the tests with Clang's source-based coverage and add a `coverage` target" OFF)

set(COVERAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/coverage")

if (ENABLE_COVERAGE AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "ENABLE_COVERAGE needs Clang: the report is made by llvm-cov")
endif()

# Builds the Qt Test executable `name` from `ARGN`, which lists the test's own sources first and then the application
# sources it needs. There is no application library yet, so a test compiles just the files it exercises.
function(add_app_test name)
    # The Rust half allocates through the application's allocator, so every test that links it needs that file
    qt_add_executable(${name} ${ARGN} ${APP_SOURCE_DIR}/utilities/Allocator.cpp)

    target_include_directories(${name} PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${name} PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>
    )
    target_compile_options(${name} PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_OPTIONS>
    )
    target_link_libraries(${name} PRIVATE
        Qt6::Test
        $<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>
    )

    # The application's compile definitions make QTEST_MAIN define only qMain on Windows (QT_NEEDS_QMAIN); the entry
    # point that calls it is linked into WIN32 executables
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE TRUE)

    # The generated headers the sources include
    add_dependencies(${name} rust_header scripts_header)

    # The mock keeps the API keys that the tests save out of the credential store of the user
    set(test_environment "QT_QPA_PLATFORM=offscreen" "RPGMTRANSLATE_MOCK_KEYRING=1")

    if (ENABLE_COVERAGE)
        target_compile_options(${name} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${name} PRIVATE -fprofile-instr-generate)
        # %p keeps the processes of one test from overwriting each other's profile
        list(APPEND test_environment "LLVM_PROFILE_FILE=${COVERAGE_DIR}/${name}-%p.profraw")
        set_property(GLOBAL APPEND PROPERTY COVERAGE_TESTS ${name})
    endif()

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${test_environment}")
endfunction()

add_app_test(tst_task
    tst_task.cpp
)

add_app_test(tst_lints
    tst_lints.cpp
)

add_app_test(tst_strings
    tst_strings.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_packedbits
    tst_packedbits.cpp
)

add_app_test(tst_types
    tst_types.cpp
)

add_app_test(tst_projectsettings
    tst_projectsettings.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_notice
    tst_notice.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_scripthost
    tst_scripthost.cpp
    ${APP_SOURCE_DIR}/TaskWorker/ScriptHost.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_models
    tst_models.cpp
    ${APP_SOURCE_DIR}/TabPanel/TabListModel.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTableModel.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_utilsio
    tst_utilsio.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_taskworker_project
    tst_taskworker_project.cpp
    ${APP_SOURCE_DIR}/TaskWorker/TaskWorker.cpp
    ${APP_SOURCE_DIR}/TaskWorker/ScriptHost.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_utils
    tst_utils.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_utilsimages
    tst_utilsimages.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_taskpanel
    tst_taskpanel.cpp
    ${APP_SOURCE_DIR}/TaskPanel/TaskPanel.cpp
    ${APP_SOURCE_DIR}/TaskWorker/TaskWorker.cpp
    ${APP_SOURCE_DIR}/TaskWorker/ScriptHost.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_bookmarklist
    tst_bookmarklist.cpp
    ${APP_SOURCE_DIR}/BookmarkMenu/BookmarkList.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_bookmarkmenu
    tst_bookmarkmenu.cpp
    ${APP_SOURCE_DIR}/BookmarkMenu/BookmarkMenu.cpp
    ${APP_SOURCE_DIR}/BookmarkMenu/BookmarkList.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_gitcommitlist
    tst_gitcommitlist.cpp
    ${APP_SOURCE_DIR}/SourceControlDock/GitCommitList.cpp
)

add_app_test(tst_gitfilelist
    tst_gitfilelist.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_linttable
    tst_linttable.cpp
    ${APP_SOURCE_DIR}/LintMenu/LintTable.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_translationheader
    tst_translationheader.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTableHeader.cpp
    ${APP_SOURCE_DIR}/TranslationTable/LabelInput.cpp
)

add_app_test(tst_searchpanel
    tst_searchpanel.cpp
    ${APP_SOURCE_DIR}/SearchPanelDock/SearchPanelDock.cpp
    ${APP_SOURCE_DIR}/SearchPanelDock/SearchResultList.cpp
    ${APP_SOURCE_DIR}/TabPanel/TabListModel.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_translationinput
    tst_translationinput.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationInput.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationHighlighter.cpp
    ${APP_SOURCE_DIR}/TranslationTable/LintTooltip.cpp
    ${APP_SOURCE_DIR}/TabPanel/TabListModel.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_translationtable
    tst_translationtable.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTable.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTableDelegate.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTableHeader.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationTableModel.cpp
    ${APP_SOURCE_DIR}/TranslationTable/LabelInput.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationInput.cpp
    ${APP_SOURCE_DIR}/TranslationTable/TranslationHighlighter.cpp
    ${APP_SOURCE_DIR}/TranslationTable/LintTooltip.cpp
    ${APP_SOURCE_DIR}/TabPanel/TabListModel.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_linttooltip
    tst_linttooltip.cpp
    ${APP_SOURCE_DIR}/TranslationTable/LintTooltip.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_filemenus
    tst_filemenus.cpp
    ${APP_SOURCE_DIR}/FileSelectMenu/FileSelectMenu.cpp
    ${APP_SOURCE_DIR}/PersistentMenu/PersistentMenu.cpp
    ${APP_SOURCE_DIR}/SerdeMenu/SerdeMenu.cpp
    ${APP_SOURCE_DIR}/WriteMenu/WriteMenu.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_dialogs
    tst_dialogs.cpp
    ${APP_SOURCE_DIR}/AboutWindow/AboutWindow.cpp
    ${APP_SOURCE_DIR}/FileSelectMenu/FileSelectMenu.cpp
    ${APP_SOURCE_DIR}/PersistentMenu/PersistentMenu.cpp
    ${APP_SOURCE_DIR}/GlossaryMenu/GlossaryMenu.cpp
    ${APP_SOURCE_DIR}/LintSelectMenu/LintSelectMenu.cpp
    ${APP_SOURCE_DIR}/ReadMenu/ReadMenu.cpp
    ${APP_SOURCE_DIR}/SearchMenu/SearchMenu.cpp
    ${APP_SOURCE_DIR}/StatisticsDialog/StatisticsDialog.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_lintmenu
    tst_lintmenu.cpp
    ${APP_SOURCE_DIR}/FileSelectMenu/FileSelectMenu.cpp
    ${APP_SOURCE_DIR}/GlossaryMenu/GlossaryMenu.cpp
    ${APP_SOURCE_DIR}/LintMenu/LintMenu.cpp
    ${APP_SOURCE_DIR}/LintMenu/LintTable.cpp
    ${APP_SOURCE_DIR}/LintSelectMenu/LintSelectMenu.cpp
    ${APP_SOURCE_DIR}/PersistentMenu/PersistentMenu.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)
# LintMenu binds to the widgets of the main window's form
set_property(TARGET tst_lintmenu APPEND PROPERTY AUTOUIC_SEARCH_PATHS ${APP_SOURCE_DIR}/MainWindow)

add_app_test(tst_batchmenu
    tst_batchmenu.cpp
    ${APP_SOURCE_DIR}/AssetMenu/CodeViewer.cpp
    ${APP_SOURCE_DIR}/BatchMenu/BatchMenu.cpp
    ${APP_SOURCE_DIR}/BatchMenu/ScriptEditor.cpp
    ${APP_SOURCE_DIR}/FileSelectMenu/FileSelectMenu.cpp
    ${APP_SOURCE_DIR}/PersistentMenu/PersistentMenu.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

if (ENABLE_UPDATER)
    add_app_test(tst_autoupdater
        tst_autoupdater.cpp
        ${APP_SOURCE_DIR}/AutoUpdater/AutoUpdater.cpp
        ${APP_SOURCE_DIR}/utilities/Utils.cpp
        ${APP_SOURCE_DIR}/utilities/Notice.cpp
        ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
    )
endif()

add_app_test(tst_json
    tst_json.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

add_app_test(tst_taskworker
    tst_taskworker.cpp
    ${APP_SOURCE_DIR}/TaskWorker/TaskWorker.cpp
    ${APP_SOURCE_DIR}/TaskWorker/ScriptHost.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

# The main window pulls in nearly every component, so the tests that drive it link all of the application's sources
# but its entry point, compiled once here. The resources, the generated and the vendored files stay out: the first two
# belong to the application target, and the last are linked as libraries.
get_target_property(_app_sources ${PROJECT_NAME} SOURCES)
set(_core_sources)

foreach (_source IN LISTS _app_sources)
    cmake_path(ABSOLUTE_PATH _source BASE_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE _absolute)

    if (_absolute MATCHES "^${PROJECT_SOURCE_DIR}/src/" AND NOT _absolute MATCHES "/(3rdparty|main\\.cpp$|Allocator\\.cpp$)")
        list(APPEND _core_sources "${_absolute}")
    endif()
endforeach()

add_library(app_core OBJECT ${_core_sources})
target_include_directories(app_core PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
target_compile_definitions(app_core PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)
# The media player tests play on miniaudio's null backend, which the application leaves out
target_compile_definitions(app_core PRIVATE MA_ENABLE_NULL)
target_compile_options(app_core PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_OPTIONS>)
target_link_libraries(app_core PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>)
set_property(TARGET app_core APPEND PROPERTY AUTOUIC_SEARCH_PATHS ${APP_SOURCE_DIR}/MainWindow)
add_dependencies(app_core rust_header scripts_header)

if (ENABLE_COVERAGE)
    target_compile_options(app_core PRIVATE -fprofile-instr-generate -fcoverage-mapping)
endif()

# Without a catalog the main window warns that it cannot retranslate itself, in a modal box, at every start
qt6_add_lrelease(app_core
    TS_FILES "${PROJECT_SOURCE_DIR}/translations/rpgmtranslate.en.ts"
    QM_FILES_OUTPUT_VARIABLE _english_catalog
)
set_source_files_properties("${_english_catalog}" PROPERTIES QT_RESOURCE_ALIAS en.qm)
qt_add_resources(app_core catalog PREFIX "/" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${_english_catalog}")

add_app_test(tst_mainwindow
    tst_mainwindow_saveload.cpp
    $<TARGET_OBJECTS:app_core>
)

# The refusals of the credential store are made by a mock of it. Without GMock those tests are left out.
find_package(GTest CONFIG QUIET)

if (TARGET GTest::gmock)
    target_link_libraries(tst_mainwindow PRIVATE GTest::gmock)
    target_compile_definitions(tst_mainwindow PRIVATE HAVE_GMOCK)
endif()

# The refusals of the credential store are made by a mock of it. Without GMock those tests are left out.
find_package(GTest CONFIG QUIET)

if (TARGET GTest::gmock)
    target_link_libraries(tst_mainwindow PRIVATE GTest::gmock)
    target_compile_definitions(tst_mainwindow PRIVATE HAVE_GMOCK)
endif()

add_app_test(tst_mainwindow_flows
    tst_mainwindow_flows.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_mainwindow_connections
    tst_mainwindow_connections.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_smallwidgets
    tst_smallwidgets.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_sourcecontrol
    tst_sourcecontrol.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_mainwindow_search
    tst_mainwindow_search.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_mainwindow_scripts
    tst_mainwindow_scripts.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_settingswindow
    tst_settingswindow.cpp
    $<TARGET_OBJECTS:app_core>
)

if (TARGET GTest::gmock)
    target_link_libraries(tst_settingswindow PRIVATE GTest::gmock)
    target_compile_definitions(tst_settingswindow PRIVATE HAVE_GMOCK)
endif()

add_app_test(tst_assetmenu
    tst_assetmenu.cpp
    $<TARGET_OBJECTS:app_core>
)

add_app_test(tst_assetpreview
    tst_assetpreview.cpp
    $<TARGET_OBJECTS:app_core>
)

if (ENABLE_ASSET_PLAYBACK)
    add_app_test(tst_mediaplayer
        tst_mediaplayer.cpp
        $<TARGET_OBJECTS:app_core>
    )
endif()

add_app_test(tst_mainwindow_project
    tst_mainwindow_project.cpp
    $<TARGET_OBJECTS:app_core>
)

if (ENABLE_UPDATER)
    add_app_test(tst_mainwindow_updates
        tst_mainwindow_updates.cpp
        $<TARGET_OBJECTS:app_core>
    )
endif()

# The Rust half has its own unit tests, built with the same features as the application
find_program(CARGO_EXECUTABLE cargo)

if (CARGO_EXECUTABLE)
    set(_cargo_args test --lib --manifest-path "${PROJECT_SOURCE_DIR}/rust/Cargo.toml")

    if (RUST_NO_DEFAULT)
        list(APPEND _cargo_args --no-default-features)
    endif()

    # The search tests need a tokenizer, and `snowball` is the one without heavy dependencies
    set(_cargo_feature_list ${RUST_FEATURES})
    list(APPEND _cargo_feature_list snowball)
    list(REMOVE_DUPLICATES _cargo_feature_list)
    list(JOIN _cargo_feature_list "," _cargo_features)
    list(APPEND _cargo_args --features "${_cargo_features}")

    add_test(NAME rust COMMAND "${CARGO_EXECUTABLE}" ${_cargo_args})
    # Apart from the application's own cargo build, which uses another profile
    set_tests_properties(rust PROPERTIES ENVIRONMENT "CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/rust")
else()
    message(WARNING "cargo not found, the Rust tests are not registered")
endif()

if (ENABLE_COVERAGE)
    get_property(_coverage_tests GLOBAL PROPERTY COVERAGE_TESTS)

    set(_coverage_binaries)
    foreach (_test IN LISTS _coverage_tests)
        list(APPEND _coverage_binaries "$<TARGET_FILE:${_test}>")
    endforeach()
    list(JOIN _coverage_binaries "|" _coverage_binaries)

    # The Rust half is covered by `cargo llvm-cov`, which is not part of this report
    add_custom_target(coverage
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${COVERAGE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${COVERAGE_DIR}"
        COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure -E "^rust$"
        COMMAND "${CMAKE_COMMAND}"
            "-DCOVERAGE_DIR=${COVERAGE_DIR}"
            "-DBINARIES=${_coverage_binaries}"
            "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "-DLLVM_VERSION=${CMAKE_CXX_COMPILER_VERSION}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/Coverage.cmake"
        DEPENDS ${_coverage_tests}
        VERBATIM
    )
endif()
