# Copyright Ben Paul Wise. All Rights Reserved.
# PFORM application: the parliament-formation sub-model over the SAOE class,
# plus the pform_cli CLI and the pform_gui Qt viewer (gui/, built only when
# Qt6 is available). Links saoeapp (the App::SAOE class + vincp) and vincpgms
# (the GMS reader). Both front ends render through the shared pformtext layer.

# --- PFORM application library (the reusable core) -----------------------------
add_library(pformapp STATIC
    lib/pformproblem.cpp
    lib/pformgms.cpp
    lib/pformtext.cpp
)
target_include_directories(pformapp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/apps/include
)
# saoeapp carries vincp + the App::SAOE headers (apps/include, apps/saoe/include);
# vincpgms carries the GMS front end.
target_link_libraries(pformapp PUBLIC saoeapp vincpgms)

if (MSVC)
    target_compile_options(pformapp PRIVATE /W4)
else()
    target_compile_options(pformapp PRIVATE -Wall -Wextra)
endif()

# --- pform_cli: the CLI app ------------------------------------------------------
add_executable(pform_cli src/pform_cli.cpp)
target_link_libraries(pform_cli PRIVATE pformapp)
if (MSVC)
    target_compile_options(pform_cli PRIVATE /W4)
else()
    target_compile_options(pform_cli PRIVATE -Wall -Wextra)
endif()

# --- GoogleTest suites ---------------------------------------------------------
add_executable(pform_problem_test test/pform_problem_test.cpp)
target_link_libraries(pform_problem_test PRIVATE pformapp GTest::gtest_main)
gtest_discover_tests(pform_problem_test)

add_executable(pform_gms_test test/pform_gms_test.cpp)
target_link_libraries(pform_gms_test PRIVATE pformapp GTest::gtest_main)
# The reader test opens apps/pform/doc/pform-example.gms in place.
target_compile_definitions(pform_gms_test PRIVATE
    VINCP_APPS_PFORM_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
gtest_discover_tests(pform_gms_test)

add_executable(pform_text_test test/pform_text_test.cpp)
target_link_libraries(pform_text_test PRIVATE pformapp GTest::gtest_main)
gtest_discover_tests(pform_text_test)

# --- pform_gui: the Qt viewer (skipped when Qt6 is not available) ----------------
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gui/CMakeLists.txt)
    add_subdirectory(gui)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
