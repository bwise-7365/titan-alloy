# Copyright Ben Paul Wise. All Rights Reserved.
# HexMapEd -- the map sheet editor (task 16). The only executable besides hexqt and the *_gui targets
# that links Qt. windeployqt puts the Qt runtime (Core, Gui, Widgets and platforms/) beside the
# executable after every link, so it runs from the build tree.
# Takes whichever major version of Qt is installed, Qt6 preferred (Qt5 on Debian); QT_VERSION_MAJOR
# names the targets below.
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg Concurrent)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# help.qrc builds the user guide into the executable for Help > About (":/help/README.md").
# hexqt_mapview -- the map view and the painter, shared with pgg_trc_viewer.
add_library(hexqt_mapview STATIC
  MapView.cpp
  ScenePainter.cpp)
target_include_directories(hexqt_mapview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hexqt_mapview PUBLIC hexmaped hexview
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)
target_compile_features(hexqt_mapview PUBLIC cxx_std_20)

add_executable(HexMapEd WIN32
  Main.cpp
  MainWindow.cpp
  GenerateDock.cpp
  help.qrc)
# Qt Svg renders a panel's SVG picture (hexsheet.xsd PanelImage, 2026-10-06); Qt Concurrent runs the
# map generator (hexgen) off the GUI thread.
target_link_libraries(HexMapEd PRIVATE hexqt_mapview hexmaped hexview hexgen
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Concurrent)
target_compile_features(HexMapEd PRIVATE cxx_std_20)
target_compile_definitions(HexMapEd PRIVATE
  HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  HEXGAMES_PYTHON="${HEXGAMES_PYTHON}")
hexgames_deploy_qt(HexMapEd)

# The Generate dock, offscreen: it generates a map and hands over the sheet.
hexgames_add_gtest(hexmaped_generate_dock_test
  SOURCES GenerateDockTest.cpp GenerateDock.cpp
  LIBS hexgen Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
  LABELS hexgen gui
  GUI)
# Copyright Ben Paul Wise. All Rights Reserved.
