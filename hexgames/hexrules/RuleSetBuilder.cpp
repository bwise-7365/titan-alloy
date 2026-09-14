// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/RuleSetBuilder.h"

#include "hexrules/RuleSetBuildContext.h"
#include "hexrules/RuleSetBuilderDetail.h"

#include <algorithm>
#include <map>
#include <stdexcept>

namespace HexRules {

  using Detail::resolveMany;
  using Detail::resolveOne;
  using Detail::resolvePurposes;
  using Detail::resolveSideMask;
  using Detail::parseExtent;
  using Detail::parseMoveCost;
  using Detail::parseTurnsOr;

  namespace {

    // The four concealment attributes, already checked against their enumerations by RulesDoc.
    HiddenFrom
    hiddenFromOf(const std::string& text)
    {
      if ("enemy" == text) {
        return HiddenFrom::Enemy;
      }
      if ("all" == text) {
        return HiddenFrom::All;
      }
      throw std::invalid_argument("RuleSetBuilder: hidden-from '" + text + "'");
    }

    Conceals
    concealsOf(const std::string& text)
    {
      if ("values" == text) {
        return Conceals::Values;
      }
      if ("identity" == text) {
        return Conceals::Identity;
      }
      throw std::invalid_argument("RuleSetBuilder: conceals '" + text + "'");
    }

    RevealTrigger
    revealTriggerOf(const std::string& text)
    {
      if ("attacked" == text) {
        return RevealTrigger::Attacked;
      }
      if ("attacking" == text) {
        return RevealTrigger::Attacking;
      }
      if ("combat" == text) {
        return RevealTrigger::Combat;
      }
      if ("adjacent" == text) {
        return RevealTrigger::Adjacent;
      }
      if ("rule" == text) {
        return RevealTrigger::Rule;
      }
      if ("owner" == text) {
        return RevealTrigger::Owner;
      }
      throw std::invalid_argument("RuleSetBuilder: reveal '" + text + "'");
    }

    Rehide
    rehideOf(const std::string& text)
    {
      if ("never" == text) {
        return Rehide::Never;
      }
      if ("rule" == text) {
        return Rehide::Rule;
      }
      throw std::invalid_argument("RuleSetBuilder: rehide '" + text + "'");
    }

    // A hidden type carries all four attributes and a visible one none of them.
    std::optional<Concealment>
    concealmentOf(const HexXml::UnitTypeDoc& u)
    {
      const bool anyP = u.hiddenFrom || u.conceals || !u.reveal.empty() || u.rehide;
      if (!u.hidden) {
        if (anyP) {
          throw std::invalid_argument("unit-type '" + u.id +
                                      "': hidden-from, conceals, reveal and rehide need hidden=\"true\"");
        }
        return std::nullopt;
      }
      if (!u.hiddenFrom || !u.conceals || u.reveal.empty() || !u.rehide) {
        throw std::invalid_argument("unit-type '" + u.id +
                                    "': hidden=\"true\" needs hidden-from, conceals, reveal and rehide");
      }
      Concealment c{hiddenFromOf(*u.hiddenFrom), concealsOf(*u.conceals), {}, rehideOf(*u.rehide)};
      for (const std::string& trigger : u.reveal) {
        c.reveal.push_back(revealTriggerOf(trigger));
      }
      return c;
    }

  }  // namespace

  void
  RuleSetBuilder::buildSides(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::SideDoc& s : doc.sides) {
      ctx.sideByName[s.id] = SideId{static_cast<std::uint32_t>(rs.sides_.size())};
      rs.sides_.push_back(Side{s.id, s.name, "automaton" == s.control});
    }
    rs.hostility_.rows_.assign(rs.sides_.size(), SideMask{});
    for (const HexXml::SideDoc& s : doc.sides) {
      const SideId from = ctx.sideByName.at(s.id);
      for (const std::string& other : s.hostileTo) {
        const SideId to = resolveOne(ctx.sideByName, other, "side");
        rs.hostility_.rows_[from.value].set(to.value);
      }
    }
    for (std::size_t i = 0; i < rs.sides_.size(); ++i) {
      for (std::size_t j = 0; j < rs.sides_.size(); ++j) {
        if (rs.hostility_.rows_[i].test(j) != rs.hostility_.rows_[j].test(i)) {
          throw std::invalid_argument("RuleSetBuilder: asymmetric hostile-to between '" + rs.sides_[i].id +
                                       "' and '" + rs.sides_[j].id + "'");
        }
      }
    }
    return;
  }

  void
  RuleSetBuilder::buildUnitTypes(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::UnitTypeDoc& u : doc.unitTypes) {
      ctx.unitTypeByName[u.id] = UnitTypeId{static_cast<std::uint32_t>(rs.unitTypes_.size())};
      UnitType type;
      type.id = u.id;
      type.name = u.name;
      type.sides = resolveSideMask(ctx.sideByName, u.side);
      type.kind = u.kind;
      type.steps = u.steps;
      type.zoc = u.zoc;
      type.stacking = u.stacking;
      type.concealment = concealmentOf(u);
      rs.unitTypes_.push_back(std::move(type));
    }
    return;
  }

  void
  RuleSetBuilder::buildTerrain(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    const auto convert = [&](const HexXml::TerrainDoc& t) {
      Terrain out;
      out.id = t.id;
      out.name = t.name;
      out.moveCost = parseMoveCost(t.moveCost);
      out.stopP = t.stop;
      out.stopExcept = resolveMany(ctx.unitTypeByName, t.stopExcept, "unit type");
      out.enterOnly = resolveMany(ctx.unitTypeByName, t.enterOnly, "unit type");
      out.defenceMultiplier = t.defenceMultiplier;
      out.shift = t.shift;
      out.blocks = resolvePurposes(t.blocks);
      return out;
    };
    for (const HexXml::TerrainDoc& t : doc.map.hexTerrain) {
      ctx.terrainByName[t.id] = TerrainId{static_cast<std::uint32_t>(rs.hexTerrain_.size())};
      rs.hexTerrain_.push_back(convert(t));
    }
    for (const HexXml::TerrainDoc& t : doc.map.hexsideTerrain) {
      ctx.edgeTerrainByName[t.id] = EdgeTerrainId{static_cast<std::uint32_t>(rs.hexsideTerrain_.size())};
      rs.hexsideTerrain_.push_back(convert(t));
    }
    return;
  }

  void
  RuleSetBuilder::buildNetworks(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::NetworkDoc& n : doc.map.networks) {
      ctx.networkByName[n.id] = NetworkId{static_cast<std::uint32_t>(rs.networks_.size())};
      Network out;
      out.id = n.id;
      out.carries = resolvePurposes(n.carries);
      out.mutableP = n.mutableFlag;
      rs.networks_.push_back(out);
    }
    return;
  }

  void
  RuleSetBuilder::buildLayers(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    ctx.regionByNamePerLayer.assign(doc.map.regionLayers.size(), {});
    for (std::size_t i = 0; i < doc.map.regionLayers.size(); ++i) {
      const HexXml::RegionLayerDoc& l = doc.map.regionLayers[i];
      ctx.layerByName[l.id] = LayerId{static_cast<std::uint32_t>(i)};
      RegionLayerSpec spec;
      spec.id = l.id;
      spec.partitionP = l.partition;
      spec.boundedBy = l.boundedBy ? std::optional<EdgeTerrainId>(resolveOne(ctx.edgeTerrainByName, *l.boundedBy,
                                                                              "hexside terrain"))
                                    : std::nullopt;
      spec.mutableP = l.mutableFlag;
      for (std::size_t r = 0; r < l.regions.size(); ++r) {
        ctx.regionByNamePerLayer[i][l.regions[r].id] = RegionId{static_cast<std::uint32_t>(r)};
        spec.regionIds.push_back(l.regions[r].id);
        spec.regionNames.push_back(l.regions[r].name);
        spec.regionSides.push_back(resolveSideMask(ctx.sideByName, l.regions[r].side));
      }
      rs.layers_.push_back(std::move(spec));
    }
    return;
  }

  void
  RuleSetBuilder::buildSpaces(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::SpaceDoc& s : doc.map.spaces) {
      SpaceSpec spec;
      spec.id = s.id;
      spec.name = s.name;
      spec.kind = s.kind;
      spec.sides = resolveSideMask(ctx.sideByName, s.side);
      spec.returnsP = s.returnsFlag.value_or(false);
      rs.spaces_.push_back(std::move(spec));
    }
    return;
  }

  namespace {

    PhaseNode
    mintPhase(const HexXml::PhaseDoc& p, RuleSetBuildContext& ctx)
    {
      PhaseNode node;
      node.id = PhaseId{static_cast<std::uint32_t>(ctx.phaseByName.size())};
      ctx.phaseByName[p.id] = node.id;
      node.name = p.name;
      node.sides = resolveSideMask(ctx.sideByName, p.side);
      node.turns = parseTurnsOr(p.turns, TurnSelector::all());
      node.condition = p.condition;
      node.optionalP = p.optionalFlag;
      for (const HexXml::PhaseDoc& c : p.children) {
        node.children.push_back(mintPhase(c, ctx));
      }
      return node;
    }

  }  // namespace

  void
  RuleSetBuilder::buildPhases(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::PhaseDoc& p : doc.phases) {
      rs.phases_.push_back(mintPhase(p, ctx));
    }
    rs.phaseByName_ = ctx.phaseByName;
    return;
  }

  void
  RuleSetBuilder::buildStacking(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    rs.stacking_.units = doc.stacking.units;
    rs.stacking_.steps = doc.stacking.steps;
    rs.stacking_.enforced = doc.stacking.enforced;
    rs.stacking_.repair = doc.stacking.repair;
    rs.stacking_.exempt = resolveMany(ctx.unitTypeByName, doc.stacking.exempt, "unit type");
    return;
  }

  void
  RuleSetBuilder::buildZocs(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::ZocDoc& z : doc.zocs) {
      ZocSpec spec;
      spec.id = z.id;
      spec.sides = resolveSideMask(ctx.sideByName, z.side);
      spec.range = parseExtent(z.range);
      spec.projectedBy = resolveMany(ctx.unitTypeByName, z.projectedBy, "unit type");
      spec.blockedBy = resolveMany(ctx.edgeTerrainByName, z.blockedBy, "hexside terrain");
      spec.negatedByFriendlyP = z.negatedByFriendly;
      spec.stopsMovementP = z.stopsMovement;
      spec.zocToZocForbiddenP = z.zocToZocForbidden;
      spec.mandatoryAttackP = z.mandatoryAttack;
      spec.blocks = resolvePurposes(z.blocks);
      rs.zocs_.push_back(std::move(spec));
    }
    return;
  }

  RuleSet
  RuleSetBuilder::build(const HexXml::RulesDoc& doc)
  {
    RuleSet rs;
    BuildContext ctx;
    rs.gameId_ = doc.id;

    buildSides(rs, doc, ctx);
    buildUnitTypes(rs, doc, ctx);
    buildTerrain(rs, doc, ctx);
    buildNetworks(rs, doc, ctx);
    buildLayers(rs, doc, ctx);
    buildSpaces(rs, doc, ctx);
    buildPhases(rs, doc, ctx);
    buildStacking(rs, doc, ctx);
    buildZocs(rs, doc, ctx);
    buildRandomizers(rs, doc, ctx);
    buildMovement(rs, doc, ctx);
    buildSupply(rs, doc, ctx);
    buildCombat(rs, doc, ctx);
    buildRetreat(rs, doc, ctx);
    buildWeather(rs, doc, ctx);
    buildVictory(rs, doc, ctx);
    buildProse(rs, doc, ctx);

    return rs;
  }

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
