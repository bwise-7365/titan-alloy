# Copyright Ben Paul Wise. All Rights Reserved.
# trc_game -- The Russian Campaign: facts, policies, sequence of play, command grammar, ledger.
add_library(trc_game STATIC
  TrcCalendar.cpp
  TrcFacts.cpp
  TrcFlags.cpp
  TrcUnits.cpp
  TrcBinding.cpp
  TrcZoc.cpp
  TrcWeather.cpp
  TrcMovement.cpp
  TrcMovementChart.cpp
  TrcStacking.cpp
  TrcSupply.cpp
  TrcCombat.cpp
  TrcAir.cpp
  TrcRetreat.cpp
  TrcControl.cpp
  TrcRail.cpp
  TrcVictory.cpp
  TrcMandatory.cpp
  TrcArrivals.cpp
  TrcSpecialMoves.cpp
  TrcPolitics.cpp
  TrcPhaseGate.cpp
  TrcGame.cpp
  TrcGamePhases.cpp
  TrcCommandGrammar.cpp
  TrcPolicySet.cpp
  TrcLedger.cpp)
target_include_directories(trc_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(trc_game PUBLIC hexengine hexrecord)
target_compile_features(trc_game PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../test/CMakeLists.txt")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../test ${CMAKE_CURRENT_BINARY_DIR}/test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
