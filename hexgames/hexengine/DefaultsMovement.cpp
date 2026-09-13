// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The three movement budgets over one shared reading of terrain, and the engine's default reading
// of a printed value line.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

namespace HexEngine {

  namespace {

    std::vector<std::string>
    splitOnDashes(std::string_view line)
    {
      std::vector<std::string> tokens;
      std::string current;
      for (char c : line) {
        if ('-' == c) {
          tokens.push_back(current);
          current.clear();
        } else if ('(' != c && ')' != c) {
          current += c;
        }
      }
      tokens.push_back(current);
      return tokens;
    }

    bool
    numberP(const std::string& text)
    {
      return !text.empty() &&
             std::all_of(text.begin(), text.end(), [](unsigned char c) { return 0 != std::isdigit(c); });
    }

    // The unit type's entry restrictions, read off the hex or hexside terrain it is entering.
    bool
    excludedP(const std::vector<UnitTypeId>& list, UnitTypeId type)
    {
      return !list.empty() && list.end() == std::find(list.begin(), list.end(), type);
    }

    bool
    exceptedP(const std::vector<UnitTypeId>& list, UnitTypeId type)
    {
      return list.end() != std::find(list.begin(), list.end(), type);
    }

  }  // namespace

  HexModel::Strengths
  defaultValueLine(std::string_view line, HexModel::UnitKind)
  {
    const std::vector<std::string> tokens = splitOnDashes(line);
    HexModel::Strengths out;
    if (2 == tokens.size() && numberP(tokens[0]) && "U" == tokens[1]) {
      out.attack = HexModel::Strength{std::stoi(tokens[0])};
      out.defence = out.attack;
      out.unlimitedRangeP = true;
      return out;
    }
    if (3 == tokens.size() && numberP(tokens[0]) && numberP(tokens[1]) && numberP(tokens[2])) {
      out.attack = HexModel::Strength{std::stoi(tokens[0])};
      out.defence = HexModel::Strength{std::stoi(tokens[1])};
      out.allowance = HexModel::MovementPoints::whole(std::stoi(tokens[2]));
      return out;
    }
    if (2 == tokens.size() && numberP(tokens[0]) && numberP(tokens[1])) {
      // One printed combat factor used in attack and defence, then the movement allowance.
      out.attack = HexModel::Strength{std::stoi(tokens[0])};
      out.defence = out.attack;
      out.allowance = HexModel::MovementPoints::whole(std::stoi(tokens[1]));
      return out;
    }
    if (1 == tokens.size() && numberP(tokens[0])) {
      out.attack = HexModel::Strength{std::stoi(tokens[0])};
      out.defence = out.attack;
      return out;
    }
    return out;  // a counter that prints words rather than factors carries no strengths
  }

  TerrainMovement::TerrainMovement(const HexRules::RuleSet& rules) : rules_(rules)
  {
  }

  std::span<const std::string_view>
  TerrainMovement::claims() const
  {
    return {};
  }

  MovementPoints
  TerrainMovement::printedAllowance(const Ctx& ctx, UnitId unit)
  {
    const UnitSpec& spec = ctx.roster.unit(unit);
    const std::optional<Budget>& printed = spec.front.allowance;
    if (!printed) {
      throw std::invalid_argument("TerrainMovement: counter '" + spec.counter.text +
                                   "' prints no movement allowance");
    }
    if (const MovementPoints* points = std::get_if<MovementPoints>(&*printed)) {
      return *points;
    }
    if (const HexCount* hexes = std::get_if<HexCount>(&*printed)) {
      return MovementPoints::whole(hexes->value);
    }
    return MovementPoints::whole(std::get<Actions>(*printed).value);
  }

  EntryVerdict
  TerrainMovement::enter(const Ctx& ctx, UnitId unit, HexIndex from, Direction direction, ModeId) const
  {
    const EntryVerdict prohibited{HexModel::Prohibited{}, false};
    const std::optional<HexIndex> to = ctx.board.neighbour(from, direction);
    if (!to) {
      return prohibited;
    }
    const UnitTypeId type = ctx.roster.unit(unit).type;

    MovementPoints cost = MovementPoints{0};
    bool mustStopP = false;

    for (EdgeTerrainId edge : ctx.board.edge(from, direction)) {
      const HexRules::Terrain& terrain = rules_.hexsideTerrain()[edge.value];
      if (terrain.blocks.test(static_cast<std::size_t>(Purpose::Movement))) {
        return prohibited;
      }
      if (excludedP(terrain.enterOnly, type)) {
        return prohibited;
      }
      if (std::holds_alternative<HexModel::Prohibited>(terrain.moveCost)) {
        return prohibited;
      }
      if (const MovementPoints* points = std::get_if<MovementPoints>(&terrain.moveCost)) {
        cost = cost + *points;
      }
      if (terrain.stopP && !exceptedP(terrain.stopExcept, type)) {
        mustStopP = true;
      }
    }

    const HexRules::Terrain& hex = rules_.hexTerrain()[ctx.board.terrain(*to).value];
    if (hex.blocks.test(static_cast<std::size_t>(Purpose::Movement))) {
      return prohibited;
    }
    if (excludedP(hex.enterOnly, type)) {
      return prohibited;
    }
    if (std::holds_alternative<HexModel::Prohibited>(hex.moveCost)) {
      return prohibited;
    }
    if (const MovementPoints* points = std::get_if<MovementPoints>(&hex.moveCost)) {
      cost = cost + *points;
    } else if (std::holds_alternative<HexModel::Entire>(hex.moveCost)) {
      cost = cost + printedAllowance(ctx, unit);
      mustStopP = true;
    } else if (std::holds_alternative<HexModel::OtherTerrain>(hex.moveCost)) {
      // "other-terrain": the hex is a feature drawn over ordinary ground, so it costs what ordinary
      // ground costs, which is one point in every game in view.
      cost = cost + MovementPoints::whole(1);
    }
    if (hex.stopP && !exceptedP(hex.stopExcept, type)) {
      mustStopP = true;
    }

    // A feature drawn on the hex (a city over clear ground) may name its own terrain; the dearer
    // reading wins, so a river hex is never cheaper than the plain it sits on.
    for (const HexModel::Feature& feature : ctx.board.features(*to)) {
      const HexRules::Terrain& drawn = rules_.hexTerrain()[feature.terrain.value];
      if (drawn.stopP && !exceptedP(drawn.stopExcept, type)) {
        mustStopP = true;
      }
    }

    return EntryVerdict{cost, mustStopP};
  }

  Budget
  PointCostMovement::allowance(const Ctx& ctx, UnitId unit, ModeId) const
  {
    return printedAllowance(ctx, unit);
  }

  Budget
  HexCountMovement::allowance(const Ctx& ctx, UnitId unit, ModeId) const
  {
    return HexCount{printedAllowance(ctx, unit).halves / 2};
  }

  Budget
  ActionMovement::allowance(const Ctx& ctx, UnitId unit, ModeId) const
  {
    return Actions{printedAllowance(ctx, unit).halves / 2};
  }

  std::unique_ptr<MovementPolicy>
  movementFor(const HexRules::RuleSet& rules)
  {
    const std::string& budget = rules.movement().budget;
    if ("points" == budget) {
      return std::make_unique<PointCostMovement>(rules);
    }
    if ("hexes" == budget) {
      return std::make_unique<HexCountMovement>(rules);
    }
    if ("actions" == budget) {
      return std::make_unique<ActionMovement>(rules);
    }
    throw std::invalid_argument("movementFor: unknown movement budget '" + budget + "'");
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
