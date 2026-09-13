# Copyright Ben Paul Wise. All Rights Reserved.
# hexxml -- the TinyXML2 facade (XmlDocument) and the five document value models. No dependency on
# any other hexgames module; only these .cpp files include <tinyxml2.h>.
add_library(hexxml STATIC
  XmlDocument.cpp
  RulesDoc.cpp
  SheetDoc.cpp
  CounterSetDoc.cpp
  PackageDoc.cpp
  SaveDoc.cpp)
target_include_directories(hexxml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexxml PRIVATE tinyxml2::tinyxml2)
target_compile_features(hexxml PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
