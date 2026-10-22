# Copyright Ben Paul Wise. All Rights Reserved.
# hexmaped -- the map sheet editor's model (Qt-free: the document, its frame, the schema vocabularies,
# the writer) and, under HEXGAMES_BUILD_GUI, the HexMapEd Qt6 application in app/.
add_library(hexmaped STATIC
  Schema.cpp
  SheetFrame.cpp
  SheetWriter.cpp
  Document.cpp)
target_include_directories(hexmaped PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexmaped PUBLIC hexxml hexcoord)
target_compile_features(hexmaped PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
if(HEXGAMES_BUILD_GUI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/CMakeLists.txt")
  add_subdirectory(app)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
