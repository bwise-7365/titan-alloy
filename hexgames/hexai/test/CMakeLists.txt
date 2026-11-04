# Copyright Ben Paul Wise. All Rights Reserved.
# The tactics walk the toy board of the hexsearch tests.
hexgames_add_gtest(hexai_tactics_test SOURCES TacticsTest.cpp SeizeTest.cpp LIBS hexai hexrules LABELS core)
target_compile_definitions(hexai_tactics_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(hexai_tactics_test PRIVATE ${CMAKE_SOURCE_DIR}/hexsearch/test)
# Copyright Ben Paul Wise. All Rights Reserved.
