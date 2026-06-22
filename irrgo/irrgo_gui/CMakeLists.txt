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
    MainWindow_save.cpp
    BoardWidget.h
    BoardWidget.cpp
)

# App icon
qt_add_resources(irrgo_gui "icons_res"
    PREFIX "/icons"
    BASE "../images"
    FILES ../images/irrgo-logo-gold-v2.png)

# Stone textures
qt_add_resources(irrgo_gui "stones_res"
    PREFIX "/stones"
    BASE "../images"
    FILES
        ../images/black01.png ../images/black02.png ../images/black03.png
        ../images/black04.png ../images/black05.png ../images/black06.png
        ../images/black07.png
        ../images/white01.png ../images/white02.png ../images/white03.png
        ../images/white04.png ../images/white05.png ../images/white06.png)

# Background texture
qt_add_resources(irrgo_gui "textures_res"
    PREFIX "/textures"
    BASE "../images"
    FILES
        ../images/texture_bg.png)

if(WIN32)
    target_sources(irrgo_gui PRIVATE irrgo.rc)
endif()


# Comment this out to get a CMD window and view output
#set_target_properties(irrgo_gui PROPERTIES  WIN32_EXECUTABLE TRUE )

target_link_libraries(irrgo_gui PRIVATE
    Qt6::Widgets
    gui_common
    irrgo_lib
)
# Copyright Ben Paul Wise. All Rights Reserved.
