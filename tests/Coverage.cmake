# Turns the raw profiles the instrumented tests wrote into a report. Run by the `coverage` target:
#
#   COVERAGE_DIR  where the `.profraw` files are, and where the reports go
#   BINARIES      the test executables, separated by `|`
#   SOURCE_DIR    the repository root
#   LLVM_VERSION  the version of the compiler, whose major number picks the matching LLVM tools: the formats differ
#                 between them

string(REGEX MATCH "^[0-9]+" LLVM_VERSION "${LLVM_VERSION}")

find_program(LLVM_PROFDATA NAMES llvm-profdata-${LLVM_VERSION} llvm-profdata REQUIRED)
find_program(LLVM_COV NAMES llvm-cov-${LLVM_VERSION} llvm-cov REQUIRED)

file(GLOB profiles "${COVERAGE_DIR}/*.profraw")

if (NOT profiles)
    message(FATAL_ERROR "No .profraw files in ${COVERAGE_DIR}: the tests did not run, or were not built with ENABLE_COVERAGE=ON")
endif()

set(merged "${COVERAGE_DIR}/all.profdata")
execute_process(
    COMMAND "${LLVM_PROFDATA}" merge -sparse ${profiles} -o "${merged}"
    COMMAND_ERROR_IS_FATAL ANY
)

string(REPLACE "|" ";" binaries "${BINARIES}")
list(POP_FRONT binaries first_binary)

set(coverage_args "${first_binary}")
foreach (binary IN LISTS binaries)
    list(APPEND coverage_args "-object=${binary}")
endforeach()

# Only the application's own sources count: not the vendored code, the generated code or the tests themselves
set(ignore_regex ".*/(3rdparty|tests|build[^/]*|[^/]*_autogen)/.*|.*/(moc_|qrc_|mocs_compilation).*")
list(APPEND coverage_args
    "-instr-profile=${merged}"
    "-ignore-filename-regex=${ignore_regex}"
    "${SOURCE_DIR}/src"
)

execute_process(
    COMMAND "${LLVM_COV}" report ${coverage_args}
    OUTPUT_VARIABLE report
    COMMAND_ERROR_IS_FATAL ANY
)
file(WRITE "${COVERAGE_DIR}/report.txt" "${report}")
message("${report}")

execute_process(
    COMMAND "${LLVM_COV}" show ${coverage_args} -format=html "-output-dir=${COVERAGE_DIR}/html" -show-line-counts-or-regions
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
)
execute_process(
    COMMAND "${LLVM_COV}" export ${coverage_args} -format=lcov
    OUTPUT_FILE "${COVERAGE_DIR}/lcov.info"
    COMMAND_ERROR_IS_FATAL ANY
)

# A file no test compiles has no coverage data at all, so the report above never mentions it. List those: they are the
# real zeroes.
file(GLOB_RECURSE all_sources RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/src/*.cpp")
set(missing)
foreach (source IN LISTS all_sources)
    if (source MATCHES "^src/3rdparty/|^src/main\\.cpp$")
        continue()
    endif()

    get_filename_component(name "${source}" NAME)
    string(FIND "${report}" "${name}" found)
    if (found EQUAL -1)
        list(APPEND missing "${source}")
    endif()
endforeach()

list(LENGTH missing missing_count)
string(REPLACE ";" "\n  " missing_text "${missing}")
file(WRITE "${COVERAGE_DIR}/untested-sources.txt" "${missing_text}\n")
message("Sources no test compiles (${missing_count}), listed in ${COVERAGE_DIR}/untested-sources.txt:\n  ${missing_text}")
message("HTML report: ${COVERAGE_DIR}/html/index.html")
message("lcov data:   ${COVERAGE_DIR}/lcov.info")
