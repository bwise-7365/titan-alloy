// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Strong identifiers. Dense ids index vectors; string ids name things in the XML documents. No two
// tags convert to each other, so a UnitId can never be handed to a function wanting a HexIndex.
// ----------------------------------------------
#pragma once
#include <bitset>
#include <compare>
#include <cstdint>
#include <string>

namespace HexModel {

  // A dense index into a Board or Roster table.
  template <class Tag>
  struct Id {
    std::uint32_t value = 0;
    constexpr auto operator<=>(const Id&) const = default;
  };

  struct HexTag;
  struct UnitTag;
  struct SideTag;
  struct UnitTypeTag;
  struct TerrainTag;
  struct EdgeTerrainTag;
  struct NetworkTag;
  struct LayerTag;
  struct RegionTag;
  struct SpaceTag;
  struct PhaseTag;
  struct ModeTag;
  struct TraceTag;
  struct ResolverTag;
  struct ModifierTag;
  struct RandomizerTag;
  struct TrackTag;

  using HexIndex = Id<HexTag>;
  using UnitId = Id<UnitTag>;
  using SideId = Id<SideTag>;
  using UnitTypeId = Id<UnitTypeTag>;
  using TerrainId = Id<TerrainTag>;
  using EdgeTerrainId = Id<EdgeTerrainTag>;
  using NetworkId = Id<NetworkTag>;
  using LayerId = Id<LayerTag>;
  using RegionId = Id<RegionTag>;
  using SpaceId = Id<SpaceTag>;
  using PhaseId = Id<PhaseTag>;
  using ModeId = Id<ModeTag>;
  using TraceId = Id<TraceTag>;
  using ResolverId = Id<ResolverTag>;
  using ModifierId = Id<ModifierTag>;
  using RandomizerId = Id<RandomizerTag>;
  using TrackId = Id<TrackTag>;

  // A string identifier as written in a document: a rule id, a counter id, a printed hex id.
  template <class Tag>
  struct Name {
    std::string text;
    auto operator<=>(const Name&) const = default;
  };
  struct RuleTag;
  struct CounterTag;
  using RuleId = Name<RuleTag>;
  using CounterId = Name<CounterTag>;

  // Bit i is side i. Sixteen sides covers every game in view (Dai Senso's three factions, a
  // Diplomacy variant's eleven powers); the loader throws if a rules document declares more.
  inline constexpr std::size_t kMaxSides = 16;
  using SideMask = std::bitset<kMaxSides>;

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
