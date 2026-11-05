# Copyright Ben Paul Wise. All Rights Reserved.
# pgg_trc_viewer -- steps through a game of PGG or TRC in one window, on HexMapEd's map view. ViewerGame is the
# Qt-free part: a hexsave script or a run-file line into a replay.
find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg Concurrent)
set(CMAKE_AUTOMOC ON)

add_library(hexqt_viewer_game STATIC ViewerGame.cpp)
target_include_directories(hexqt_viewer_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/games)
target_link_libraries(hexqt_viewer_game PUBLIC hexview_play hexmaped hexai pgg_agent trc_game hexrecord)
target_compile_features(hexqt_viewer_game PUBLIC cxx_std_20)

add_executable(pgg_trc_viewer WIN32 ViewerMain.cpp ViewerWindow.cpp)
target_link_libraries(pgg_trc_viewer PRIVATE hexqt_viewer_game hexqt_mapview
  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent)
target_compile_definitions(pgg_trc_viewer PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_features(pgg_trc_viewer PRIVATE cxx_std_20)
hexgames_deploy_qt(pgg_trc_viewer)

hexgames_add_gtest(hexqt_viewer_game_test SOURCES test/ViewerGameTest.cpp LIBS hexqt_viewer_game LABELS core pgg)
target_compile_definitions(hexqt_viewer_game_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
hexgames_add_gtest(hexqt_viewer_window_test SOURCES test/ViewerWindowTest.cpp ViewerWindow.cpp
  LIBS hexqt_viewer_game hexqt_mapview Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
  LABELS pgg gui long GUI)
target_compile_definitions(hexqt_viewer_window_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(hexqt_viewer_window_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(hexqt_viewer_window_test pgg_trc_viewer)  # both deploy Qt into this directory, one at a time
# Copyright Ben Paul Wise. All Rights Reserved.
