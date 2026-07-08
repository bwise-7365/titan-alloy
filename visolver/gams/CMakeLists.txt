# Copyright Ben Paul Wise. All Rights Reserved.
# GAMS-subset front end (gates GP1..GP5; plan of record:
# doc/2026-07-08-gams-frontend-plan.md). GP1 is parsing only: the vincpgms
# library depends on nothing but the standard library (no Eigen, no vincp);
# the solver-facing builder arrives at GP3. Included from the root
# CMakeLists AFTER the GoogleTest setup (gtest_discover_tests is used here).
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
)
target_include_directories(vincpgms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

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

# CLion-friendly aggregate: build + run just the GAMS front-end suites.
add_custom_target(run_gams_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "GmsParse" --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS gms_parse_test
    USES_TERMINAL
)
# Copyright Ben Paul Wise. All Rights Reserved.
