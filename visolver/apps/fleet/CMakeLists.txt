# Copyright Ben Paul Wise. All Rights Reserved.
# Fleet application: the Fleet problem class (wraps the network QP + greedy /
# swap / sparsify) and, when Qt is available, the fleet_app GUI that reuses the
# shared viewer_common. Links vimcpnet (unmodified) for the solver library.

# --- Fleet application library (the reusable core; no Qt) ----------------------
add_library(fleetapp STATIC lib/fleetproblem.cpp)
target_include_directories(fleetapp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/apps/include
)
target_link_libraries(fleetapp PUBLIC vimcp vimcpnet)

if (MSVC)
    target_compile_options(fleetapp PRIVATE /W4)
else()
    target_compile_options(fleetapp PRIVATE -Wall -Wextra)
endif()

# --- headless test -------------------------------------------------------------
add_executable(fleet_problem_test test/fleet_problem_test.cpp)
target_link_libraries(fleet_problem_test PRIVATE fleetapp GTest::gtest_main)
gtest_discover_tests(fleet_problem_test)

# --- fleet_app GUI (added in Gate F2; skipped until gui/ exists) ----------------
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gui/CMakeLists.txt)
    add_subdirectory(gui)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
