find_program(METAVIRT_LCOV_EXEC lcov)
find_program(METAVIRT_GENHTML_EXEC genhtml)

if(METAVIRT_LCOV_EXEC-NOTFOUND OR METAVIRT_GENHTML_EXEC-NOTFOUND)
    message(WARNING "lcov and genhtml command needed for coverage.")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    metavirt_find_llvm_progs(METAVIRT_LLVMCOV_EXEC "llvm-cov-${LLVM_VERSION_MAJOR};llvm-cov")
    if(NOT METAVIRT_LLVMCOV_EXEC)
        message(FATAL_ERROR "Did not find llvm-cov, which is required for Clang-based LCOV coverage.")
    endif()
    # workaround lcov and clang --coverage have a version mismatch
    file(
        GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/script/llvm-gcov.sh
        CONTENT "#!/usr/bin/env bash\n\n${METAVIRT_LLVMCOV_EXEC} gcov \"$@\"\n"
        FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ
        WORLD_READ
    )
    set(GCOV_TOOL --gcov-tool ${CMAKE_BINARY_DIR}/script/llvm-gcov.sh)
endif()

set(
    METAVIRT_COVERAGE_TRACE_COMMAND
    ${METAVIRT_LCOV_EXEC} ${GCOV_TOOL} --rc branch_coverage=1 --rc derive_function_end_line=0 -c -q
    -o "${PROJECT_BINARY_DIR}/coverage.info"
    -d "${PROJECT_BINARY_DIR}"
    --include "${PROJECT_SOURCE_DIR}/*"
    CACHE STRING
    "; Command to generate a trace for the 'metavirt_coverage' target"
)

set(
    METAVIRT_COVERAGE_HTML_COMMAND
    ${METAVIRT_GENHTML_EXEC} --rc branch_coverage=1 --rc derive_function_end_line=0 --legend -f -q
    "${PROJECT_BINARY_DIR}/coverage.info"
    -p "${PROJECT_SOURCE_DIR}"
    -o "${PROJECT_BINARY_DIR}/coverage_html"
    CACHE STRING
    "; Command to generate an HTML report for the 'metavirt_coverage' target"
)

add_custom_target(
    metavirt-coverage
    COMMAND ${METAVIRT_COVERAGE_TRACE_COMMAND}
    COMMAND ${METAVIRT_COVERAGE_HTML_COMMAND}
    COMMENT "Generating metavirt coverage report"
    VERBATIM
)

add_custom_target(
    metavirt-coverage-clean
    COMMAND ${METAVIRT_LCOV_EXEC} -d ${CMAKE_BINARY_DIR} -z
)

