# Copyright Ben Paul Wise. All Rights Reserved.
# pform_gui: the Qt viewer for the PFORM parliament-formation model, adapted
# from the old C# ProbPForm window but computing ONLY the pform math (setup,
# solve, and coalition post-processing all come from pformapp, exactly as
# pform_cli). Built only when Qt6 is available; deliberately does NOT link
# viewer_common (that library is map-centric, nothing here draws a map).

if (WIN32 AND NOT DEFINED Qt6_DIR AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(Qt6_DIR "C:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()
find_package(Qt6 QUIET COMPONENTS Widgets Concurrent)

if (Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    qt_add_executable(pform_gui
        pform_gui.cpp
        pformmainwindow.hpp pformmainwindow.cpp
        pformpartytable.hpp pformpartytable.cpp)
    target_link_libraries(pform_gui PRIVATE Qt6::Widgets Qt6::Concurrent pformapp)
    set_target_properties(pform_gui PROPERTIES WIN32_EXECUTABLE TRUE)
    if (MSVC)
        target_compile_options(pform_gui PRIVATE /W4)
    else()
        target_compile_options(pform_gui PRIVATE -Wall -Wextra)
    endif()
else()
    message(STATUS "apps/pform/gui: Qt6 not available; skipping pform_gui.")
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
