# Copyright Ben Paul Wise. All Rights Reserved.
hexgames_add_gtest(hexmodel_ids_test SOURCES IdsTest.cpp LIBS hexmodel LABELS core)
hexgames_add_gtest(hexmodel_quantities_test SOURCES QuantitiesTest.cpp LIBS hexmodel LABELS core)

# Board/Roster/Position exercise the real builders, which need HexRules::RuleSet; hexrules already
# links hexmodel, so linking it here pulls in hexmodel, hexxml and hexcoord too.
hexgames_add_gtest(hexmodel_board_test SOURCES BoardTest.cpp LIBS hexrules LABELS core)
target_compile_definitions(hexmodel_board_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexmodel_roster_test SOURCES RosterTest.cpp LIBS hexrules LABELS core)
target_compile_definitions(hexmodel_roster_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexmodel_position_test SOURCES PositionTest.cpp LIBS hexrules LABELS core)
target_compile_definitions(hexmodel_position_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
# Copyright Ben Paul Wise. All Rights Reserved.
