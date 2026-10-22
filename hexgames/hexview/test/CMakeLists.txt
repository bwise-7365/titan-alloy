# Copyright Ben Paul Wise. All Rights Reserved.
# One executable per suite. The reference is hexsheet2svg.py itself: svg-golden.py (beside this file)
# renders a sheet's reference SVG and dumps its grid geometry at test time, into the build tree, and
# normalises and compares two SVGs for MapSvgGoldenTest. Tests that run it need the Python with lxml
# (HEXGAMES_PYTHON, else whatever find_package finds); without one they are not built, with a message.
set(_hexviewPy "")
if(HEXGAMES_PYTHON AND EXISTS "${HEXGAMES_PYTHON}")
  set(_hexviewPy "${HEXGAMES_PYTHON}")
else()
  find_package(Python3 COMPONENTS Interpreter QUIET)
  set(_hexviewPy "${Python3_EXECUTABLE}")
endif()

set(_hexviewDefs
  HEXVIEW_SOURCE_ROOT="${PROJECT_SOURCE_DIR}"
  HEXVIEW_OUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/out"
  HEXVIEW_PYTHON="${_hexviewPy}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/out")

hexgames_add_gtest(hexview_style_test SOURCES StyleTest.cpp LIBS hexview LABELS core)
hexgames_add_gtest(hexview_scene_test SOURCES SceneTest.cpp LIBS hexview LABELS core)
hexgames_add_gtest(hexview_linegeometry_test SOURCES LineGeometryTest.cpp LIBS hexview LABELS core)
hexgames_add_gtest(hexview_scratch_test SOURCES ScratchTest.cpp LIBS hexview LABELS core)
hexgames_add_gtest(hexview_symbollibrary_test SOURCES SymbolLibraryTest.cpp LIBS hexview LABELS core)
hexgames_add_gtest(hexview_mapscenebuilder_test SOURCES MapSceneBuilderTest.cpp LIBS hexview LABELS core)
foreach(_t hexview_symbollibrary_test hexview_mapscenebuilder_test hexview_linegeometry_test)
  target_compile_definitions(${_t} PRIVATE ${_hexviewDefs})
endforeach()

if(_hexviewPy)
  hexgames_add_gtest(hexview_mapframe_test SOURCES MapFrameTest.cpp LIBS hexview LABELS core)
  hexgames_add_gtest(hexview_map_svg_golden_test SOURCES MapSvgGoldenTest.cpp LIBS hexview LABELS golden)
  target_compile_definitions(hexview_mapframe_test PRIVATE ${_hexviewDefs})
  target_compile_definitions(hexview_map_svg_golden_test PRIVATE ${_hexviewDefs})
else()
  message(STATUS "hexview: no Python for the reference renderer; MapFrameTest and MapSvgGoldenTest are skipped")
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
