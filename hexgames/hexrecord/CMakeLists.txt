# Copyright Ben Paul Wise. All Rights Reserved.
# hexrecord -- hexsave reading and canonical writing, replay, golden comparison.
# SaveModel::read needs hexxml (target `hexxml`), which lands in its own module once W2 gives it a
# CMakeLists.txt and .cpp; until then ReadSaveModel.cpp is left out and SaveModel::read stays declared
# but undefined, which is harmless as long as nothing calls it (the read-side tests are gated the
# same way, just below).
add_library(hexrecord STATIC
  SaveModel.cpp
  GoldenCompare.cpp
  Record.cpp)
target_include_directories(hexrecord PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexrecord PUBLIC hexengine)
# hexxml is a real target as of 2026-09-13, so the read side is always compiled in.
if(TARGET hexxml)
  target_sources(hexrecord PRIVATE ReadSaveModel.cpp)
  target_link_libraries(hexrecord PUBLIC hexxml)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
