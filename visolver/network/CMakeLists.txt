# Copyright Ben Paul Wise. All Rights Reserved.
# Flow-planning network layer (see network/plan.md): the vincpnet library on
# top of vincp, plus its GoogleTest suites. Added from the root CMakeLists via
# add_subdirectory(network), after GoogleTest is available.

add_library(vincpnet STATIC
        lib/instance.cpp
        lib/plan.cpp
        lib/greedy.cpp
        lib/reduction.cpp
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

# Same pattern as the root vincp_add_gtest, but sourced from network/test/ and
# linked against vincpnet.
function(vincpnet_add_gtest name)
    add_executable(${name} test/${name}.cpp)
    target_link_libraries(${name} PRIVATE vincpnet GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()

vincpnet_add_gtest(instance_test)
vincpnet_add_gtest(plan_test)
vincpnet_add_gtest(greedy_test)
vincpnet_add_gtest(reduction_test)
# Copyright Ben Paul Wise. All Rights Reserved.
