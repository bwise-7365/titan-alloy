# Copyright Ben Paul Wise. All Rights Reserved.
hexgames_add_gtest(hexgen_test
  SOURCES SettingsTest.cpp FieldsTest.cpp NetworkTest.cpp SheetTest.cpp
  LIBS hexgen
  LABELS hexgen core)
target_compile_definitions(hexgen_test PRIVATE HEXRULES_DIR="${HEXRULES_DIR}"
                                               HEXGEN_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Generated sheets validate against hexsheet.xsd (needs the Python with lxml that the xsd tests use).
if(_py AND NOT _lxmlMissing)
  add_test(NAME hexgen_sheets_validate
           COMMAND "${_py}" "${CMAKE_CURRENT_SOURCE_DIR}/validate-generated.py" $<TARGET_FILE:hexgen_cli>
                   "${HEXRULES_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/generated")
  set_tests_properties(hexgen_sheets_validate PROPERTIES LABELS "hexgen;xsd")
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
