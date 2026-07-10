# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(visolver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Eigen is header-only: no build or install step is needed, only its headers on
# the include path. EIGEN3_INCLUDE_DIR is the directory that CONTAINS the "Eigen"
# folder, so that <Eigen/Dense> resolves -- i.e. the Eigen checkout root, NOT the
# "Eigen" subfolder itself. It is a cache entry; override it on the command line
    # or in CLion's CMake options for a different location (e.g. /usr/include/eigen3
# on Debian, from sudo apt install libeigen3-dev).
set(EIGEN3_INCLUDE_DIR "C:/repos/ghub-ext/eigen" CACHE PATH
    "Directory containing the Eigen/ header folder (the Eigen source root).")
if (NOT EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Dense")
    message(FATAL_ERROR
        "Eigen headers not found: '${EIGEN3_INCLUDE_DIR}/Eigen/Dense' is missing. "
        "Set EIGEN3_INCLUDE_DIR to the directory that contains the 'Eigen' folder.")
endif()

add_library(vincp STATIC
        lib/vincp.cpp
        lib/dhan06.cpp
        lib/bshe94b.cpp
        lib/fdjacobian.cpp
        lib/armijo.cpp
        lib/levenbergmarquardt.cpp
        lib/josephynewton.cpp
        lib/saoe.cpp
        lib/solodovsvaiter.cpp
        lib/chainedsolver.cpp
        lib/fbshyz04.cpp
        lib/mehrotraipm.cpp
        lib/semismoothnewton.cpp
        lib/alternatingchain.cpp
        lib/chooseengine.cpp
        lib/ellipsoidprojector.cpp
        lib/dampednewton.cpp
        lib/smoothingnewton.cpp
        lib/utils.cpp
)
target_include_directories(vincp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# SYSTEM so Eigen's own headers are not subject to /W4 and -Wall -Wextra.
target_include_directories(vincp SYSTEM
    PUBLIC ${EIGEN3_INCLUDE_DIR}
)

if (MSVC)
    target_compile_options(vincp PRIVATE /W4)
else()
    target_compile_options(vincp PRIVATE -Wall -Wextra)
endif()

# --- GoogleTest, fetched at configure time (source, built from source here) ---
# FetchContent git-clones the pinned tag into build/_deps and add_subdirectory's
# it, so GoogleTest is compiled by THIS toolchain with THESE flags -- no vendored
# or prebuilt binaries. The pin (a release tag, not a branch) keeps the dependency
# reproducible; bump it deliberately to update. gtest_force_shared_crt makes
# GoogleTest link the dynamic CRT (/MD) MSVC uses by default; without it the link
# fails with CRT-mismatch errors on Windows.
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)         # provides gtest_discover_tests()

enable_testing()           # must precede the gtest_discover_tests() calls below

# test/ holds test-only headers (e.g. testsupport.hpp) that pull in <gtest/gtest.h>;
# kept off the vincp target so GoogleTest never leaks into the library.
set(VINCP_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

# Declare a GoogleTest executable: test/<name>.cpp -> target <name>, linked against
# vincp + gtest_main, with test/ on its include path and its TEST() cases auto-
# registered with ctest (each shows up individually as Suite.Case).
function(vincp_add_gtest name)
    add_executable(${name} test/${name}.cpp)
    target_link_libraries(${name} PRIVATE vincp GTest::gtest_main)
    target_include_directories(${name} PRIVATE ${VINCP_TEST_INCLUDE_DIR})
    gtest_discover_tests(${name})
endfunction()

add_executable(lvi_demo src/main.cpp)
target_link_libraries(lvi_demo PRIVATE vincp)

add_executable(lvi_vi_demo src/driver_demo.cpp)
target_link_libraries(lvi_vi_demo PRIVATE vincp)

# Run-for-printout demo (not a ctest gate; the pass/fail check is han_vs_he_test).
add_executable(han_vs_he_demo src/han_vs_he_demo.cpp)
target_link_libraries(han_vs_he_demo PRIVATE vincp)

# SAOE (Strategic Allocation Of Effort) demo (run for printout).
add_executable(saoe_demo src/saoe_demo.cpp)
target_link_libraries(saoe_demo PRIVATE vincp)

# GoogleTest suites. Each is a TEST()-based executable; gtest_discover_tests()
# registers its cases with ctest individually. The lcp_random_test suite asserts
# the divergence itself (EXPECT_THROW on the indefinite-M instance), so it no
# longer needs the WILL_FAIL exit-code inversion that the plain-exe version did.
vincp_add_gtest(lcp_random_test)
vincp_add_gtest(lcp_psd_test)
vincp_add_gtest(lm_test)
vincp_add_gtest(han_vs_he_test)
vincp_add_gtest(saoe_test)
vincp_add_gtest(ellipsoid_projector_test)
vincp_add_gtest(lvi_ellipsoid_test)
vincp_add_gtest(damped_newton_test)
vincp_add_gtest(smoothing_newton_test)
vincp_add_gtest(ss_lcp_test)
vincp_add_gtest(ss_ellipsoid_test)
vincp_add_gtest(mehrotra_ipm_test)
vincp_add_gtest(semismooth_newton_test)
vincp_add_gtest(josephy_newton_test)
vincp_add_gtest(alternating_chain_test)
vincp_add_gtest(saoe_chain_test)
vincp_add_gtest(saoe_risk_test)
vincp_add_gtest(choose_engine_test)
vincp_add_gtest(fbs_hyz04_test)
# Translated GMS-corpus MCP models (convergence-gated, multi-engine; see the
# test headers for the .gms sources and the variable/equation mappings).
vincp_add_gtest(gms_alloceff_test)
vincp_add_gtest(gms_deploy_test)

# Flow-planning network layer (network/plan.md): vincpnet library + its tests.
# Must come after the GoogleTest setup above (it uses gtest_discover_tests).
add_subdirectory(network)

# GMS-subset front end (doc/2026-07-08-gams-frontend-plan.md): vincpgms
# library + its tests. Also needs the GoogleTest setup above.
add_subdirectory(gms)

# Focused application framework (apps/): VINCP::App problem classes + their
# CLI/GUI apps. Additive; links vincp + vincpgms, so it must follow both.
option(VINCP_BUILD_APPS "Build the focused app framework under apps/" ON)
if (VINCP_BUILD_APPS)
    add_subdirectory(apps)
endif()

# CLion-friendly aggregates (the pattern of network/CMakeLists' run_network_tests).
# "run_engine_tests" builds and runs the suites the MC2/MC3 engine work touches
# (semismooth + IPM + the four-row comparison + the network engine switch);
# "run_all_tests" builds and runs the entire ctest suite.
add_custom_target(run_engine_tests
    COMMAND ${CMAKE_CTEST_COMMAND}
        -R "SemismoothNewton|MehrotraIpm|HanVsHe|NetworkFlowPlan|GmsAlloceff|GmsDeploy|AlternatingChain|SaoeChain|SaoeRisk|ChooseEngine"
        --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS semismooth_newton_test mehrotra_ipm_test han_vs_he_test flowplan_test
            gms_alloceff_test gms_deploy_test alternating_chain_test
            saoe_chain_test saoe_risk_test choose_engine_test
    USES_TERMINAL
)
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS lcp_random_test lcp_psd_test lm_test han_vs_he_test saoe_test
            ellipsoid_projector_test lvi_ellipsoid_test damped_newton_test
            smoothing_newton_test ss_lcp_test ss_ellipsoid_test
            mehrotra_ipm_test semismooth_newton_test josephy_newton_test
            alternating_chain_test saoe_chain_test saoe_risk_test
            choose_engine_test fbs_hyz04_test
            gms_alloceff_test gms_deploy_test gms_parse_test gms_eval_test
            gms_model_test gms_parity_test gms_solve_test
            network_tests lvi_demo lvi_vi_demo
    USES_TERMINAL
)

# The two demo binaries are still run as plain smoke tests (exit 0 == pass); they
# are not GoogleTest suites.
add_test(NAME lvi_demo COMMAND lvi_demo)
add_test(NAME lvi_vi_demo COMMAND lvi_vi_demo)
# Copyright Ben Paul Wise. All Rights Reserved.
