# Copyright Ben Paul Wise. All Rights Reserved.
# hexrecord -- hexsave reading and canonical writing, replay, golden comparison.
# Contract stage: headers only. W3 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexrecord INTERFACE)
target_include_directories(hexrecord INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexrecord INTERFACE hexengine)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
