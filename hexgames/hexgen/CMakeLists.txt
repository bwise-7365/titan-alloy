# Copyright Ben Paul Wise. All Rights Reserved.
# hexgen -- the procedural map generator (doc/2026-10-31-map-generator-plan.md): settings in the
# HexRules hexgen.xsd language in, a hexsheet document out. Headless; hexgen_cli is its command, and
# HexMapEd's Generate dock its GUI. Depends on hexcoord, hexxml, hexsearch (Dijkstra) and hexmaped
# (the sheet writer); the command also draws an SVG through hexview.
add_library(hexgen STATIC
  Settings.cpp
  GenMap.cpp
  Noise.cpp
  Terrain.cpp
  Network.cpp
  Chains.cpp
  ToSheet.cpp
  Generate.cpp)
target_include_directories(hexgen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexgen PUBLIC hexcoord hexxml hexsearch hexmaped)
target_compile_features(hexgen PUBLIC cxx_std_20)

add_executable(hexgen_cli app/HexGenCli.cpp)
target_link_libraries(hexgen_cli PRIVATE hexgen hexview)
target_compile_features(hexgen_cli PRIVATE cxx_std_20)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
