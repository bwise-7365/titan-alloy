# Copyright Ben Paul Wise. All Rights Reserved.
#
# palette_widgets: thin Qt layer over palette_core (DESIGN.md §6). Built as a
# STATIC library plus a small palette_gui executable, mirroring irrgo_lib/irrgo_gui.
# Configured as part of the top-level IrrGo tree (depends on the palette_core target).

# If Qt6 is not found automatically, point CMake at your installation:
#   cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64 ...
# or set it in CLion: Settings > Build > CMake > CMake options.
set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

add_library(palette_widgets STATIC
    ColorConvert.h
    BoardPreview.h
    BoardPreview.cpp
    RybColorWheel.h
    RybColorWheel.cpp
    PaletteController.h
    PaletteController.cpp
    MainWindow.h
    MainWindow.cpp
)
target_include_directories(palette_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(palette_widgets PUBLIC cxx_std_20)
target_link_libraries(palette_widgets PUBLIC
    palette_core
    Qt6::Widgets
)

qt_add_executable(palette_gui ../palette_gui/main_gui.cpp)
target_link_libraries(palette_gui PRIVATE palette_widgets)

# Uncomment to suppress the console window on Windows (hides stdout/stderr):
#if(WIN32)
#    set_target_properties(palette_gui PROPERTIES WIN32_EXECUTABLE TRUE)
#endif()
# Copyright Ben Paul Wise. All Rights Reserved.
