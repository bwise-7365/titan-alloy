# Copyright Ben Paul Wise. All Rights Reserved.
# The TRC module's tests. Every suite loads the real TRC package, so each needs HEXGAMES_SOURCE_DIR
# and this directory on its include path for TrcTestFixture.h.
function(trc_add_test target source labels)
  hexgames_add_gtest(${target} SOURCES ${source} LIBS trc_game LABELS "${labels}")
  target_compile_definitions(${target} PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

trc_add_test(trc_package_test TrcPackageTest.cpp "package;trc")
trc_add_test(trc_ledger_test TrcLedgerTest.cpp "trc")
trc_add_test(trc_rules_test TrcRulesTest.cpp "trc")
trc_add_test(trc_combat_test TrcCombatTest.cpp "trc")
trc_add_test(trc_sequence_test TrcSequenceTest.cpp "trc")
trc_add_test(trc_golden_test TrcGoldenTest.cpp "golden;trc")
trc_add_test(trc_golden_long_test TrcGoldenLongTest.cpp "golden;trc;long")
# Copyright Ben Paul Wise. All Rights Reserved.
