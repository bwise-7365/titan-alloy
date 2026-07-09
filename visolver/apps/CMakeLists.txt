# Copyright Ben Paul Wise. All Rights Reserved.
# Focused application framework (VINCP::App): one reusable class per problem
# kind, wrapping the existing solver library without modifying it, plus the
# apps that demonstrate each class. Included from the root CMakeLists AFTER the
# GoogleTest setup and the gams/ subdirectory (the app libraries link vincp and
# vincpgms; the tests use gtest_discover_tests).

# --- SAOE application library (the reusable core) ------------------------------
# saoeproblem.cpp wraps the solver engines; pmatrixgms.cpp wraps the vincpgms
# front end to read a limited-subset GMS data file. Links vincp (solver core +
# Eigen include path) and vincpgms (GMS parser/evaluator).
add_library(saoeapp STATIC
    saoe/lib/saoeproblem.cpp
    saoe/lib/pmatrixgms.cpp
)
target_include_directories(saoeapp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/saoe/include
)
target_link_libraries(saoeapp PUBLIC vincp vincpgms)

if (MSVC)
    target_compile_options(saoeapp PRIVATE /W4)
else()
    target_compile_options(saoeapp PRIVATE -Wall -Wextra)
endif()

# --- pmatrix: the CLI app ------------------------------------------------------
add_executable(pmatrix saoe/src/pmatrix.cpp)
target_link_libraries(pmatrix PRIVATE saoeapp)
if (MSVC)
    target_compile_options(pmatrix PRIVATE /W4)
else()
    target_compile_options(pmatrix PRIVATE -Wall -Wextra)
endif()

# --- GoogleTest suites ---------------------------------------------------------
add_executable(saoe_problem_test saoe/test/saoe_problem_test.cpp)
target_link_libraries(saoe_problem_test PRIVATE saoeapp GTest::gtest_main)
gtest_discover_tests(saoe_problem_test)

add_executable(pmatrix_gms_test saoe/test/pmatrix_gms_test.cpp)
target_link_libraries(pmatrix_gms_test PRIVATE saoeapp GTest::gtest_main)
# The reader test opens apps/saoe/doc/pmatrix-example.gms in place.
target_compile_definitions(pmatrix_gms_test PRIVATE
    VINCP_APPS_SAOE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/saoe")
gtest_discover_tests(pmatrix_gms_test)

# --- Fleet application (problem class + headless test; GUI when Qt is present) --
add_subdirectory(fleet)

# --- PFORM application (parliament-formation model over SAOE + pform CLI) --------
# The PForm GUI is a pending task (no gui/ subdir yet).
add_subdirectory(pform)
# Copyright Ben Paul Wise. All Rights Reserved.
