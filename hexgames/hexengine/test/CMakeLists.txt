# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. Every suite but PrngStreamsTest loads a real document set, so it needs
# HEXGAMES_SOURCE_DIR and the test directory on its include path for TrcFixture.h.
hexgames_add_gtest(hexengine_prng_test SOURCES PrngStreamsTest.cpp LIBS hexengine LABELS core)

foreach(_suite PhaseCursorTest OddsTableResolverTest SessionTest TextEventEncoderTest DeterminismTest)
  string(TOLOWER ${_suite} _lower)
  hexgames_add_gtest(hexengine_${_lower} SOURCES ${_suite}.cpp LIBS hexengine LABELS core)
  target_compile_definitions(hexengine_${_lower} PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_include_directories(hexengine_${_lower} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

hexgames_add_gtest(hexengine_parallel_rollout_test SOURCES ParallelRolloutTest.cpp LIBS hexengine
                    LABELS "core;long")
target_compile_definitions(hexengine_parallel_rollout_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(hexengine_parallel_rollout_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Copyright Ben Paul Wise. All Rights Reserved.
