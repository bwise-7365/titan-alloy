# Copyright Ben Paul Wise. All Rights Reserved.
# Optional Qt6 instance viewer (network_viewer). This subdirectory is included
# from network/CMakeLists.txt only when VINCPNET_BUILD_GUI is ON; even then it
# quietly no-ops if Qt6 is not found, so the solver/test build never depends on
# Qt. Point CMake at your Qt install with -DCMAKE_PREFIX_PATH=<qt>/msvc2022_64
# (or -DQt6_DIR=...), the same convention the irrgo tree uses.

# Convenience default for a standard Windows Qt install, applied only when the
# user has not already pointed CMake at Qt. Linux resolves Qt through the normal
# find_package search (keeps the build portable: Windows + Debian).
if (WIN32 AND NOT DEFINED Qt6_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()

# Concurrent runs the optimal-plan solve off the UI thread (the gravity-swap
# episode taught that a synchronous multi-second solve freezes the viewer).
find_package(Qt6 QUIET COMPONENTS Widgets Concurrent)
if (NOT Qt6_FOUND)
    message(STATUS
        "vincpnet GUI: Qt6 not found -- skipping network_viewer. "
        "Set CMAKE_PREFIX_PATH or Qt6_DIR to enable it.")
    return()
endif()

set(CMAKE_AUTOMOC ON)

qt_add_executable(network_viewer
    main_gui.cpp
    mainwindow.hpp
    mainwindow.cpp
    flowplanview.hpp
    flowplanview.cpp
    costhistogram.hpp
    costhistogram.cpp
    nodelistwidget.hpp
    nodelistwidget.cpp
)
target_include_directories(network_viewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(network_viewer PRIVATE Qt6::Widgets Qt6::Concurrent vincpnet)

# Windowed app (no console). Comment out to get a console for debug output.
set_target_properties(network_viewer PROPERTIES WIN32_EXECUTABLE TRUE)

# The fleet twin: the same map/histogram/list widgets driven by the
# multi-asset / multi-vehicle fleet model, one asset displayed at a time.
qt_add_executable(fleet_viewer
    fleet_gui.cpp
    fleetmainwindow.hpp
    fleetmainwindow.cpp
    flowplanview.hpp
    flowplanview.cpp
    costhistogram.hpp
    costhistogram.cpp
    nodelistwidget.hpp
    nodelistwidget.cpp
)
target_include_directories(fleet_viewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fleet_viewer PRIVATE Qt6::Widgets Qt6::Concurrent vincpnet)
set_target_properties(fleet_viewer PROPERTIES WIN32_EXECUTABLE TRUE)

message(STATUS "vincpnet GUI: network_viewer + fleet_viewer enabled "
               "(Qt6 ${Qt6_VERSION}).")
# Copyright Ben Paul Wise. All Rights Reserved.
