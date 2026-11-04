# Copyright Ben Paul Wise. All Rights Reserved.
# hexai -- game-neutral tactics and the loop that plays a game with one agent per side.
add_library(hexai STATIC
  Tactics.cpp
  Seize.cpp
  Fingerprint.cpp
  Agent.cpp)
target_include_directories(hexai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexai PUBLIC hexengine hexsearch)
target_compile_features(hexai PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
