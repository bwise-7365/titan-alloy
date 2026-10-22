cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# vcpkg discovery (Windows only). This MUST run before project(), because that
# is the point at which CMAKE_TOOLCHAIN_FILE takes effect.
#
# Still no machine-specific paths in this file (BUILD_NOTES section 3, rule 1):
# all it does is read the VCPKG_ROOT environment variable, and only when the
# caller has not already supplied a toolchain. A preset or an explicit
# -DCMAKE_TOOLCHAIN_FILE always wins.
#
# This exists so that a bare configure - an IDE's own default profile, say -
# still finds PDCurses instead of failing deep inside find_package.
# ---------------------------------------------------------------------------
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain, auto-detected from VCPKG_ROOT")
    message(STATUS "modcurses: vcpkg toolchain from VCPKG_ROOT -> ${CMAKE_TOOLCHAIN_FILE}")
endif()

project(modcurses VERSION 0.1.0 LANGUAGES CXX)

option(MODCURSES_BUILD_EXAMPLES "Build the example programs" ON)
option(MODCURSES_BUILD_TESTS    "Build the unit tests"       ON)

# ---------------------------------------------------------------------------
# Build configurations
#
# Debug and Release are both first-class. With a multi-config generator (the
# presets use Ninja Multi-Config; Visual Studio and Xcode also qualify) a
# single configure directory produces BOTH, and you pick one per build with
# --config. With a single-config generator, CMAKE_BUILD_TYPE picks one and
# defaults to Debug rather than to the unoptimised-and-unlabelled empty string.
# ---------------------------------------------------------------------------
get_property(MODCURSES_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(MODCURSES_MULTI_CONFIG)
    # Trim the default four down to the two that are actually supported and
    # tested, unless the caller asked for something specific.
    if(NOT MODCURSES_CONFIG_TYPES_SET)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release"
            CACHE STRING "Configurations to generate" FORCE)
        set(MODCURSES_CONFIG_TYPES_SET ON CACHE INTERNAL "")
    endif()
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()

# Per-config output directories. Without these, a multi-config generator
# writes every configuration's binaries to the same path and the last build
# wins - so a Release run could silently be executing the Debug binary.
# $<CONFIG> resolves to the build type under single-config generators too,
# which keeps the layout identical either way.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin")

# ---------------------------------------------------------------------------
# Dependency lookup - the ONLY place in the build allowed to branch on OS.
# ---------------------------------------------------------------------------
if(WIN32)
    find_package(unofficial-pdcurses CONFIG QUIET)
    if(NOT unofficial-pdcurses_FOUND)
        message(FATAL_ERROR
            "PDCurses was not found.\n"
            "On Windows, modcurses gets PDCurses from vcpkg, which requires "
            "vcpkg's toolchain file. Any ONE of these fixes it:\n"
            "  1. Configure with a preset:  cmake --preset windows-msvc\n"
            "     (in an IDE, select a preset profile rather than the IDE's own "
            "default CMake profile - the default passes no toolchain file, "
            "which is the usual cause of this message)\n"
            "  2. Set the VCPKG_ROOT environment variable to your vcpkg "
            "checkout, then re-configure from scratch\n"
            "  3. Pass -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/"
            "vcpkg.cmake\n"
            "CMAKE_TOOLCHAIN_FILE is currently: '${CMAKE_TOOLCHAIN_FILE}'")
    endif()
    set(MODCURSES_CURSES_LIB unofficial::pdcurses::pdcurses)
else()
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(NCURSESW IMPORTED_TARGET ncursesw)
    endif()
    if(TARGET PkgConfig::NCURSESW)
        set(MODCURSES_CURSES_LIB PkgConfig::NCURSESW)
    else()
        # Fallback for systems without the ncursesw pkg-config file.
        find_library(MODCURSES_NCURSESW_LIB NAMES ncursesw)
        find_path(MODCURSES_NCURSESW_INCLUDE NAMES ncurses.h)
        if(NOT MODCURSES_NCURSESW_LIB OR NOT MODCURSES_NCURSESW_INCLUDE)
            message(FATAL_ERROR
                "The wide-character ncurses library (ncursesw) was not found.\n"
                "On Debian and Ubuntu:  sudo apt install libncurses-dev "
                "pkg-config\n"
                "Then check that 'pkg-config --libs ncursesw' prints "
                "-lncursesw.\n"
                "The narrow ncurses library will not do: modcurses needs the "
                "wide API (wget_wch, setcchar, wadd_wchnstr).")
        endif()
        add_library(modcurses_ncursesw INTERFACE)
        target_link_libraries(modcurses_ncursesw INTERFACE ${MODCURSES_NCURSESW_LIB})
        target_include_directories(modcurses_ncursesw
            INTERFACE ${MODCURSES_NCURSESW_INCLUDE})
        set(MODCURSES_CURSES_LIB modcurses_ncursesw)
    endif()
endif()

# ---------------------------------------------------------------------------
# The library
# ---------------------------------------------------------------------------
add_library(modcurses STATIC
    src/core.cpp
    src/utf8.cpp
    src/render.cpp
    src/widget.cpp
    src/widgets.cpp
    src/text_input.cpp
    src/list_view.cpp
    src/grid_canvas.cpp
    src/text_buffer.cpp
    src/text_area.cpp
    src/args.cpp
    src/app.cpp
    src/backend/mock_terminal.cpp
    src/backend/curses_terminal.cpp
)
add_library(modcurses::modcurses ALIAS modcurses)

target_include_directories(modcurses
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(modcurses PUBLIC cxx_std_20)

# PRIVATE is the curses firewall: consumers of modcurses never see a curses
# header, a curses macro, or a curses link dependency in their own compiles.
target_link_libraries(modcurses PRIVATE ${MODCURSES_CURSES_LIB})

# Flags branch on COMPILER, never on OS.
if(MSVC)
    # PUBLIC and mandatory: the sources contain UTF-8 literals (box drawing).
    # Without /utf-8, MSVC parses them in the system codepage and silently
    # corrupts every one of them (warning C4066).
    target_compile_options(modcurses PUBLIC /utf-8)
    # Note: never pass -Wall to MSVC; it means /Wall (thousands of lines).
    target_compile_options(modcurses PRIVATE /W4 /permissive-)
else()
    target_compile_options(modcurses PRIVATE -Wall -Wextra)
endif()

if(MODCURSES_BUILD_EXAMPLES)
    add_subdirectory(examples/scratch)
    add_subdirectory(examples/hello)
    add_subdirectory(examples/kitchen)
endif()

# mtetris is always configured: the tests link its game logic, so its rules
# are covered even when the example programs are switched off.
add_subdirectory(examples/mtetris)
add_subdirectory(examples/mamon)

if(MODCURSES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
