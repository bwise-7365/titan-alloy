// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What the PGG tests share: the real package, a position with no units at a chosen turn and phase,
// lookups by counter and hex id, searches for hexside kinds (a road with no river, a river with no
// road, a clear pair) so a test's arithmetic never meets terrain it did not ask for, and a seed that
// rolls a wanted die.
// ----------------------------------------------
#pragma once
#include "PggBinding.h"
#include "PggPolicySet.h"

#include "hexengine/Session.h"
#include "hexmodel/PositionBuilder.h"
#include "hexxml/SaveDoc.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace PggTest {

  inline std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  inline std::shared_ptr<const HexRules::GameDefinition>
  definition()
  {
    return Pgg::loadPackage(root() / "packages" / "xml" / "pgg.package.xml");
  }

  // No unit anywhere and no flag, the clock at (turn, phase, side).
  inline HexModel::Position
  blank(const Pgg::PggPolicySet& set, int turn, const std::string& phase, const std::string& side)
  {
    const HexRules::GameDefinition& definition = set.facts().definition();
    HexXml::SaveDoc doc;
    doc.format = "hexsave-1.0";
    doc.kind = "scenario";
    doc.game = "pgg";
    doc.cursor.turn = turn;
    doc.cursor.phase = phase;
    doc.cursor.side = side;
    return HexModel::PositionBuilder::build(doc, *definition.board, *definition.roster, *definition.rules,
                                            set.stateCodec(), set.battleCodec());
  }

  inline HexModel::UnitId
  unit(const Pgg::PggPolicySet& set, const std::string& id)
  {
    return set.facts().counter(id);
  }

  inline HexModel::HexIndex
  hex(const Pgg::PggPolicySet& set, const std::string& id)
  {
    return set.facts().hex(id);
  }

  inline HexEngine::Ctx
  ctx(const Pgg::PggPolicySet& set, const HexModel::Position& position)
  {
    const HexRules::GameDefinition& definition = set.facts().definition();
    return HexEngine::Ctx{*definition.board, *definition.rules, *definition.roster, position};
  }

  // The first hex (in index order) with a neighbour such that wantedP(from, direction, to) holds, both
  // hexes and every neighbour of both on the map and away from the westernmost columns.
  inline std::pair<HexModel::HexIndex, HexModel::Direction>
  findHexside(const Pgg::PggPolicySet& set,
              const std::function<bool(HexModel::HexIndex, HexModel::Direction, HexModel::HexIndex)>& wantedP)
  {
    const HexModel::Board& board = *set.facts().definition().board;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexModel::HexIndex from{static_cast<std::uint32_t>(h)};
      if (4 > set.facts().column(from) || 50 < set.facts().column(from)) {
        continue;
      }
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const HexModel::Direction direction = static_cast<HexModel::Direction>(d);
        const std::optional<HexModel::HexIndex> to = board.neighbour(from, direction);
        if (to && wantedP(from, direction, *to)) {
          return {from, direction};
        }
      }
    }
    throw std::invalid_argument("PggTest: the sheet has no such hexside");
  }

  // Clear, with no city, no river or road on any of its six hexsides.
  inline bool
  plainP(const Pgg::PggPolicySet& set, HexModel::HexIndex hex)
  {
    const Pgg::PggFacts& facts = set.facts();
    const HexModel::Board& board = *facts.definition().board;
    if ("clear" != facts.terrainOf(hex) || facts.majorCityP(hex) || facts.minorCityP(hex) || facts.railHexP(hex)) {
      return false;
    }
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const HexModel::Direction direction = static_cast<HexModel::Direction>(d);
      const std::optional<HexModel::HexIndex> near = board.neighbour(hex, direction);
      if (!near || facts.riverP(hex, direction) || facts.roadP(hex, *near) || facts.lakeP(*near)) {
        return false;
      }
    }
    return true;
  }

  inline std::uint64_t
  seedRolling(HexEngine::StreamTag tag, int wanted)
  {
    for (std::uint64_t seed = 1; seed < 1000; ++seed) {
      HexEngine::PrngStreams streams(seed);
      if (wanted == HexEngine::rollDie(streams, tag, 6)) {
        return seed;
      }
    }
    throw std::invalid_argument("PggTest: no seed in the first thousand rolls that");
  }

}  // namespace PggTest
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
