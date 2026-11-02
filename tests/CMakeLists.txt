# Tests are opt-in (-DENABLE_TESTS=ON) and run with `ctest`. They borrow the include directories, definitions and
# libraries of the application target instead of repeating them, so this directory has to be added after that target is
# fully set up.

set(APP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")

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

    # The generated headers the sources include
    add_dependencies(${name} rust_header scripts_header)

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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

add_app_test(tst_utils
    tst_utils.cpp
    ${APP_SOURCE_DIR}/utilities/Utils.cpp
    ${APP_SOURCE_DIR}/utilities/Notice.cpp
    ${APP_SOURCE_DIR}/utilities/QtStrings.cpp
)

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
