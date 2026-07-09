# Copyright Ben Paul Wise. All Rights Reserved.
# PFORM application: the parliament-formation sub-model over the SAOE class,
# plus the pform CLI. Links saoeapp (the App::SAOE class + vincp) and vincpgms
# (the GMS reader). The PForm GUI is a PENDING task (see apps/pform, no gui/ yet).

# --- PFORM application library (the reusable core) -----------------------------
add_library(pformapp STATIC
    lib/pformproblem.cpp
    lib/pformgms.cpp
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

# --- pform: the CLI app --------------------------------------------------------
add_executable(pform src/pform.cpp)
target_link_libraries(pform PRIVATE pformapp)
if (MSVC)
    target_compile_options(pform PRIVATE /W4)
else()
    target_compile_options(pform PRIVATE -Wall -Wextra)
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
# Copyright Ben Paul Wise. All Rights Reserved.
