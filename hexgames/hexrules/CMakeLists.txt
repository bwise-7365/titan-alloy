# Copyright Ben Paul Wise. All Rights Reserved.
# hexrules -- RuleSet from the rules document, package loading and bindings, the coverage ledger.
#
# BoardBuilder, RosterBuilder and PositionBuilder are declared in namespace HexModel (they are
# friends of Board/Roster/Position, so the friend declarations there must name them unqualified in
# that namespace) and so their headers and .cpp files live under hexmodel/ -- but their .cpp files
# need the complete HexRules::RuleSet type, and hexmodel must not depend on hexrules (hexrules
# already depends on hexmodel; a dependency the other way would be a cycle). They are compiled here
# instead: hexrules already links hexmodel (which PUBLICly links hexxml), so every header those
# three .cpp files need is already visible, with no link cycle.
add_library(hexrules STATIC
  RuleSet.cpp
  RuleSetBuilder.cpp
  RuleSetBuilderCombat.cpp
  Ledger.cpp
  PackageLoader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../hexmodel/BoardBuilder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../hexmodel/RosterBuilder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../hexmodel/PositionBuilder.cpp)
target_include_directories(hexrules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexrules PUBLIC hexmodel)
target_compile_features(hexrules PUBLIC cxx_std_20)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
  add_subdirectory(test)
endif()
# Copyright Ben Paul Wise. All Rights Reserved.
