# Copyright Ben Paul Wise. All Rights Reserved.
# The hexxml document models (HexRules cpp/hexxml) on the hexgames game documents: the sheets,
# counters, rules, records and packages of TRC, PGG, DS and DDaT. HexRules tests the same models on
# its own corpus; these tests stay here because the documents do.
foreach(_suite CounterSetDoc PackageDoc RulesDoc SaveDoc SheetDoc)
  string(TOLOWER "${_suite}" _lower)
  hexgames_add_gtest(hexxml_games_${_lower}_test SOURCES ${_suite}GamesTest.cpp LIBS hexxml LABELS core)
  target_compile_definitions(hexxml_games_${_lower}_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endforeach()
# Copyright Ben Paul Wise. All Rights Reserved.
