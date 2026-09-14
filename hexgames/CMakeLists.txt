# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(HexGames LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators default to Release; multi-config ones pick at build time (as irrgo).
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _isMultiConfig AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT _ipoSupported OUTPUT _ipoError LANGUAGES CXX)
if(_ipoSupported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
  message(STATUS "IPO/LTO unavailable, skipping: ${_ipoError}")
endif()

# Warnings: every target declared below inherits these.
if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
  string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  add_compile_options("$<$<CONFIG:Release>:/Gw>")
else()
  add_compile_options(-Wall -Wextra)
endif()

option(HEXGAMES_BUILD_GUI "Build the Qt6 GUI targets" ON)
option(HEXGAMES_SYSTEM_TINYXML2 "Use an installed TinyXML2 instead of fetching it" OFF)

# Qt6 convenience default for a standard Windows install, only when the user has not pointed CMake at Qt.
if(HEXGAMES_BUILD_GUI AND WIN32 AND NOT DEFINED Qt6_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
  set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

# ---- third-party: GoogleTest and TinyXML2, both pinned, both built by this toolchain ----------------
include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.16.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)   # MSVC /MD; without it the link fails
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)
enable_testing()

if(HEXGAMES_SYSTEM_TINYXML2)
  find_package(tinyxml2 REQUIRED)
else()
  FetchContent_Declare(tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG        10.0.0)
  set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(tinyxml2_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(tinyxml2)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(HexGamesGtest)
include(HexGamesQtDeploy)

# ---- hygiene and schema checks (Python; skipped with a message if Python or lxml is absent) --------
# HEXGAMES_PYTHON names the interpreter that has lxml (machine-specific, so it lives in the presets, as
# Qt6_DIR does); otherwise whatever find_package turns up is tried.
set(HEXGAMES_PYTHON "" CACHE FILEPATH "Python interpreter with lxml for the xsd ctest entries")
if(HEXGAMES_PYTHON AND EXISTS "${HEXGAMES_PYTHON}")
  set(_py "${HEXGAMES_PYTHON}")
else()
  find_package(Python3 COMPONENTS Interpreter QUIET)
  set(_py "${Python3_EXECUTABLE}")
endif()
if(_py)
  add_test(NAME hygiene_banner_check
           COMMAND "${_py}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/banner-check.py" "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(hygiene_banner_check PROPERTIES LABELS "hygiene")
  execute_process(COMMAND "${_py}" -c "import lxml" RESULT_VARIABLE _lxmlMissing OUTPUT_QUIET ERROR_QUIET)
  if(_lxmlMissing)
    message(STATUS "lxml not importable by ${_py}: xsd ctest entries are skipped (set HEXGAMES_PYTHON)")
  else()
    foreach(_lang game_rules map_graphics unit_graphics game_records packages)
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}/xml")
        add_test(NAME xsd_validate_${_lang}
                 COMMAND "${_py}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate-xml.py"
                         "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}/xml")
        set_tests_properties(xsd_validate_${_lang} PROPERTIES LABELS "xsd")
      endif()
    endforeach()
    add_test(NAME hygiene_map_networks
             COMMAND "${_py}" "${CMAKE_CURRENT_SOURCE_DIR}/map_graphics/xml/tools/network_check.py" --quiet
                     "${CMAKE_CURRENT_SOURCE_DIR}/map_graphics/xml/the-russian-campaign.xml"
                     "${CMAKE_CURRENT_SOURCE_DIR}/map_graphics/xml/panzergruppe-guderian.xml")
    set_tests_properties(hygiene_map_networks PROPERTIES LABELS "hygiene")
  endif()
else()
  message(STATUS "Python3 not found: hygiene and xsd ctest entries are skipped")
endif()

# ---- modules, in dependency order; each is added as it comes into existence ------------------------
foreach(_mod hexcoord hexxml hexmodel hexrules hexsearch hexengine hexrecord)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_mod}/CMakeLists.txt")
    add_subdirectory(${_mod})
  endif()
endforeach()
foreach(_game trc pgg ds ddat)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/games/${_game}/engine/CMakeLists.txt")
    add_subdirectory(games/${_game}/engine)
  endif()
endforeach()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hexview/CMakeLists.txt")
  add_subdirectory(hexview)
endif()
if(HEXGAMES_BUILD_GUI)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hexqt/CMakeLists.txt")
    add_subdirectory(hexqt)
  endif()
  foreach(_game trc pgg ds ddat)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/games/${_game}/gui/CMakeLists.txt")
      add_subdirectory(games/${_game}/gui)
    endif()
  endforeach()
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/CMakeLists.txt")
  add_subdirectory(tools)
endif()
add_subdirectory(tests)
# Copyright Ben Paul Wise. All Rights Reserved.
