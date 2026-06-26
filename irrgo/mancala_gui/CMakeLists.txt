# Copyright Ben Paul Wise. All Rights Reserved.
# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

qt_add_executable(mancala_gui
    main_gui.cpp
    MainWindow.h
    MainWindow.cpp
    MancalaWidget.h
    MancalaWidget.cpp
)

# Comment this out to get a CMD window and view output
#set_target_properties(mancala_gui PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(mancala_gui PRIVATE
    Qt6::Widgets
    gui_common
    mancala_lib
)
# Copyright Ben Paul Wise. All Rights Reserved.
