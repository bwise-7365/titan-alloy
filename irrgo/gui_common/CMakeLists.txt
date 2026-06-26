# Copyright Ben Paul Wise. All Rights Reserved.
# Shared Qt GUI scaffolding for the game windows (irrgo, mancala, latrunculi):
# the NegaMax/MCTS menu builders, the threaded SearchController, and the
# GameMainWindow base. Links Qt6::Widgets and the pure-algorithm abs_game lib.
# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

add_library(gui_common STATIC
    menu_helpers.h      menu_helpers.cpp
    SearchController.h  SearchController.cpp
    GameMainWindow.h    GameMainWindow.cpp
    BoardWidgetBase.h   BoardWidgetBase.cpp
    PlaybackBar.h       PlaybackBar.cpp
    MoveListWidget.h    MoveListWidget.cpp
)
target_include_directories(gui_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(gui_common PUBLIC cxx_std_20)
target_link_libraries(gui_common PUBLIC Qt6::Widgets abs_game)
# Copyright Ben Paul Wise. All Rights Reserved.
