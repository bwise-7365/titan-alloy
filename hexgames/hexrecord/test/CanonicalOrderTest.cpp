// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Canonical order: units and control hexes sorted by @id, moves sorted by @n, regardless of the
// order they were added to the model in.
// ----------------------------------------------
#include "hexrecord/SaveModel.h"

#include <gtest/gtest.h>

namespace {

  using HexRecord::SaveControlHex;
  using HexRecord::SaveModel;
  using HexRecord::SaveMove;
  using HexRecord::SaveUnit;

  SaveUnit
  makeUnit(std::string id, std::string hex)
  {
    SaveUnit u;
    u.id = std::move(id);
    u.counter = u.id;
    u.type = "infantry";
    u.owner = "axis";
    u.hex = std::move(hex);
    return u;
  }

  SaveMove
  makeMove(int n)
  {
    SaveMove m;
    m.n = n;
    m.turn = 1;
    m.phase = "p";
    m.side = "axis";
    m.cmd = "end-phase";
    return m;
  }

}  // namespace

TEST(CanonicalOrderTest, UnitsControlHexesAndMovesAreSorted)
{
  SaveModel model;
  model.kind = "save";
  model.game = "trc";
  model.package = "p.xml";
  model.seed = 1;
  model.cursor.turn = 1;
  model.cursor.phase = "p";

  model.units = {makeUnit("z-unit", "A1"), makeUnit("a-unit", "A2")};
  model.controlHexes = {SaveControlHex{"Z9", "axis"}, SaveControlHex{"A1", "russian"}};
  model.log = {makeMove(3), makeMove(1), makeMove(2)};

  const std::string text = HexRecord::canonicalText(model);

  const std::size_t unitA = text.find("id=\"a-unit\"");
  const std::size_t unitZ = text.find("id=\"z-unit\"");
  ASSERT_NE(std::string::npos, unitA);
  ASSERT_NE(std::string::npos, unitZ);
  EXPECT_LT(unitA, unitZ);

  const std::size_t hexA = text.find("<hex id=\"A1\"");
  const std::size_t hexZ = text.find("<hex id=\"Z9\"");
  ASSERT_NE(std::string::npos, hexA);
  ASSERT_NE(std::string::npos, hexZ);
  EXPECT_LT(hexA, hexZ);

  const std::size_t move1 = text.find("n=\"1\"");
  const std::size_t move2 = text.find("n=\"2\"");
  const std::size_t move3 = text.find("n=\"3\"");
  ASSERT_NE(std::string::npos, move1);
  ASSERT_NE(std::string::npos, move2);
  ASSERT_NE(std::string::npos, move3);
  EXPECT_LT(move1, move2);
  EXPECT_LT(move2, move3);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
