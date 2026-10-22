# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. Every suite walks a real Board, so each links hexrules (which compiles
# BoardBuilder) and needs HEXGAMES_SOURCE_DIR to find its documents.
foreach(_suite ScratchTest DijkstraTest AStarTest BfsFloodTest ComponentsTest ReachCountTest
                MonotoneWalkTest RegionFloodTest NetworkGraphTest)
  string(TOLOWER ${_suite} _lower)
  hexgames_add_gtest(hexsearch_${_lower} SOURCES ${_suite}.cpp LIBS hexsearch hexrules LABELS core)
  target_compile_definitions(hexsearch_${_lower} PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_include_directories(hexsearch_${_lower} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
# Copyright Ben Paul Wise. All Rights Reserved.
