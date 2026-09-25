# Copyright Ben Paul Wise. All Rights Reserved.
# hexview -- the presentation model, no Qt (M10). The map half (task 17) is implemented: Style, Scene,
# MapFrame, LineGeometry, SymbolLibrary, MapSceneBuilder and the scene writers form the STATIC hexview
# library. The play-state half is still contract headers only (FaceModel, ViewState,
# StateSceneBuilder, Interaction, Replay); hexview_contracts compiles every header once, so a contract
# that does not compile is found before anyone implements it. It is EXCLUDE_FROM_ALL:
#   cmake --build --preset win-msvc-debug --target hexview_contracts
add_library(hexview STATIC
  Style.cpp
  Scene.cpp
  Shapes.cpp
  MapFrame.cpp
  LineGeometry.cpp
  Scratch.cpp
  MarkShapes.cpp
  SymbolLibrary.cpp
  Buildings.cpp
  MapMarks.cpp
  MapSceneBuilder.cpp
  MapAreaLayers.cpp
  MapLineLayers.cpp
  MapGlyphLayers.cpp
  MapTextLayers.cpp
  MapPanelLayer.cpp
  WriterText.cpp
  SceneWriters.cpp
  SceneJson.cpp)
target_include_directories(hexview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexview PUBLIC hexxml hexcoord hexmodel)
target_compile_features(hexview PUBLIC cxx_std_20)

add_library(hexview_contracts OBJECT EXCLUDE_FROM_ALL ContractsCompile.cpp)
target_include_directories(hexview_contracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexview_contracts PUBLIC hexrecord hexengine hexrules hexxml hexmodel hexcoord)
target_compile_features(hexview_contracts PUBLIC cxx_std_20)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
