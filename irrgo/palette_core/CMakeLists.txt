# Copyright Ben Paul Wise. All Rights Reserved.
#
# palette_core: pure palette-completion algorithms. NO Qt dependency.
# Configurable standalone (build + verify this before the widget) or as a
# subdirectory of the top-level IrrGo build.

cmake_minimum_required(VERSION 3.20)
project(palette_core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(palette_core STATIC
    src/legibility.cpp
    src/minimax.cpp
    src/conversion.cpp
    src/harmony.cpp
    src/modes_internal.cpp
    src/modes_background.cpp
    src/modes_one_piece.cpp
    src/modes_two_pieces.cpp
)
target_include_directories(palette_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(palette_core PUBLIC cxx_std_20)

# Tests use a self-contained header harness (see tests/test_support.hpp) so no
# network/package fetch is required at configure time.
enable_testing()

add_executable(test_legibility tests/test_legibility.cpp)
target_link_libraries(test_legibility PRIVATE palette_core)
target_include_directories(test_legibility PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME legibility COMMAND test_legibility)

add_executable(test_minimax tests/test_minimax.cpp)
target_link_libraries(test_minimax PRIVATE palette_core)
target_include_directories(test_minimax PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME minimax COMMAND test_minimax)

add_executable(test_conversion tests/test_conversion.cpp)
target_link_libraries(test_conversion PRIVATE palette_core)
target_include_directories(test_conversion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME conversion COMMAND test_conversion)

add_executable(test_harmony tests/test_harmony.cpp)
target_link_libraries(test_harmony PRIVATE palette_core)
target_include_directories(test_harmony PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME harmony COMMAND test_harmony)

add_executable(test_modes tests/test_modes.cpp)
target_link_libraries(test_modes PRIVATE palette_core)
target_include_directories(test_modes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME modes COMMAND test_modes)

add_executable(test_roundtrip tests/test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE palette_core)
target_include_directories(test_roundtrip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME roundtrip COMMAND test_roundtrip)
# Copyright Ben Paul Wise. All Rights Reserved.
