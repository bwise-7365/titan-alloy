# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. ReadRecordTest and RoundTripTest need hexxml (a real target as of
# 2026-09-13; see the note in ../CMakeLists.txt) and the real trc-test.xml fixture.
hexgames_add_gtest(hexrecord_write_test SOURCES WriteRecordTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_canonical_order_test SOURCES CanonicalOrderTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_diff_test SOURCES DiffTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_golden_test SOURCES GoldenTest.cpp LIBS hexrecord LABELS records)

if(TARGET hexxml)
  hexgames_add_gtest(hexrecord_read_test SOURCES ReadRecordTest.cpp LIBS hexrecord LABELS records)
  target_compile_definitions(hexrecord_read_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

  hexgames_add_gtest(hexrecord_roundtrip_test SOURCES RoundTripTest.cpp LIBS hexrecord LABELS records)
  target_compile_definitions(hexrecord_roundtrip_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
