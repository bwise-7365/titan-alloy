cmake_minimum_required(VERSION 3.8)
project(flowplan)

set(CMAKE_CXX_STANDARD 17)

add_executable(flowplan

        src/main.cpp

        lib/flowplanner.h
        lib/flowplanner.cpp
        lib/utils.cpp
)

target_include_directories(flowplan PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
