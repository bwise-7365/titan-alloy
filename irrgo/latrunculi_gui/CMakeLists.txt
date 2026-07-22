# Copyright Ben Paul Wise. All Rights Reserved.
# Latrunculi ("Ludus Latrunculorum") Qt GUI. Draws the board/pieces via the
# irregular_grids SVG library rendered through QSvgRenderer (hence Qt6::Svg), and
# reuses the shared gui_common scaffolding (menus + threaded search).
# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...

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
    DisplayConstants.h
    latrunculi.qrc
)

# Windows: build a multi-resolution latrunculi.ico from the per-size icons and embed
# it as the executable's icon (shown by File Explorer, the desktop and the taskbar).
# tools/merge-ico.ps1 packs the sizes (no external tools); configure_file bakes the
# merged icon's absolute path into the generated .rc so rc.exe resolves it regardless
# of its working directory; with MSVC, CMake compiles and links the .rc automatically.
if(WIN32)
    set(_lat_icon_srcs
        "${CMAKE_SOURCE_DIR}/images/latrunculi16.ico"
        "${CMAKE_SOURCE_DIR}/images/latrunculi32.ico"
        "${CMAKE_SOURCE_DIR}/images/latrunculi48.ico"
        "${CMAKE_SOURCE_DIR}/images/latrunculi256.ico")
    set(_lat_icon "${CMAKE_CURRENT_BINARY_DIR}/latrunculi.ico")
    # Pass the inputs as one comma-joined argument: powershell -File binds a single
    # token to the script's -In string, which the script splits on commas.
    string(REPLACE ";" "," _lat_icon_srcs_csv "${_lat_icon_srcs}")
    add_custom_command(
        OUTPUT  "${_lat_icon}"
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_SOURCE_DIR}/tools/merge-ico.ps1"
                -Out "${_lat_icon}" -In "${_lat_icon_srcs_csv}"
        DEPENDS ${_lat_icon_srcs} "${CMAKE_SOURCE_DIR}/tools/merge-ico.ps1"
        COMMENT "Merging per-size icons into latrunculi.ico"
        VERBATIM)
    # Build the icon before the GUI target so the merged .ico exists when rc.exe runs.
    add_custom_target(latrunculi_icon DEPENDS "${_lat_icon}")
    add_dependencies(latrunculi_gui latrunculi_icon)

    set(LATRUNCULI_ICON_PATH "${_lat_icon}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc" @ONLY)
    target_sources(latrunculi_gui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
endif()

# Build as a Windows GUI subsystem app, so launching it from Explorer does not also open
# a console window. Ignored on non-Windows platforms, so this stays portable; Qt supplies
# the WinMain entry point via Qt6::EntryPoint, which Qt6::Core links in automatically.
#
# The cost: qWarning/qDebug and anything written to stderr have nowhere to go when the
# program is started from Explorer. Comment this line out to get the console back while
# debugging. (Launched from an existing terminal, output still appears there.)
set_target_properties(latrunculi_gui PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(latrunculi_gui PRIVATE
    Qt6::Widgets
    Qt6::Svg
    gui_common
    latrunculi_lib
    irregular_grid
)
# Copyright Ben Paul Wise. All Rights Reserved.
