# Copyright Ben Paul Wise. All Rights Reserved.
# The PGG module's tests. Every suite loads the real PGG package, so each needs HEXGAMES_SOURCE_DIR
# and this directory on its include path for PggTestFixture.h.
function(pgg_add_test target source labels)
  hexgames_add_gtest(${target} SOURCES ${source} LIBS pgg_game LABELS "${labels}")
  target_compile_definitions(${target} PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

pgg_add_test(pgg_package_test PggPackageTest.cpp "package;pgg")
pgg_add_test(pgg_ledger_test PggLedgerTest.cpp "pgg")
pgg_add_test(pgg_rules_test PggRulesTest.cpp "pgg")
pgg_add_test(pgg_combat_test PggCombatTest.cpp "pgg")
pgg_add_test(pgg_sequence_test PggSequenceTest.cpp "pgg")
pgg_add_test(pgg_state_test PggStateTest.cpp "pgg")
pgg_add_test(pgg_golden_test PggGoldenTest.cpp "golden;pgg")
pgg_add_test(pgg_golden_long_test PggGoldenLongTest.cpp "golden;pgg;long")
# Copyright Ben Paul Wise. All Rights Reserved.
