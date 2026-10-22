// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The second half of RuleSetBuilder: randomizers, movement, supply, combat, retreat, weather,
// victory and the flattened prose rules. Split from RuleSetBuilder.cpp per the house style's "small
// files"; both halves share RuleSetBuildContext.h and RuleSetBuilderDetail.h.
// ----------------------------------------------
#include "hexrules/RuleSetBuildContext.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexrules/RuleSetBuilderDetail.h"

namespace HexRules {

  using Detail::parseExtent;
  using Detail::parseRounding;
  using Detail::parseTurnsOr;
  using Detail::resolveMany;
  using Detail::resolveOne;
  using Detail::resolveSideMask;
  using Detail::toTable;

  void
  RuleSetBuilder::buildRandomizers(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::RandomizerDoc& r : doc.randomizers) {
      ctx.randomizerByName[r.id] = RandomizerId{static_cast<std::uint32_t>(rs.randomizers_.size())};
      RandomizerSpec spec;
      spec.id = r.id;
      spec.kind = r.kind;
      spec.size = r.size;
      spec.sides = resolveSideMask(ctx.sideByName, r.side);
      spec.fields = r.fields;
      rs.randomizers_.push_back(std::move(spec));
    }
    return;
  }

  namespace {

    Mode
    toMode(const HexXml::ModeDoc& m, const RuleSetBuildContext& ctx)
    {
      Mode out;
      out.id = m.id;
      out.name = m.name;
      out.sides = resolveSideMask(ctx.sideByName, m.side);
      out.units = resolveMany(ctx.unitTypeByName, m.units, "unit type");
      out.phases = resolveMany(ctx.phaseByName, m.phase, "phase");
      out.network = m.network ? std::optional<NetworkId>(resolveOne(ctx.networkByName, *m.network, "network"))
                               : std::nullopt;
      out.randomizer = m.randomizer
                            ? std::optional<RandomizerId>(resolveOne(ctx.randomizerByName, *m.randomizer,
                                                                      "randomizer"))
                            : std::nullopt;
      return out;
    }

  }  // namespace

  void
  RuleSetBuilder::buildMovement(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    rs.movement_.budget = doc.movement.budget;
    for (const HexXml::ModeDoc& m : doc.movement.modes) {
      rs.movement_.modes.push_back(toMode(m, ctx));
    }
    return;
  }

  void
  RuleSetBuilder::buildSupply(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::TraceDoc& t : doc.supply.traces) {
      Trace out;
      out.id = t.id;
      out.sides = resolveSideMask(ctx.sideByName, t.side);
      out.sources = t.sources;
      out.maxLength = parseExtent(t.maxLength);
      out.blockedByZocP = t.blockedByZoc;
      out.threshold = t.threshold;
      out.fatalP = t.fatal;
      out.checked = t.checked;
      for (const HexXml::SegmentDoc& s : t.segments) {
        Segment seg;
        seg.order = s.order;
        seg.kind = s.kind;
        seg.length = s.length ? parseExtent(*s.length) : Extent{Unlimited{}};
        seg.network = s.network ? std::optional<NetworkId>(resolveOne(ctx.networkByName, *s.network, "network"))
                                 : std::nullopt;
        seg.layer =
            s.layer ? std::optional<LayerId>(resolveOne(ctx.layerByName, *s.layer, "layer")) : std::nullopt;
        out.segments.push_back(std::move(seg));
      }
      rs.traces_.push_back(std::move(out));
    }
    return;
  }

  void
  RuleSetBuilder::buildCombat(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    rs.combat_.mandatoryP = doc.combat.mandatory;
    for (const HexXml::ResolverDoc& r : doc.combat.resolvers) {
      Resolver out;
      out.id = r.id;
      out.kind = r.kind;
      out.randomizer = r.randomizer ? std::optional<RandomizerId>(resolveOne(ctx.randomizerByName, *r.randomizer,
                                                                              "randomizer"))
                                     : std::nullopt;
      out.minOdds = r.minOdds;
      out.maxOdds = r.maxOdds;
      out.rounding = parseRounding(r.rounding);
      for (const HexXml::TableAnnex& t : r.tables) {
        out.tables.push_back(toTable(t));
      }
      rs.combat_.resolvers.push_back(std::move(out));
    }
    for (const HexXml::ModifierDoc& m : doc.combat.modifiers) {
      Modifier out;
      out.id = m.id;
      out.appliesTo = m.appliesTo;
      out.kind = m.kind;
      out.value = m.value;
      out.cap = m.cap;
      out.sides = resolveSideMask(ctx.sideByName, m.side);
      out.phases = resolveMany(ctx.phaseByName, m.phase, "phase");
      rs.combat_.modifiers.push_back(std::move(out));
    }
    return;
  }

  void
  RuleSetBuilder::buildRetreat(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    if (!doc.retreat) {
      return;
    }
    RetreatSpec spec;
    spec.routedBy = doc.retreat->routedBy;
    spec.distance = doc.retreat->distance;
    spec.monotoneP = doc.retreat->monotone;
    spec.advanceAfterCombatP = doc.retreat->advanceAfterCombat;
    spec.unsatisfiable = doc.retreat->unsatisfiable;
    spec.blockedBy = resolveMany(ctx.edgeTerrainByName, doc.retreat->blockedBy, "hexside terrain");
    rs.retreat_ = std::move(spec);
    return;
  }

  void
  RuleSetBuilder::buildWeather(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    if (!doc.weather) {
      return;
    }
    WeatherSpec spec;
    spec.source = doc.weather->source;
    spec.randomizer = doc.weather->randomizer
                           ? std::optional<RandomizerId>(resolveOne(ctx.randomizerByName, *doc.weather->randomizer,
                                                                     "randomizer"))
                           : std::nullopt;
    spec.layer = doc.weather->layer
                     ? std::optional<LayerId>(resolveOne(ctx.layerByName, *doc.weather->layer, "layer"))
                     : std::nullopt;
    rs.weather_ = std::move(spec);
    return;
  }

  void
  RuleSetBuilder::buildVictory(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::ConditionDoc& c : doc.victory.conditions) {
      Condition out;
      out.id = c.id;
      out.sides = resolveSideMask(ctx.sideByName, c.side);
      out.kind = c.kind;
      out.turns = parseTurnsOr(c.turns, TurnSelector::all());
      out.text = c.text;
      rs.victory_.push_back(std::move(out));
    }
    return;
  }

  void
  RuleSetBuilder::buildProse(RuleSet& rs, const HexXml::RulesDoc& doc, BuildContext& ctx)
  {
    for (const HexXml::RuleAnnex& r : doc.allRules) {
      ProseRule out;
      out.id = RuleId{r.id};
      out.ref = r.ref;
      out.topic = r.topic;
      out.phases = resolveMany(ctx.phaseByName, r.phase, "phase");
      out.sides = resolveSideMask(ctx.sideByName, r.sides);
      out.units = resolveMany(ctx.unitTypeByName, r.units, "unit type");
      out.turns = parseTurnsOr(r.turns, TurnSelector::all());
      out.optionalP = r.optionalFlag;
      out.text = r.text;
      out.line = r.line;
      rs.prose_.push_back(std::move(out));
    }
    return;
  }

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
