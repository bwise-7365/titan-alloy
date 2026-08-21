# As with mtetris, the logic that can be tested without a terminal lives in a
# library so that it actually is.
add_library(mamon_core STATIC
    editor.cpp
    ui.cpp
)
target_include_directories(mamon_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mamon_core PUBLIC modcurses::modcurses)

if(MODCURSES_BUILD_EXAMPLES)
    add_executable(mamon main.cpp)
    target_link_libraries(mamon PRIVATE mamon_core)
endif()
