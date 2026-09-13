# Copyright Ben Paul Wise. All Rights Reserved.
# hexrules -- RuleSet from the rules document, package loading and bindings, the coverage ledger.
# Contract stage: headers only. W2 replaces INTERFACE with STATIC and adds the .cpp files and tests.
add_library(hexrules INTERFACE)
target_include_directories(hexrules INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexrules INTERFACE hexmodel)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
