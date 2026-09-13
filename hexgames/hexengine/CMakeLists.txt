# Copyright Ben Paul Wise. All Rights Reserved.
# hexengine -- Session, Command, Player, PhaseCursor, policies and defaults, adjudicators, events, PRNG.
add_library(hexengine STATIC
  PrngStreams.cpp
  Event.cpp
  GameNames.cpp
  PhaseCursor.cpp
  DefaultsZoc.cpp
  DefaultsMovement.cpp
  DefaultsSupply.cpp
  DefaultsCombat.cpp
  DefaultsMisc.cpp
  DefaultsGrammar.cpp
  AdjudicatorsDetail.cpp
  AdjudicatorsMove.cpp
  AdjudicatorsCombat.cpp
  Session.cpp
  Player.cpp)
target_include_directories(hexengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexengine PUBLIC hexrules hexsearch)
target_compile_features(hexengine PUBLIC cxx_std_20)
find_package(Threads REQUIRED)
target_link_libraries(hexengine PUBLIC Threads::Threads)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
