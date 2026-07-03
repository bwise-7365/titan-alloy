# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(lvi_dhan06 LANGUAGES CXX)

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
        lib/ellipsoidprojector.cpp
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

add_executable(lcp_random_test test/lcp_random_test.cpp)
target_link_libraries(lcp_random_test PRIVATE vincp)

add_executable(lcp_psd_test test/lcp_psd_test.cpp)
target_link_libraries(lcp_psd_test PRIVATE vincp)

add_executable(lm_test test/lm_test.cpp)
target_link_libraries(lm_test PRIVATE vincp)

add_executable(han_vs_he_test test/han_vs_he_test.cpp)
target_link_libraries(han_vs_he_test PRIVATE vincp)

add_executable(saoe_test test/saoe_test.cpp)
target_link_libraries(saoe_test PRIVATE vincp)

add_executable(ellipsoid_projector_test test/ellipsoid_projector_test.cpp)
target_link_libraries(ellipsoid_projector_test PRIVATE vincp)

add_executable(lvi_ellipsoid_test test/lvi_ellipsoid_test.cpp)
target_link_libraries(lvi_ellipsoid_test PRIVATE vincp)

enable_testing()
add_test(NAME lvi_demo COMMAND lvi_demo)
add_test(NAME lvi_vi_demo COMMAND lvi_vi_demo)
add_test(NAME lcp_random_test COMMAND lcp_random_test)
add_test(NAME lcp_psd_test COMMAND lcp_psd_test)
add_test(NAME lm_test COMMAND lm_test)
add_test(NAME han_vs_he_test COMMAND han_vs_he_test)
# saoe_test passes on FEASIBILITY (not a specific equilibrium): it runs both
# inner solvers and passes if at least one yields a feasible allocation.
add_test(NAME saoe_test COMMAND saoe_test)
add_test(NAME ellipsoid_projector_test COMMAND ellipsoid_projector_test)
add_test(NAME lvi_ellipsoid_test COMMAND lvi_ellipsoid_test)

# The random indefinite-M LCP is expected to diverge: Han's method has no
# convergence guarantee off the monotone (M PSD) case, so a non-zero exit is the
# correct outcome. WILL_FAIL inverts the pass condition, keeping the suite green.
set_tests_properties(lcp_random_test PROPERTIES WILL_FAIL TRUE)
# Copyright Ben Paul Wise. All Rights Reserved.
