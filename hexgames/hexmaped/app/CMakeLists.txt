# Copyright Ben Paul Wise. All Rights Reserved.
# HexMapEd -- the map sheet editor (task 16). The only executable besides hexqt and the *_gui targets
# that links Qt. windeployqt puts the Qt runtime (Core, Gui, Widgets and platforms/) beside the
# executable after every link, so it runs from the build tree.
# Takes whichever major version of Qt is installed, Qt6 preferred (Qt5 on Debian); QT_VERSION_MAJOR
# names the targets below.
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# help.qrc builds the user guide into the executable for Help > About (":/help/README.md").
add_executable(HexMapEd WIN32
  Main.cpp
  MainWindow.cpp
  MapView.cpp
  ScenePainter.cpp
  help.qrc)
# Qt Svg renders a panel's SVG picture (hexsheet.xsd PanelImage, 2026-10-06)
target_link_libraries(HexMapEd PRIVATE hexmaped hexview
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)
target_compile_features(HexMapEd PRIVATE cxx_std_20)
target_compile_definitions(HexMapEd PRIVATE
  HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  HEXGAMES_PYTHON="${HEXGAMES_PYTHON}")
hexgames_deploy_qt(HexMapEd)
# Copyright Ben Paul Wise. All Rights Reserved.
