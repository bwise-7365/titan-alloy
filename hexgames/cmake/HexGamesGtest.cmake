# Copyright Ben Paul Wise. All Rights Reserved.
# hexgames_add_gtest(<target> SOURCES <files...> [LIBS <targets...>] [LABELS <labels...>] [GUI])
#
# One executable per test suite, linked against gtest_main, registered with ctest by discovery so that
# every TEST(Suite, Case) is its own ctest entry carrying the given labels.  GUI marks a test that
# needs the Qt runtime beside it (hexgames_deploy_qt is applied) and the offscreen platform.
function(hexgames_add_gtest target)
  cmake_parse_arguments(HG "GUI" "" "SOURCES;LIBS;LABELS" ${ARGN})
  if(NOT HG_SOURCES)
    message(FATAL_ERROR "hexgames_add_gtest(${target}): SOURCES is required")
  endif()
  add_executable(${target} ${HG_SOURCES})
  target_link_libraries(${target} PRIVATE ${HG_LIBS} GTest::gtest_main)
  if(HG_GUI)
    hexgames_deploy_qt(${target})
    set(_env "QT_QPA_PLATFORM=offscreen")
  else()
    set(_env "")
  endif()
  string(REPLACE ";" ";" _labels "${HG_LABELS}")
  gtest_discover_tests(${target}
    PROPERTIES LABELS "${_labels}" ENVIRONMENT "${_env}"
    DISCOVERY_MODE PRE_TEST)
endfunction()
# Copyright Ben Paul Wise. All Rights Reserved.
