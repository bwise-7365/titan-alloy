# Copyright Ben Paul Wise. All Rights Reserved.
# HexMapEd -- the map sheet editor (task 16). The only executable besides hexqt and the *_gui targets
# that links Qt. windeployqt puts the Qt runtime (Core, Gui, Widgets and platforms/) beside the
# executable after every link, so it runs from the build tree.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)
add_executable(HexMapEd WIN32
  Main.cpp
  MainWindow.cpp
  MapView.cpp
  ScenePainter.cpp)
target_link_libraries(HexMapEd PRIVATE hexmaped hexview Qt6::Widgets)
target_compile_features(HexMapEd PRIVATE cxx_std_20)
target_compile_definitions(HexMapEd PRIVATE
  HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  HEXGAMES_PYTHON="${HEXGAMES_PYTHON}")
hexgames_deploy_qt(HexMapEd)
# Copyright Ben Paul Wise. All Rights Reserved.
