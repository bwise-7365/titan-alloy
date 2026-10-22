// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Printed identifiers. The four games print their hexes four different ways, and between them they
// use every placeholder the renderer's make_id knows.
// ----------------------------------------------
#include "hexcoord/Grid.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {

  using HexCoord::HexIdFormat;
  using HexCoord::letters;

}  // namespace

TEST(HexIdFormatTest, RendersAllFourGames)
{
  // The Russian Campaign: rows are lettered, columns count down from 33, so the Kerch hex at grid
  // cell (14, 36) prints its row letter 37 and its column number 19.
  EXPECT_EQ("KK19", HexIdFormat("{rowletter}{col}").render(33 + 14 * -1, 1 + 36 * 1));
  EXPECT_EQ("A33", HexIdFormat("{rowletter}{col}").render(33, 1));

  // D-Day at Tarawa: two-digit row then two-digit column, rows counting down from 25.
  EXPECT_EQ("2336", HexIdFormat("{row:02}{col:02}").render(2 + 34 * 1, 25 + 2 * -1));
  EXPECT_EQ("2502", HexIdFormat("{row:02}{col:02}").render(2, 25));

  // Dai Senso: the same, with the map's letter in front.
  EXPECT_EQ("w5227", HexIdFormat("w{row:02}{col:02}").render(1 + 26 * 1, 61 + 9 * -1));
  EXPECT_EQ("e6101", HexIdFormat("e{row:02}{col:02}").render(1, 61));

  // Panzergruppe Guderian: column first.
  EXPECT_EQ("0109", HexIdFormat("{col:02}{row:02}").render(1, 9));
  EXPECT_EQ("5631", HexIdFormat("{col:02}{row:02}").render(56, 31));

  EXPECT_EQ("A", letters(1));
  EXPECT_EQ("Z", letters(26));
  EXPECT_EQ("AA", letters(27));
  EXPECT_EQ("BB", letters(28));
  EXPECT_EQ("KK", letters(37));
  EXPECT_THROW(letters(0), std::invalid_argument);
  EXPECT_THROW(letters(-4), std::invalid_argument);

  EXPECT_EQ("x1y", HexIdFormat("x{col}y").render(1, 1));  // text outside the braces is literal
  EXPECT_THROW(HexIdFormat("{hex}"), std::invalid_argument);
  EXPECT_THROW(HexIdFormat("{col"), std::invalid_argument);
  EXPECT_THROW(HexIdFormat("{col:}"), std::invalid_argument);
  EXPECT_THROW(HexIdFormat("{colletter}").render(0, 1), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
