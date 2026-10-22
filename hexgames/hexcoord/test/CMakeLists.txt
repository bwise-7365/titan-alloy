# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. The reference pixels in GridTest.cpp come from reference-centres.py,
# which runs the sheet renderer's own Grid class; it is not part of the build.
hexgames_add_gtest(hexcoord_abc_test SOURCES AbcTest.cpp LIBS hexcoord LABELS core)
hexgames_add_gtest(hexcoord_direction_test SOURCES DirectionTest.cpp LIBS hexcoord LABELS core)
hexgames_add_gtest(hexcoord_hexaddress_test SOURCES HexAddressTest.cpp LIBS hexcoord LABELS core)
hexgames_add_gtest(hexcoord_hexidformat_test SOURCES HexIdFormatTest.cpp LIBS hexcoord LABELS core)
hexgames_add_gtest(hexcoord_grid_test SOURCES GridTest.cpp LIBS hexcoord LABELS core)
# Copyright Ben Paul Wise. All Rights Reserved.
