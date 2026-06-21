# Copyright Ben Paul Wise. All Rights Reserved.
#
# irregular_grids: hand-scratched game-board grid generator. Pure algorithms,
# NO Qt dependency. Buildable standalone or as a subdirectory of the top-level
# IrrGo build.

cmake_minimum_required(VERSION 3.20)
project(irregular_grids CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(irregular_grid STATIC
    irregular_grid.cpp
    irregular_grid.hpp
    board_params.cpp
    board_params.hpp
    draw_params.cpp
    draw_params.hpp
)
target_include_directories(irregular_grid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(irregular_grid PUBLIC cxx_std_20)
# PRNG seed utilities (AbsGame::makeSeed) live in abs_game; the public header
# uses them in RenderConfig's default seed, so the dependency is PUBLIC. This
# requires building as part of the top-level project, where abs_game is defined.
target_link_libraries(irregular_grid PUBLIC abs_game)

add_executable(irregular_grid_demo main.cpp)
target_link_libraries(irregular_grid_demo PRIVATE irregular_grid)
# Copyright Ben Paul Wise. All Rights Reserved.
