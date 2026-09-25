// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Style.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace {

  using HexView::Color;

  TEST(StyleTest, ParsesPaletteColours)
  {
    EXPECT_EQ((Color{0x05, 0xbc, 0xf2, 255}), Color::parse("#05bcf2", "palette water"));
    EXPECT_EQ((Color{0xff, 0xff, 0xff, 255}), Color::parse("#FFFFFF", "palette white"));
    EXPECT_EQ((Color{0, 0, 0, 255}), Color::parse("#000000", "palette ink"));
  }

  TEST(StyleTest, RefusesNoneAndMalformed)
  {
    for (const char* bad : {"none", "#fff", "#12345g", "123456", "#1234567", ""}) {
      try {
        Color::parse(bad, "palette murk");
        FAIL() << "accepted '" << bad << "'";
      }
      catch (const std::invalid_argument& e) {
        EXPECT_NE(std::string::npos, std::string(e.what()).find("palette murk")) << e.what();
      }
    }
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
