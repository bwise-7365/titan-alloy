# Copyright Ben Paul Wise. All Rights Reserved.
# hexengine -- Session, Command, Player, PhaseCursor, policies and defaults, adjudicators, events, PRNG.
# Contract stage: headers only. W1 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexengine INTERFACE)
target_include_directories(hexengine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexengine INTERFACE hexrules hexsearch)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
