# Copyright Ben Paul Wise. All Rights Reserved.
hexgames_add_gtest(hexmaped_schema_test SOURCES SchemaTest.cpp LIBS hexmaped LABELS core)
target_compile_definitions(hexmaped_schema_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexmaped_sheetframe_test SOURCES SheetFrameTest.cpp LIBS hexmaped LABELS core)
target_compile_definitions(hexmaped_sheetframe_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexmaped_sheetwriter_test SOURCES SheetWriterTest.cpp LIBS hexmaped LABELS core)
target_compile_definitions(hexmaped_sheetwriter_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexmaped_document_test SOURCES DocumentTest.cpp LIBS hexmaped LABELS core)
target_compile_definitions(hexmaped_document_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
# Copyright Ben Paul Wise. All Rights Reserved.
