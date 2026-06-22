# Copyright Ben Paul Wise. All Rights Reserved.
# Latrunculi ("Ludus Latrunculorum") Qt GUI. Draws the board/pieces via the
# irregular_grids SVG library rendered through QSvgRenderer (hence Qt6::Svg), and
# reuses the shared gui_common scaffolding (menus + threaded search).
# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...

set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(latrunculi_gui
    main_gui.cpp
    MainWindow.h
    MainWindow.cpp
    MainWindow_save.cpp
    BoardWidget.h
    BoardWidget.cpp
    latrunculi.qrc
)

# Comment this out to get a console window for the qWarning/stderr output.
#set_target_properties(latrunculi_gui PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(latrunculi_gui PRIVATE
    Qt6::Widgets
    Qt6::Svg
    gui_common
    latrunculi_lib
    irregular_grid
)
# Copyright Ben Paul Wise. All Rights Reserved.
