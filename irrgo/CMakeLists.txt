# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(IrrGo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(absgame)
add_subdirectory(src)
add_subdirectory(driver)
add_subdirectory(gui)
# Copyright Ben Paul Wise. All Rights Reserved.
