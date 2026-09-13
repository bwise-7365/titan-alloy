# Copyright Ben Paul Wise. All Rights Reserved.
# Toolchain smoke test: proves GoogleTest, TinyXML2 and the C++20 flags work, and that every contract
# header in the tree compiles (it includes them without linking against their implementations).
hexgames_add_gtest(smoke_test
  SOURCES SmokeTest.cpp
  LIBS tinyxml2::tinyxml2 hexcoord hexmodel hexsearch
  LABELS core)
# Copyright Ben Paul Wise. All Rights Reserved.
