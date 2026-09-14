// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Retreat, stacking, victory, the phase gate, and the set that owns one of each.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace HexEngine {

  namespace {

    bool
    namesP(const std::string& haystack, const char* needle)
    {
      return std::string::npos != haystack.find(needle);
    }

    void
    collectCaps(const HexRules::PhaseNode& node, std::vector<PhaseCaps>& out)
    {
      if (out.size() <= node.id.value) {
        out.resize(node.id.value + 1);
      }
      PhaseCaps caps;
      if (namesP(node.name, "Movement") || namesP(node.name, "Move")) {
        caps.set(static_cast<std::size_t>(Cap::Move));
        caps.set(static_cast<std::size_t>(Cap::RailMove));
        caps.set(static_cast<std::size_t>(Cap::Reinforce));
      }
      if (namesP(node.name, "Combat")) {
        caps.set(static_cast<std::size_t>(Cap::Combat));
      }
      if (namesP(node.name, "End")) {
        caps.set(static_cast<std::size_t>(Cap::Supply));
        caps.set(static_cast<std::size_t>(Cap::Admin));
      }
      if (namesP(node.name, "Weather")) {
        caps.set(static_cast<std::size_t>(Cap::Weather));
      }
      if (caps.none()) {
        caps.set(static_cast<std::size_t>(Cap::Admin));
      }
      caps.set(static_cast<std::size_t>(Cap::Decision));
      out[node.id.value] = caps;
      for (const HexRules::PhaseNode& child : node.children) {
        collectCaps(child, out);
      }
      return;
    }

  }  // namespace

  SideMask
  enemiesOf(const HexRules::RuleSet& rules, SideId side)
  {
    bool declaredP = false;
    for (std::size_t i = 0; i < rules.sides().size(); ++i) {
      if (rules.hostility().enemiesOf(SideId{static_cast<std::uint32_t>(i)}).any()) {
        declaredP = true;
      }
    }
    if (declaredP) {
      return rules.hostility().enemiesOf(side);
    }
    SideMask all;
    for (std::size_t i = 0; i < rules.sides().size(); ++i) {
      all.set(i);
    }
    all.reset(side.value);
    return all;
  }

  // ---- retreat -----------------------------------------------------------------------------------

  AdjacentRetreat::AdjacentRetreat(const HexRules::RuleSet& rules, const ZocPolicy& zoc)
    : rules_(rules), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  AdjacentRetreat::claims() const
  {
    return {};
  }

  std::vector<HexIndex>
  AdjacentRetreat::candidates(const Ctx& ctx, UnitId unit, HexIndex origin) const
  {
    const SideId side = ctx.roster.unit(unit).side;
    const SideMask enemies = enemiesOf(ctx.rules, side);
    const std::vector<EdgeTerrainId> blockedBy =
        rules_.retreat() ? rules_.retreat()->blockedBy : std::vector<EdgeTerrainId>{};

    std::vector<HexIndex> out;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      const std::optional<HexIndex> to = ctx.board.neighbour(origin, direction);
      if (!to) {
        continue;
      }
      bool blockedP = false;
      for (EdgeTerrainId edge : ctx.board.edge(origin, direction)) {
        const HexRules::Terrain& terrain = rules_.hexsideTerrain()[edge.value];
        if (terrain.blocks.test(static_cast<std::size_t>(Purpose::Retreat)) ||
            blockedBy.end() != std::find(blockedBy.begin(), blockedBy.end(), edge)) {
          blockedP = true;
        }
      }
      if (blockedP) {
        continue;
      }
      const HexRules::Terrain& terrain = rules_.hexTerrain()[ctx.board.terrain(*to).value];
      if (std::holds_alternative<HexModel::Prohibited>(terrain.moveCost)) {
        continue;
      }
      bool enemyHeldP = false;
      for (UnitId occupant : ctx.position.unitsAt(*to)) {
        if (enemies.test(ctx.roster.unit(occupant).side.value)) {
          enemyHeldP = true;
        }
      }
      if (enemyHeldP) {
        continue;
      }
      if (zoc_.blockedForP(ctx, *to, side, Purpose::Retreat)) {
        continue;
      }
      out.push_back(*to);
    }
    return out;
  }

  RetreatFate
  AdjacentRetreat::fate(const Ctx&, UnitId) const
  {
    return RetreatFate::Walk;
  }

  // ---- stacking ----------------------------------------------------------------------------------

  CountStacking::CountStacking(const HexRules::RuleSet& rules) : rules_(rules)
  {
  }

  std::span<const std::string_view>
  CountStacking::claims() const
  {
    return {};
  }

  std::vector<UnitId>
  CountStacking::excess(const Ctx& ctx, HexIndex hex) const
  {
    const HexRules::StackingSpec& spec = rules_.stacking();
    if (!spec.units) {
      return {};
    }
    std::vector<UnitId> counted;
    for (UnitId unit : ctx.position.unitsAt(hex)) {
      const UnitTypeId type = ctx.roster.unit(unit).type;
      if (spec.exempt.end() != std::find(spec.exempt.begin(), spec.exempt.end(), type)) {
        continue;
      }
      counted.push_back(unit);
    }
    const std::size_t limit = static_cast<std::size_t>(*spec.units);
    if (counted.size() <= limit) {
      return {};
    }
    // The units placed last, newest first: what arrived over the limit is what has to go.
    std::vector<UnitId> over(counted.begin() + static_cast<std::ptrdiff_t>(limit), counted.end());
    std::reverse(over.begin(), over.end());
    return over;
  }

  // ---- victory -----------------------------------------------------------------------------------

  std::span<const std::string_view>
  DefaultVictory::claims() const
  {
    return {};
  }

  std::optional<Outcome>
  DefaultVictory::check(const Ctx&) const
  {
    return std::nullopt;
  }

  // ---- phases ------------------------------------------------------------------------------------

  DefaultPhaseGate::DefaultPhaseGate(const HexRules::RuleSet& rules)
  {
    capsByPhase_.resize(rules.phaseIds().size());
    for (const HexRules::PhaseNode& root : rules.phases()) {
      collectCaps(root, capsByPhase_);
    }
  }

  std::span<const std::string_view>
  DefaultPhaseGate::claims() const
  {
    return {};
  }

  bool
  DefaultPhaseGate::activeP(const Ctx&, PhaseId) const
  {
    return true;
  }

  PhaseCaps
  DefaultPhaseGate::capsFor(PhaseId phase) const
  {
    if (capsByPhase_.size() <= phase.value) {
      throw std::invalid_argument("DefaultPhaseGate: phase index " + std::to_string(phase.value) +
                                   " outside the rules");
    }
    return capsByPhase_[phase.value];
  }

  // ---- the set -----------------------------------------------------------------------------------

  DefaultPolicySet::DefaultPolicySet(const HexRules::GameDefinition& definition, GameSteps gameSteps)
    : names_(*definition.board, *definition.roster, *definition.rules),
      zoc_(*definition.rules),
      movement_(movementFor(*definition.rules)),
      supply_(*definition.rules, zoc_),
      combat_(*definition.rules),
      retreat_(*definition.rules, zoc_),
      stacking_(*definition.rules),
      phases_(*definition.rules),
      grammar_(names_),
      state_(*definition.rules)
  {
    policies_.zoc = &zoc_;
    policies_.movement = movement_.get();
    policies_.supply = &supply_;
    policies_.combat = &combat_;
    policies_.retreat = &retreat_;
    policies_.stacking = &stacking_;
    policies_.victory = &victory_;
    policies_.phases = &phases_;
    policies_.grammar = &grammar_;
    registerEngineSteps(steps_);
    switch (gameSteps) {
      case GameSteps::Required:
        break;
      case GameSteps::Withheld:
        withholdUnregistered(steps_, *definition.rules, grammar_, "the engine's default policy set runs no game module");
        break;
    }
    policies_.steps = &steps_;
    policies_.state = &state_;
    policies_.obligationCodec = &obligationCodec_;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
