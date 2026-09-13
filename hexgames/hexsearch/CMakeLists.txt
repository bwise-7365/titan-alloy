# Copyright Ben Paul Wise. All Rights Reserved.
# hexsearch -- scratch, Field, Dijkstra/A*/flood/components/monotone walk over a const Board.
# The algorithms are templates over a SearchGraph and so live in Search.h; Search.cpp holds the
# scratch's bookkeeping, the two lattice walks and the two Board adaptors.
add_library(hexsearch STATIC
  Search.cpp)
target_include_directories(hexsearch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexsearch PUBLIC hexmodel)
target_compile_features(hexsearch PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
