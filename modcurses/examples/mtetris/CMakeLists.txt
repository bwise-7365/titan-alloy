# The game logic is a small library so the tests can reach it: the seed's
# whole purpose is reproducibility, and that deserves an assertion rather
# than a claim.
add_library(mtetris_core STATIC
    shape.cpp
    board.cpp
    scheme.cpp
)
target_include_directories(mtetris_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mtetris_core PUBLIC modcurses::modcurses)

if(MODCURSES_BUILD_EXAMPLES)
    add_executable(mtetris main.cpp)
    target_link_libraries(mtetris PRIVATE mtetris_core)
endif()
