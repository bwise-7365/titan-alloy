# Copyright Ben Paul Wise. All Rights Reserved.
add_library(irrgo_lib STATIC
    Node.cpp
    Graph.cpp
    RectangularGraph.cpp
    IrregularGraph.cpp
    LoadedGraph.cpp
    Game.cpp
    DVR.cpp
    utils.cpp
    utils.h
)
target_include_directories(irrgo_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(irrgo_lib PUBLIC cxx_std_20)
target_link_libraries(irrgo_lib PUBLIC abs_game)
# Copyright Ben Paul Wise. All Rights Reserved.
