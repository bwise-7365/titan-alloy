# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(IrrGo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(palette_core)
add_subdirectory(absgame)
add_subdirectory(irrgo_game)
add_subdirectory(driver)
add_subdirectory(irrgo_gui)
add_subdirectory(mancala_game)
add_subdirectory(mancala_gui)
# Copyright Ben Paul Wise. All Rights Reserved.
