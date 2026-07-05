# Copyright Ben Paul Wise. All Rights Reserved.
# Flow-planning network layer (see network/plan.md): the vincpnet library on
# top of vincp, plus its GoogleTest suites. Added from the root CMakeLists via
# add_subdirectory(network), after GoogleTest is available.

add_library(vincpnet STATIC
        lib/instance.cpp
        lib/plan.cpp
        lib/greedy.cpp
        lib/reduction.cpp
        lib/flowlcp.cpp
        lib/oracle.cpp
        lib/flowplan.cpp
        lib/config.cpp
        lib/swap.cpp
        lib/gravity.cpp
)
target_include_directories(vincpnet
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# vincp is PUBLIC: vincpnet headers include vincp.hpp, so dependents need the
# vincp include path (and Eigen, which vincp carries as SYSTEM) transitively.
target_link_libraries(vincpnet PUBLIC vincp)

if (MSVC)
    target_compile_options(vincpnet PRIVATE /W4)
else()
    target_compile_options(vincpnet PRIVATE -Wall -Wextra)
endif()

# Same pattern as the root vincp_add_gtest, but sourced from network/test/,
# linked against vincpnet, and recorded in NETWORK_TEST_TARGETS so the
# aggregate targets below cover every network test automatically.
set(NETWORK_TEST_TARGETS "")
function(vincpnet_add_gtest name)
    add_executable(${name} test/${name}.cpp)
    target_link_libraries(${name} PRIVATE vincpnet GTest::gtest_main)
    gtest_discover_tests(${name})
    set(NETWORK_TEST_TARGETS ${NETWORK_TEST_TARGETS} ${name} PARENT_SCOPE)
endfunction()

vincpnet_add_gtest(instance_test)
vincpnet_add_gtest(plan_test)
vincpnet_add_gtest(greedy_test)
vincpnet_add_gtest(reduction_test)
vincpnet_add_gtest(flowlcp_test)
vincpnet_add_gtest(oracle_test)
vincpnet_add_gtest(flowplan_test)
vincpnet_add_gtest(scale_test)
vincpnet_add_gtest(config_test)
vincpnet_add_gtest(swap_test)
vincpnet_add_gtest(gravity_test)

# Scale/performance sweep (task C5): a plain executable, NOT a ctest -- run
# it manually in a Release configuration for meaningful timings.
add_executable(network_benchmark src/benchmark.cpp)
target_link_libraries(network_benchmark PRIVATE vincpnet)

# Optional Qt6 instance viewer. ON by default but self-disabling when Qt6 is
# absent (gui/CMakeLists.txt returns early), so the core solver/test build never
# requires Qt. Turn it off explicitly with -DVINCPNET_BUILD_GUI=OFF.
option(VINCPNET_BUILD_GUI "Build the Qt6 flow-plan instance viewer" ON)
if (VINCPNET_BUILD_GUI)
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
