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
irrgo_add_version_resource(mancala_gui "Mancala")

# Build as a Windows GUI subsystem app, so launching it from Explorer does not also open
# a console window. Ignored on non-Windows platforms, so this stays portable; Qt supplies
# the WinMain entry point via Qt6::EntryPoint, which Qt6::Core links in automatically.
#
# The cost: qWarning/qDebug and anything written to stderr have nowhere to go when the
# program is started from Explorer. Comment this line out to get the console back while
# debugging. (Launched from an existing terminal, output still appears there.)
set_target_properties(mancala_gui PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(mancala_gui PRIVATE
    Qt6::Widgets
    gui_common
    mancala_lib
)
# Copyright Ben Paul Wise. All Rights Reserved.
