# Copyright Ben Paul Wise. All Rights Reserved.
# Toolchain smoke test: proves GoogleTest, TinyXML2 and the C++20 flags work before any module exists.
hexgames_add_gtest(smoke_test
  SOURCES SmokeTest.cpp
  LIBS tinyxml2::tinyxml2
  LABELS core)
# Copyright Ben Paul Wise. All Rights Reserved.
