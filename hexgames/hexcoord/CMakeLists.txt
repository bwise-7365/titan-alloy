# Copyright Ben Paul Wise. All Rights Reserved.
# hexcoord -- the ABC coordinate algebra, directions, hex addressing and sheet grids. No dependencies.
add_library(hexcoord STATIC
  Abc.cpp
  Direction.cpp
  HexAddress.cpp
  Grid.cpp)
target_include_directories(hexcoord PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(hexcoord PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
