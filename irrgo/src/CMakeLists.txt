# Copyright Ben Paul Wise. All Rights Reserved.
add_library(abs_game STATIC AbsGame.cpp negamax.cpp mcts.cpp)
target_include_directories(abs_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(abs_game PUBLIC cxx_std_20)
# Copyright Ben Paul Wise. All Rights Reserved.
