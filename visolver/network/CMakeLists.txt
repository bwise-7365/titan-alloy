# Copyright Ben Paul Wise. All Rights Reserved.
# Flow-planning network layer (see network/plan.md): the vimcpnet library on
# top of vimcp, plus its GoogleTest suites. Added from the root CMakeLists via
# add_subdirectory(network), after GoogleTest is available.

add_library(vimcpnet STATIC
        lib/instance.cpp
        lib/plan.cpp
        lib/greedy.cpp
        lib/fleetinstance.cpp
        lib/fleetplan.cpp
        lib/fleetgreedy.cpp
        lib/fleetswap.cpp
        lib/fleetreduction.cpp
        lib/fleetlcp.cpp
        lib/fleetnewton.cpp
        lib/fleetsolve.cpp
        lib/reduction.cpp
        lib/flowlcp.cpp
        lib/flownewton.cpp
        lib/oracle.cpp
        lib/flowplan.cpp
        lib/config.cpp
        lib/swap.cpp
        lib/gravity.cpp
)
target_include_directories(vimcpnet
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# vimcp is PUBLIC: vimcpnet headers include vimcp.hpp, so dependents need the
# vimcp include path (and Eigen, which vimcp carries as SYSTEM) transitively.
target_link_libraries(vimcpnet PUBLIC vimcp)

if (MSVC)
    target_compile_options(vimcpnet PRIVATE /W4)
else()
    target_compile_options(vimcpnet PRIVATE -Wall -Wextra)
endif()

# Same pattern as the root vimcp_add_gtest, but sourced from network/test/,
# linked against vimcpnet, and recorded in NETWORK_TEST_TARGETS so the
# aggregate targets below cover every network test automatically.
set(NETWORK_TEST_TARGETS "")
function(vimcpnet_add_gtest name)
    add_executable(${name} test/${name}.cpp)
    target_link_libraries(${name} PRIVATE vimcpnet GTest::gtest_main)
    gtest_discover_tests(${name})
    set(NETWORK_TEST_TARGETS ${NETWORK_TEST_TARGETS} ${name} PARENT_SCOPE)
endfunction()

vimcpnet_add_gtest(instance_test)
vimcpnet_add_gtest(plan_test)
vimcpnet_add_gtest(greedy_test)
vimcpnet_add_gtest(fleetinstance_test)
vimcpnet_add_gtest(fleetplan_test)
vimcpnet_add_gtest(fleetgreedy_test)
vimcpnet_add_gtest(fleetswap_test)
vimcpnet_add_gtest(fleetreduction_test)
vimcpnet_add_gtest(fleetlcp_test)
vimcpnet_add_gtest(fleetnewton_test)
vimcpnet_add_gtest(fleetsolve_test)
vimcpnet_add_gtest(reduction_test)
vimcpnet_add_gtest(flowlcp_test)
vimcpnet_add_gtest(flownewton_test)
vimcpnet_add_gtest(oracle_test)
vimcpnet_add_gtest(flowplan_test)
vimcpnet_add_gtest(scale_test)
vimcpnet_add_gtest(config_test)
vimcpnet_add_gtest(swap_test)
vimcpnet_add_gtest(gravity_test)

# Scale/performance sweep (task C5): a plain executable, NOT a ctest -- run
# it manually in a Release configuration for meaningful timings.
add_executable(network_benchmark src/benchmark.cpp)
target_link_libraries(network_benchmark PRIVATE vimcpnet)

# Instrumented fleet-optimizer probe (stage FP0 of the 2026-07-08 fleet
# performance plan): also a plain executable, Release timings only.
add_executable(fleet_benchmark src/fleetbenchmark.cpp)
target_link_libraries(fleet_benchmark PRIVATE vimcpnet)

# Optional Qt6 instance viewer. ON by default but self-disabling when Qt6 is
# absent (gui/CMakeLists.txt returns early), so the core solver/test build never
# requires Qt. Turn it off explicitly with -DVIMCPNET_BUILD_GUI=OFF.
option(VIMCPNET_BUILD_GUI "Build the Qt6 flow-plan instance viewer" ON)
if (VIMCPNET_BUILD_GUI)
    add_subdirectory(gui)
endif()

# CLion-friendly aggregates. "network_tests" just BUILDS every network test
# executable. "run_network_tests" builds them and then RUNS ctest restricted
# to the network suites -- every network TEST() suite name starts with
# "Network", so the base library's suites (LcpPsd, ...) are excluded.
add_custom_target(network_tests DEPENDS ${NETWORK_TEST_TARGETS})
add_custom_target(run_network_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^Network" --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${NETWORK_TEST_TARGETS}
    USES_TERMINAL
)
# Copyright Ben Paul Wise. All Rights Reserved.
