# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. The suites that read real documents need HEXGAMES_SOURCE_DIR; the M4
# suites also need this directory on their include path for TrcRecordFixture.h.
hexgames_add_gtest(hexrecord_write_test SOURCES WriteRecordTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_canonical_order_test SOURCES CanonicalOrderTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_diff_test SOURCES DiffTest.cpp LIBS hexrecord LABELS records)
hexgames_add_gtest(hexrecord_golden_test SOURCES GoldenTest.cpp LIBS hexrecord LABELS records)

hexgames_add_gtest(hexrecord_read_test SOURCES ReadRecordTest.cpp LIBS hexrecord LABELS records)
target_compile_definitions(hexrecord_read_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexrecord_roundtrip_test SOURCES RoundTripTest.cpp LIBS hexrecord LABELS records)
target_compile_definitions(hexrecord_roundtrip_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexrecord_replay_test SOURCES ReplayTest.cpp LIBS hexrecord LABELS records)
target_compile_definitions(hexrecord_replay_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(hexrecord_replay_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

hexgames_add_gtest(hexrecord_pending_save_test SOURCES PendingSaveTest.cpp LIBS hexrecord LABELS records)
target_compile_definitions(hexrecord_pending_save_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(hexrecord_pending_save_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

hexgames_add_gtest(trc_smoke_golden_test SOURCES TrcSmokeGoldenTest.cpp LIBS hexrecord LABELS "golden;trc")
target_compile_definitions(trc_smoke_golden_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(trc_smoke_golden_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Copyright Ben Paul Wise. All Rights Reserved.
