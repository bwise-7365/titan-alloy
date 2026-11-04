# Copyright Ben Paul Wise. All Rights Reserved.
# pgg_agent -- scripted players of PGG on the hexai tactics; pgg_play plays games with them.
add_library(pgg_agent STATIC
  PggSeen.cpp
  PggAgent.cpp
  PggAgentMove.cpp
  PggAgentCombat.cpp
  PggAgentSeize.cpp
  PggGame.cpp)
target_include_directories(pgg_agent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pgg_agent PUBLIC pgg_game hexai)
target_compile_features(pgg_agent PUBLIC cxx_std_20)

add_executable(pgg_play PggPlay.cpp)
target_link_libraries(pgg_play PRIVATE pgg_agent)
target_compile_definitions(pgg_play PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_features(pgg_play PRIVATE cxx_std_20)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
