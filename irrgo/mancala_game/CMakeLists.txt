# Copyright Ben Paul Wise. All Rights Reserved.
add_library(mancala_lib STATIC Game.cpp)
target_include_directories(mancala_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mancala_lib PUBLIC cxx_std_20)
target_link_libraries(mancala_lib PUBLIC abs_game)
# Copyright Ben Paul Wise. All Rights Reserved.
