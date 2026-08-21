add_executable(modcurses_tests
    test_main.cpp
    test_geometry.cpp
    test_signal.cpp
    test_utf8.cpp
    test_screenbuffer.cpp
    test_canvas.cpp
    test_mock_terminal.cpp
    test_layout.cpp
    test_widget.cpp
    test_events.cpp
    test_widgets.cpp
    test_catalog.cpp
    test_text_buffer.cpp
    test_text_area.cpp
    test_args.cpp
    test_mtetris.cpp
    test_mamon.cpp
)
# mtetris_core brings modcurses with it, and lets the game rules be tested
# without a terminal.
target_link_libraries(modcurses_tests PRIVATE modcurses::modcurses mtetris_core mamon_core)
target_include_directories(modcurses_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The golden help output lives in the source tree, not the build tree.
target_compile_definitions(modcurses_tests
    PRIVATE MODCURSES_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden")

if(MSVC)
    # doctest is third-party; do not hold it to /W4, and silence its
    # std::tuple specialisation (C5285) which we cannot fix from here.
    target_compile_options(modcurses_tests PRIVATE /W3 /wd5285)
else()
    target_compile_options(modcurses_tests PRIVATE -Wall -Wextra)
endif()

add_test(NAME modcurses_tests COMMAND modcurses_tests)
