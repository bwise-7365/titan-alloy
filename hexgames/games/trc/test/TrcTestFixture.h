// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// What the TRC tests share: the real package, a position with no units at a chosen turn and phase,
// lookups by counter and hex id, and a search for open steppe (clear hexes with no feature, river,
// blocked hexside or water nearby) so a test's arithmetic never meets terrain it did not ask for.
// ----------------------------------------------
#pragma once
#include "TrcBinding.h"
#include "TrcPolicySet.h"

#include "hexengine/Session.h"
#include "hexmodel/PositionBuilder.h"
#include "hexxml/SaveDoc.h"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace TrcTest {

  inline std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  inline std::shared_ptr<const HexRules::GameDefinition>
  definition()
  {
    return Trc::loadPackage(root() / "packages" / "xml" / "trc.package.xml");
  }

  // No unit anywhere, the clock at (turn, phase, side), the weather DRM at zero.
  inline HexModel::Position
  blank(const HexRules::GameDefinition& definition, int turn, const std::string& phase, const std::string& side)
  {
    HexXml::SaveDoc doc;
    doc.format = "hexsave-1.0";
    doc.kind = "scenario";
    doc.game = "trc";
    doc.cursor.turn = turn;
    doc.cursor.phase = phase;
    doc.cursor.side = side;
    HexXml::SaveSideDoc axis;
    axis.id = "axis";
    axis.flags.push_back(HexXml::SaveFlagDoc{"weather-drm", "0"});
    doc.sides.push_back(axis);
    const Trc::TrcStateCodec codec(*definition.rules);
    const HexEngine::NoObligationCodec obligations;
    return HexModel::PositionBuilder::build(doc, *definition.board, *definition.roster, *definition.rules, codec,
                                            obligations);
  }

  inline HexModel::UnitId
  unit(const HexRules::GameDefinition& definition, const std::string& id)
  {
    const std::optional<HexModel::UnitId> found = definition.roster->find(HexModel::CounterId{id});
    if (!found) {
      throw std::invalid_argument("TrcTest: no counter '" + id + "'");
    }
    return *found;
  }

  inline HexModel::HexIndex
  hex(const HexRules::GameDefinition& definition, const std::string& id)
  {
    return definition.board->indexOf(HexCoord::HexId{id});
  }

  inline HexModel::HexIndex
  neighbour(const HexRules::GameDefinition& definition, HexModel::HexIndex from, int direction)
  {
    const std::optional<HexModel::HexIndex> to = definition.board->neighbour(from, static_cast<HexModel::Direction>(direction));
    if (!to) {
      throw std::invalid_argument("TrcTest: no neighbour in that direction");
    }
    return *to;
  }

  // Clear, featureless, riverless and rail-free, with no blocked hexside and no map edge, and so is
  // every hex within `radius` (country and district borders change nothing in play).
  inline bool
  openP(const HexRules::GameDefinition& definition, const Trc::TrcFacts& facts, HexModel::HexIndex centre, int radius)
  {
    const HexModel::Board& board = *definition.board;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
      if (radius < board.distance(centre, hex)) {
        continue;
      }
      if ("clear" != definition.rules->hexTerrain()[board.terrain(hex).value].id || !board.features(hex).empty() ||
          facts.riverHexP(hex) || facts.railHexP(hex)) {
        return false;
      }
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        if (facts.blockedHexsideP(hex, static_cast<HexModel::Direction>(d)) ||
            !board.neighbour(hex, static_cast<HexModel::Direction>(d))) {
          return false;
        }
      }
    }
    return true;
  }

  // The k-th open centre (radius 2), each at least ten hexes from the ones before it.
  inline std::vector<HexModel::HexIndex>
  openGround(const HexRules::GameDefinition& definition, const Trc::TrcFacts& facts, std::size_t count)
  {
    std::vector<HexModel::HexIndex> out;
    const HexModel::Board& board = *definition.board;
    for (std::size_t h = 0; h < board.hexCount() && out.size() < count; ++h) {
      const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
      bool farP = true;
      for (HexModel::HexIndex earlier : out) {
        farP = farP && 10 <= board.distance(earlier, hex);
      }
      if (farP && openP(definition, facts, hex, 2)) {
        out.push_back(hex);
      }
    }
    if (out.size() < count) {
      throw std::invalid_argument("TrcTest: the sheet has too little open steppe");
    }
    return out;
  }

  inline HexEngine::Ctx
  ctx(const HexRules::GameDefinition& definition, const HexModel::Position& position)
  {
    return HexEngine::Ctx{*definition.board, *definition.rules, *definition.roster, position};
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
    throw std::invalid_argument("TrcTest: no seed in the first thousand rolls that");
  }

}  // namespace TrcTest
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
