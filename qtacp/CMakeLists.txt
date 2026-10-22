# -------------------------------------------------
#   Copyright Ben Paul Wise. All Rights Reserved.
# -------------------------------------------------
# qtacp: port of yracp to Qt6 + the abzar DESim engine.
#
# The prebuilt abzar.lib/desim.lib at the abzar repo root are MSVC
# Debug (/MDd) static libraries, so qtacp links only in a Debug
# configuration. A Release qtacp requires rebuilding abzar Release
# first.
# -------------------------------------------------

cmake_minimum_required(VERSION 3.25)
project(qtacp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
  message(FATAL_ERROR "qtacp supports only the MSVC toolchain")
endif()

# quiet the MSVC deprecation warnings for sprintf and similar
# functions used by the vendored legacy code
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# -------------------------------------------------
# qtacp is in a different repository than abzar and Qt, so their
# locations are cache variables with absolute defaults, not
# relative lookups.

set(QTACP_QT_DIR    "C:/Qt/6.8.3/msvc2022_64"      CACHE PATH "Qt6 prefix (MSVC kit)")
set(QTACP_ABZAR_DIR "C:/repos/ghub-per/panj/abzar" CACHE PATH "abzar repo root: headers under libsrc/, static libs at the root")

list(APPEND CMAKE_PREFIX_PATH "${QTACP_QT_DIR}")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)

set(ABZAR_INCLUDE_DIR "${QTACP_ABZAR_DIR}/libsrc")
set(DESIM_INCLUDE_DIR "${QTACP_ABZAR_DIR}/libsrc/desim")
set(ABZAR_LIBRARIES   "${QTACP_ABZAR_DIR}/abzar.lib")
set(DESIM_LIBRARIES   "${QTACP_ABZAR_DIR}/desim.lib")

# src/aaa must precede the abzar directories so the vendored fsm.h
# shadows abzar's unrelated fsm.h; abzar's splay.h stays visible
# because the vendored tree does not carry one.
include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/aaa
  ${DESIM_INCLUDE_DIR}
  ${ABZAR_INCLUDE_DIR}
)

# -------------------------------------------------
# build the qtacp executable

set(QTACP_SRCS
  main.cpp
  # vendored AAA library
  src/aaa/aaa.cc
  src/aaa/aaa_windows.cc
  src/aaa/mat.cc
  src/aaa/fsm.cc
  # the yracp application sources, mirroring the old YRACP_SRCS list;
  # stages 2-5 of the port uncomment these as they are converted
  # (missile.cc, ktest.cc, and tprim.cc stay unbuilt, as in yracp)
  src/xtdemo.cc
  src/struct.cc
  src/xttimer.cc
  src/xtsim.cc
  src/xtfile.cc
  src/cpanel.cc
  src/tdv.cc
  src/tgrid.cc
  src/acpsim.cc
  src/scen.cc
  src/tthread.cc
  src/orders.cc
  src/defend.cc
  src/unit.cc
  src/cunit.cc
  src/runit.cc
  src/components.cc
  src/mcontrol.cc
  src/formation.cc
  src/mfrac.cc
  src/noppmove.cc
  src/reform.cc
  src/oppmove.cc
  src/lanch.cc
  src/subsa.cc
)

add_executable(qtacp ${QTACP_SRCS})

# desim references abzar symbols, so desim comes first
target_link_libraries(qtacp PRIVATE
  Qt6::Widgets Qt6::Gui Qt6::Core
  ${DESIM_LIBRARIES}
  ${ABZAR_LIBRARIES}
)

# -------------------------------------------------
# show some useful status information

message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
message(STATUS "QTACP_QT_DIR: " ${QTACP_QT_DIR})
message(STATUS "QTACP_ABZAR_DIR: " ${QTACP_ABZAR_DIR})

# -------------------------------------------------
#   Copyright Ben Paul Wise. All Rights Reserved.
# -------------------------------------------------
