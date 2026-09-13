// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/PhaseCursor.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace HexEngine {

  namespace {

    using HexModel::SideId;

    std::vector<std::optional<SideId>>
    actingSides(const HexRules::PhaseNode& node, const std::optional<SideId>& inherited)
    {
      if (!node.sides.any()) {
        return {inherited};
      }
      std::vector<std::optional<SideId>> sides;
      for (std::size_t i = 0; i < node.sides.size(); ++i) {
        if (node.sides.test(i)) {
          sides.push_back(SideId{static_cast<std::uint32_t>(i)});
        }
      }
      return sides;
    }

    void
    walk(const HexRules::PhaseNode& node, const std::optional<SideId>& inherited, int turn,
          const PhaseCursor::ActiveFilter& activeP, std::vector<PhaseCursor::Stop>& out)
    {
      if (!node.turns.containsP(turn)) {
        return;
      }
      if (activeP && !activeP(node.id)) {
        return;
      }
      for (const std::optional<SideId>& side : actingSides(node, inherited)) {
        if (node.children.empty()) {
          out.push_back(PhaseCursor::Stop{turn, node.id, side});
        } else {
          for (const HexRules::PhaseNode& child : node.children) {
            walk(child, side, turn, activeP, out);
          }
        }
      }
      return;
    }

  }  // namespace

  PhaseCursor::PhaseCursor(const HexRules::RuleSet& rules) : rules_(rules)
  {
  }

  std::vector<PhaseCursor::Stop>
  PhaseCursor::turnStops(int turn, const ActiveFilter& activeP) const
  {
    std::vector<Stop> out;
    for (const HexRules::PhaseNode& root : rules_.phases()) {
      walk(root, std::nullopt, turn, activeP, out);
    }
    return out;
  }

  PhaseCursor::Stop
  PhaseCursor::first(int turn, const ActiveFilter& activeP) const
  {
    for (int t = turn; t < turn + kTurnSearch; ++t) {
      const std::vector<Stop> stops = turnStops(t, activeP);
      if (!stops.empty()) {
        return stops.front();
      }
    }
    throw std::invalid_argument("PhaseCursor: no active phase in turns " + std::to_string(turn) +
                                 " to " + std::to_string(turn + kTurnSearch - 1));
  }

  PhaseCursor::Stop
  PhaseCursor::next(const Stop& current, const ActiveFilter& activeP) const
  {
    const std::vector<Stop> stops = turnStops(current.turn, activeP);
    const auto here = std::find(stops.begin(), stops.end(), current);
    if (stops.end() == here) {
      throw std::invalid_argument("PhaseCursor: the position's phase is not a stop of turn " +
                                   std::to_string(current.turn));
    }
    const auto following = here + 1;
    if (stops.end() != following) {
      return *following;
    }
    return first(current.turn + 1, activeP);
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
