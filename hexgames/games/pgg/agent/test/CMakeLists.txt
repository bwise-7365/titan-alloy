# Copyright Ben Paul Wise. All Rights Reserved.
# The agents, on small positions and over whole games from the 1941 scenario.
function(pgg_agent_add_test target source labels)
  hexgames_add_gtest(${target} SOURCES ${source} LIBS pgg_agent LABELS "${labels}")
  target_compile_definitions(${target} PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/games/pgg/test)
endfunction()

pgg_agent_add_test(pgg_agent_test PggAgentTest.cpp "pgg")
pgg_agent_add_test(pgg_game_test PggGameTest.cpp "pgg;long")
# Copyright Ben Paul Wise. All Rights Reserved.
