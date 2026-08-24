# Copyright Ben Paul Wise. All Rights Reserved.

add_library(latrunculi_lib STATIC
    Game.cpp Eval.cpp PlacementEval.cpp PlacementPolicy.cpp GameStats.cpp GameXml.cpp)
target_include_directories(latrunculi_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(latrunculi_lib PUBLIC cxx_std_20)
target_link_libraries(latrunculi_lib PUBLIC abs_game)

# Placement-evaluation unit tests: the seven diagram fixtures from
# doc/2026-08-24-latrunculi-placement-heuristics.md plus the Game wiring checks.
# Same self-contained-harness pattern as palette_core/tests (no framework fetch).
enable_testing()
add_executable(latrunculi_test test_placement_eval.cpp)
target_link_libraries(latrunculi_test PRIVATE latrunculi_lib)
add_test(NAME placement_eval COMMAND latrunculi_test)

# Self-play validation driver: prints the move log and writes one PNG per move
# (pos_NN.png), rasterising the irregular_grids SVG via Qt's QSvgRenderer.
find_package(Qt6 REQUIRED COMPONENTS Gui Svg)
add_executable(latrunculi_selfplay selfplay.cpp)
target_link_libraries(latrunculi_selfplay PRIVATE
    latrunculi_lib irregular_grid Qt6::Gui Qt6::Svg)

# Instrumented batch self-play: plays N games, prints game-quality metrics and can write
# each game as XML. No Qt and no per-ply rendering, so it can be run in bulk to compare
# rule sets and eval weights, and so it can carry the MB2 memory tracker (MemTrack.h).
add_executable(latrunculi_bench bench.cpp BenchAb.cpp)
target_link_libraries(latrunculi_bench PRIVATE latrunculi_lib mem_track)

# Copyright Ben Paul Wise. All Rights Reserved.
