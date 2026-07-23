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

# CMake's Release defaults already give maximum optimization (-O3 for GCC/Clang;
# /O2 /Ob2 for MSVC). Bump MSVC's inlining from /Ob2 to the more aggressive /Ob3
# in place, rather than appending it via add_compile_options: appending would leave
# both /Ob2 and /Ob3 on the command line, and MSVC warns (D9025) about the override.
# /Gw pairs with /GL above, letting the linker fold/strip unused global data as well
# as unused functions. Applies to every target declared below.
if(MSVC)
    string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    add_compile_options("$<$<CONFIG:Release>:/Gw>")
endif()

# Opt-in: tune for the build machine's CPU. OFF by default because it produces
# binaries that may not run on other CPUs (violates "build and run on both").
option(IRRGO_NATIVE_ARCH "Tune Release for this machine's CPU (non-portable)" OFF)
if(IRRGO_NATIVE_ARCH AND NOT MSVC)
    add_compile_options("$<$<CONFIG:Release>:-march=native>")
endif()

# Convenience default for a standard Windows Qt install, applied once for the whole
# tree. Only used on Windows AND only when the user has not already pointed CMake at
# Qt (via -DQt6_DIR or -DCMAKE_PREFIX_PATH). Linux/macOS resolve Qt through the normal
# find_package search, keeping the build portable (see CLAUDE.md: Windows + Debian).
if(WIN32 AND NOT DEFINED Qt6_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()

# Qt6Gui's CMake package optionally looks for WrapVulkanHeaders to expose a
# Vulkan-based QRhi backend; without the Vulkan SDK installed, that search just
# reports "Could NOT find" and Qt falls back to its other backends (no Vulkan
# rendering is used anywhere in this project). Skip the search outright so it
# stops printing noise at configure time.
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

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
