# Copyright Ben Paul Wise. All Rights Reserved.

add_library(latrunculi_lib STATIC Game.cpp Eval.cpp PlacementPolicy.cpp GameStats.cpp)
target_include_directories(latrunculi_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(latrunculi_lib PUBLIC cxx_std_20)
target_link_libraries(latrunculi_lib PUBLIC abs_game)

# Self-play validation driver: prints the move log and writes one PNG per move
# (pos_NN.png), rasterising the irregular_grids SVG via Qt's QSvgRenderer.
find_package(Qt6 REQUIRED COMPONENTS Gui Svg)
add_executable(latrunculi_selfplay selfplay.cpp)
target_link_libraries(latrunculi_selfplay PRIVATE
    latrunculi_lib irregular_grid Qt6::Gui Qt6::Svg)

# Instrumented batch self-play: plays N games and prints game-quality metrics. No Qt and
# no per-ply rendering, so it can be run in bulk to compare rule sets and eval weights.
add_executable(latrunculi_bench bench.cpp)
target_link_libraries(latrunculi_bench PRIVATE latrunculi_lib)

# Copyright Ben Paul Wise. All Rights Reserved.
