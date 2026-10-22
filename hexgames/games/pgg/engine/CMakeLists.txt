# Copyright Ben Paul Wise. All Rights Reserved.
# pgg_game -- Panzergruppe Guderian: facts, policies, the battle obligation, sequence of play,
# command grammar, ledger.
add_library(pgg_game STATIC
  PggBinding.cpp
  PggFacts.cpp
  PggFactsMap.cpp
  PggSchedule.cpp
  PggState.cpp
  PggStateCodec.cpp
  PggUnits.cpp
  PggZoc.cpp
  PggTerrain.cpp
  PggMovement.cpp
  PggSupply.cpp
  PggStacking.cpp
  PggCrt.cpp
  PggCombat.cpp
  PggRetreat.cpp
  PggBattle.cpp
  PggBattleCodec.cpp
  PggBattleFlow.cpp
  PggBattleWalk.cpp
  PggBattleAdvance.cpp
  PggBattleAnswer.cpp
  PggObligations.cpp
  PggOverrun.cpp
  PggInterdiction.cpp
  PggArrivals.cpp
  PggFirstTurn.cpp
  PggVictory.cpp
  PggPhaseGate.cpp
  PggCommandGrammar.cpp
  PggStepsCommand.cpp
  PggStepsPhase.cpp
  PggStepsVerbs.cpp
  PggPolicySet.cpp
  PggLedger.cpp)
target_include_directories(pgg_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pgg_game PUBLIC hexengine hexrecord)
target_compile_features(pgg_game PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../test/CMakeLists.txt")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../test ${CMAKE_CURRENT_BINARY_DIR}/test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
