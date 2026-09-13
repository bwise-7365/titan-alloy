# Copyright Ben Paul Wise. All Rights Reserved.
# hexmodel -- ids, quantities, Board, Roster, Position. Depends on hexcoord only.
# Contract stage: headers only. W2 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexmodel INTERFACE)
target_include_directories(hexmodel INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexmodel INTERFACE hexcoord)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
