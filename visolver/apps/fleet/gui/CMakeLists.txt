# Copyright Ben Paul Wise. All Rights Reserved.
# fleet_app GUI: the fleet planning window (a renamespaced copy of the network
# fleet viewer's window, solving through App::Fleet) built on the SHARED
# viewer_common library. Reuse, not copy: PlannerGui + FlowPlanView +
# CostHistogram + NodeListWidget all come from viewer_common (network/gui),
# which is linked, not duplicated.
#
# Self-disabling, like network/gui: viewer_common only exists when the network
# GUI was built (VIMCPNET_BUILD_GUI on AND Qt6 found). Guard on it so a headless
# / GUI-off configuration simply skips fleet_app.

if (WIN32 AND NOT DEFINED Qt6_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()
find_package(Qt6 QUIET COMPONENTS Widgets Concurrent)

if (Qt6_FOUND AND TARGET viewer_common)
    set(CMAKE_AUTOMOC ON)
    qt_add_executable(fleet_app
        fleet_app.cpp
        fleetappmainwindow.hpp
        fleetappmainwindow.cpp
    )
    # viewer_common carries Qt6::Widgets + vimcpnet PUBLIC; fleetapp carries the
    # App::Fleet class (+ vimcp/vimcpnet). Concurrent is for the worker threads.
    target_link_libraries(fleet_app PRIVATE Qt6::Concurrent viewer_common fleetapp)
    set_target_properties(fleet_app PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    message(STATUS
        "apps/fleet/gui: Qt6 or viewer_common not available; skipping fleet_app.")
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
