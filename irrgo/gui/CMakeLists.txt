# Copyright Ben Paul Wise. All Rights Reserved.
# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...
# or set it in CLion: Settings > Build > CMake > CMake options.

#set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")
set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

qt_add_executable(irrgo_gui
    main_gui.cpp
    MainWindow.h
    MainWindow.cpp
    BoardWidget.h
    BoardWidget.cpp
    irrgo.qrc
)

if(WIN32)
    target_sources(irrgo_gui PRIVATE irrgo.rc)
endif()

set_target_properties(irrgo_gui PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(irrgo_gui PRIVATE
    Qt6::Widgets
    irrgo_lib
)
# Copyright Ben Paul Wise. All Rights Reserved.
