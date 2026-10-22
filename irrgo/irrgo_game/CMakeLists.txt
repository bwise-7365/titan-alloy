# Copyright Ben Paul Wise. All Rights Reserved.
add_library(irrgo_lib STATIC
    Node.cpp
    Graph.cpp
    RectangularGraph.cpp
    IrregularGraph.cpp
    LoadedGraph.cpp
    Game.cpp
    DVR.cpp
    EyeEval.cpp
    GameXml.cpp
)
target_include_directories(irrgo_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(irrgo_lib PUBLIC cxx_std_20)
target_link_libraries(irrgo_lib PUBLIC abs_game)

# Batch self-play under MCTS, optionally writing each game as XML. No Qt, so it can be
# run in bulk and can carry the MB2 memory tracker (see absgame/MemTrack.h).
add_executable(irrgo_bench bench.cpp)
target_link_libraries(irrgo_bench PRIVATE irrgo_lib mem_track)
# Copyright Ben Paul Wise. All Rights Reserved.
