# Copyright Ben Paul Wise. All Rights Reserved.
# hexgames_cli -- the headless front end over hexrecord: validate, replay, record a golden.
add_executable(hexgames_cli HexGamesCli.cpp)
target_link_libraries(hexgames_cli PRIVATE hexrecord trc_game pgg_game)
target_include_directories(hexgames_cli PRIVATE ${CMAKE_SOURCE_DIR}/games)
target_compile_features(hexgames_cli PRIVATE cxx_std_20)
# Copyright Ben Paul Wise. All Rights Reserved.
