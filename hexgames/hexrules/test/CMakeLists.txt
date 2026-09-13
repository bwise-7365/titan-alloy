# Copyright Ben Paul Wise. All Rights Reserved.
hexgames_add_gtest(hexrules_ruleset_test SOURCES RuleSetTest.cpp LIBS hexrules LABELS core)
target_compile_definitions(hexrules_ruleset_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexrules_ledger_test SOURCES LedgerTest.cpp LIBS hexrules LABELS core)

hexgames_add_gtest(hexrules_package_test SOURCES PackageTest.cpp LIBS hexrules LABELS "package;trc")
target_compile_definitions(hexrules_package_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
# Copyright Ben Paul Wise. All Rights Reserved.
