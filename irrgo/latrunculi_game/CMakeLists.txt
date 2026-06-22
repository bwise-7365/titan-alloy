# Copyright Ben Paul Wise. All Rights Reserved.

add_library(latrunculi_lib STATIC Game.cpp)
target_include_directories(latrunculi_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(latrunculi_lib PUBLIC cxx_std_20)
target_link_libraries(latrunculi_lib PUBLIC abs_game)

# Copyright Ben Paul Wise. All Rights Reserved.
