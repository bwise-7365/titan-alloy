// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Generate.h"

#include "hexgen/Network.h"
#include "hexgen/Noise.h"
#include "hexgen/Random.h"
#include "hexgen/Terrain.h"
#include "hexgen/ToSheet.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace HexGen {

  namespace {

    StreamTag
    streamOfSide(Side side)
    {
      switch (side) {
        case Side::Top:
          return StreamTag::EdgeTop;
        case Side::Bottom:
          return StreamTag::EdgeBottom;
        case Side::Left:
          return StreamTag::EdgeLeft;
        case Side::Right:
          return StreamTag::EdgeRight;
      }
      throw std::invalid_argument("streamOfSide: not a Side");
    }

  }  // namespace

  GenMap
  generateMap(const Settings& s)
  {
    validate(s, "hexgen settings " + s.id);
    const auto seed = static_cast<std::uint64_t>(s.seed);
    GenMap m(s);
    std::mt19937_64 heightGen = streamOf(seed, StreamTag::Height);
    m.height = fieldOf(m, s.method, s.roughness, heightGen);
    std::mt19937_64 coverGen = streamOf(seed, StreamTag::Cover);
    m.cover = fieldOf(m, s.method, s.roughness, coverGen);
    std::mt19937_64 cityGen = streamOf(seed, StreamTag::City);
    assignTerrain(m, s, cityGen);
    if (s.altitude) {
      calibrateAltitude(m, s);
    }
    if (s.roads) {
      buildRoads(m, s);
      std::vector<SideSetting> sides = s.roads->sides;
      std::stable_sort(sides.begin(), sides.end(), [](const SideSetting& a, const SideSetting& b) { return a.at < b.at; });
      for (const SideSetting& side : sides) {
        std::mt19937_64 sideGen = streamOf(seed, streamOfSide(side.at));
        addEdgeRoads(m, s, side, sideGen);
      }
    }
    return m;
  }

  HexXml::SheetDoc
  generate(const Settings& s)
  {
    return toSheet(generateMap(s), s);
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
