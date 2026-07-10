# Copyright Ben Paul Wise. All Rights Reserved.
# GMS-subset front end (gates GP1..GP5; plan of record:
# doc/2026-07-08-gams-frontend-plan.md). The lexer/parser/evaluator layers
# are standard-library only; GP3's model builder (gmsmcp) is the designed
# integration point with the solver core, so vincpgms links vincp (which
# also supplies the Eigen include path). Included from the root CMakeLists
# AFTER the GoogleTest setup (gtest_discover_tests is used here).
#
# "GAMS" is a registered trademark of GAMS Development Corporation. This code
# is not endorsed or certified by GAMS Development Corporation. The subset of
# the GMS parsed by this code is incompatible with most of the GAMS modeling
# language. The software is provided without warranty of any kind, express or
# implied, including without limitation for any particular purpose. The
# provider makes no guarantees about its performance, accuracy, or
# suitability for any specific application.

add_library(vincpgms STATIC
    lib/gmslexer.cpp
    lib/gmsparser.cpp
    lib/gmsecho.cpp
    lib/gmsdatabase.cpp
    lib/gmseval.cpp
    lib/gmsmcp.cpp
)
target_include_directories(vincpgms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vincpgms PUBLIC vincp)

if (MSVC)
    target_compile_options(vincpgms PRIVATE /W4)
else()
    target_compile_options(vincpgms PRIVATE -Wall -Wextra)
endif()

# The parser tests read the real corpus files from doc/ in place (reuse over
# duplicate); the source path travels as a compile-time constant.
add_executable(gms_parse_test test/gms_parse_test.cpp)
target_link_libraries(gms_parse_test PRIVATE vincpgms GTest::gtest_main)
target_compile_definitions(gms_parse_test PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")
gtest_discover_tests(gms_parse_test)

add_executable(gms_eval_test test/gms_eval_test.cpp)
target_link_libraries(gms_eval_test PRIVATE vincpgms GTest::gtest_main)
target_compile_definitions(gms_eval_test PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")
gtest_discover_tests(gms_eval_test)

add_executable(gms_model_test test/gms_model_test.cpp)
target_link_libraries(gms_model_test PRIVATE vincpgms GTest::gtest_main)
target_compile_definitions(gms_model_test PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")
gtest_discover_tests(gms_model_test)

add_executable(gms_parity_test test/gms_parity_test.cpp)
target_link_libraries(gms_parity_test PRIVATE vincpgms GTest::gtest_main)
target_compile_definitions(gms_parity_test PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")
gtest_discover_tests(gms_parity_test)

add_executable(gms_solve_test test/gms_solve_test.cpp)
target_link_libraries(gms_solve_test PRIVATE vincpgms GTest::gtest_main)
target_compile_definitions(gms_solve_test PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")
gtest_discover_tests(gms_solve_test)

# GP5 bounded probe (manual, fleet-benchmark style; NOT a ctest gate): the
# three expensive corpus solves with timings and GAMS-listing comparisons.
add_executable(gms_gp5_probe src/gms_gp5_probe.cpp)
target_link_libraries(gms_gp5_probe PRIVATE vincpgms)
target_compile_definitions(gms_gp5_probe PRIVATE
    VINCP_GMS_CORPUS_DIR="${CMAKE_SOURCE_DIR}/doc")

# CLion-friendly aggregate: build + run just the GMS front-end suites.
add_custom_target(run_gms_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
        -R "GmsParse|GmsEval|GmsModel|GmsParity|GmsSolve"
        --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS gms_parse_test gms_eval_test gms_model_test gms_parity_test
            gms_solve_test
    USES_TERMINAL
)
# Copyright Ben Paul Wise. All Rights Reserved.
