# Copyright Ben Paul Wise. All Rights Reserved.
# hexsearch -- scratch, Field, Dijkstra/A*/flood/components/monotone walk over a const Board.
# Contract stage: headers only. W1 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexsearch INTERFACE)
target_include_directories(hexsearch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexsearch INTERFACE hexmodel)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
