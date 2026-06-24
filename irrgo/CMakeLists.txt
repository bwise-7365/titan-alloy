# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(IrrGo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release optimization (portable: MSVC on Windows, GCC/Clang on Debian) ──────
# Default to an optimized build for single-config generators (Ninja, Makefiles).
# Multi-config generators (Visual Studio) choose the config at build time, so we
# leave CMAKE_BUILD_TYPE alone there.
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()

# Link-time / whole-program optimization, Release only. CMake emits the correct
# per-compiler flags (/GL + /LTCG for MSVC, -flto for GCC/Clang). Guarded by a
# capability probe so configuration still succeeds if the toolchain lacks it.
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipoSupported OUTPUT _ipoError LANGUAGES CXX)
if(_ipoSupported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
    message(STATUS "IPO/LTO unavailable, skipping: ${_ipoError}")
endif()

# Extra Release flags layered on top of CMake's defaults (/O2 for MSVC, -O3 for
# GCC/Clang). Applies to every target declared below.
add_compile_options(
    "$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2;/Ob3>"
    "$<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O3>"
)

# Opt-in: tune for the build machine's CPU. OFF by default because it produces
# binaries that may not run on other CPUs (violates "build and run on both").
option(IRRGO_NATIVE_ARCH "Tune Release for this machine's CPU (non-portable)" OFF)
if(IRRGO_NATIVE_ARCH AND NOT MSVC)
    add_compile_options("$<$<CONFIG:Release>:-march=native>")
endif()

add_subdirectory(palette_core)
add_subdirectory(palette_widgets)
add_subdirectory(absgame)
add_subdirectory(gui_common)
add_subdirectory(irrgo_game)
add_subdirectory(driver)
add_subdirectory(irrgo_gui)
add_subdirectory(mancala_game)
add_subdirectory(mancala_gui)
add_subdirectory(irregular_grids)
add_subdirectory(latrunculi_game)
add_subdirectory(latrunculi_gui)
# Copyright Ben Paul Wise. All Rights Reserved.
