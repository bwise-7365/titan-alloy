# Copyright Ben Paul Wise. All Rights Reserved.
hexgames_add_gtest(hexxml_xmldocument_test SOURCES XmlDocumentTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_xmldocument_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexxml_rulesdoc_test SOURCES RulesDocTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_rulesdoc_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexxml_sheetdoc_test SOURCES SheetDocTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_sheetdoc_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexxml_countersetdoc_test SOURCES CounterSetDocTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_countersetdoc_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexxml_packagedoc_test SOURCES PackageDocTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_packagedoc_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

hexgames_add_gtest(hexxml_savedoc_test SOURCES SaveDocTest.cpp LIBS hexxml LABELS core)
target_compile_definitions(hexxml_savedoc_test PRIVATE HEXGAMES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
# Copyright Ben Paul Wise. All Rights Reserved.
