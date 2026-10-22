# Copyright Ben Paul Wise. All Rights Reserved.
# hexmodel -- ids, quantities, Board, Roster, Position. Depends on hexcoord and hexxml (the builders
# read hexxml document models).
add_library(hexmodel STATIC
  Quantities.cpp
  Board.cpp
  Roster.cpp
  Position.cpp)
target_include_directories(hexmodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexmodel PUBLIC hexcoord hexxml)
target_compile_features(hexmodel PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
