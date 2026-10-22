// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Toolchain smoke test: C++20 concepts, GoogleTest and TinyXML2 all link and run.
// ----------------------------------------------
#include "hexcoord/Abc.h"
#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexcoord/HexAddress.h"
#include "hexmodel/Board.h"
#include "hexmodel/Ids.h"
#include "hexmodel/Position.h"
#include "hexmodel/Quantities.h"
#include "hexmodel/Roster.h"
#include "hexsearch/Search.h"
#include "hexrules/Ledger.h"
#include "hexrules/Package.h"
#include "hexrules/RuleSet.h"
#include "hexengine/Command.h"
#include "hexengine/Event.h"
#include "hexengine/Player.h"
#include "hexengine/Policies.h"
#include "hexengine/PrngStreams.h"
#include "hexengine/Session.h"
#include "hexrecord/Record.h"

#include <gtest/gtest.h>
#include <tinyxml2.h>

#include <concepts>

// The ABC algebra is constexpr, so its first invariants are compile-time facts (tricoord testtri).
static_assert(HexCoord::Abc{} .hvCode() == 0, "the origin is a hex centre");
static_assert(HexCoord::AVec.hvCode() == 2, "origin + A is a vertex");
static_assert(HexCoord::QVec.toAbc().hvCode() == 0, "every Qrs is a centre");
static_assert(HexCoord::Abc{1, 1, 1} == HexCoord::Abc{}, "(d,d,d) is the origin");
static_assert(HexCoord::edgeDist(HexCoord::Abc{}, HexCoord::AVec) == 1, "A is one edge long");
static_assert(HexCoord::hexDist(HexCoord::QVec * 3, HexCoord::Qrs{}) == 3, "3Q is three hexes");
static_assert(HexCoord::edgeDist(HexCoord::Abc{}, (HexCoord::QVec * 3).toAbc()) == 6,
              "straight-line edge distance is twice the hex distance");

namespace {

  template <class T>
  concept Additive = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
  };

  template <Additive T>
  T
  twice(T v)
  {
    return v + v;
  }

}  // namespace

TEST(Smoke, ConceptsCompileAndRun)
{
  EXPECT_EQ(6, twice(3));
}

TEST(Smoke, TinyXml2ParsesADocument)
{
  tinyxml2::XMLDocument doc;
  ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse("<game id='trc'><side id='axis'/></game>"));
  const tinyxml2::XMLElement* game = doc.RootElement();
  ASSERT_NE(nullptr, game);
  EXPECT_STREQ("trc", game->Attribute("id"));
  EXPECT_STREQ("axis", game->FirstChildElement("side")->Attribute("id"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
