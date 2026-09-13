# Copyright Ben Paul Wise. All Rights Reserved.
# hexcoord -- the ABC coordinate algebra, directions, hex addressing and sheet grids. No dependencies.
# Contract stage: headers only. W1 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexcoord INTERFACE)
target_include_directories(hexcoord INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(hexcoord INTERFACE cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
