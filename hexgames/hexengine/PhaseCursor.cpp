// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/PhaseCursor.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace HexEngine {

  namespace {

    using HexModel::PhaseId;
    using HexModel::SideId;
    using Walked = std::pair<PhaseCursor::Stop, std::vector<PhaseCursor::Frame>>;

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
          const PhaseCursor::ActiveFilter& activeP, std::vector<PhaseCursor::Frame>& frames, std::vector<Walked>& out)
    {
      if (!node.turns.containsP(turn)) {
        return;
      }
      if (activeP && !activeP(node.id)) {
        return;
      }
      for (const std::optional<SideId>& side : actingSides(node, inherited)) {
        frames.push_back(PhaseCursor::Frame{node.id, side});
        if (node.children.empty()) {
          out.emplace_back(PhaseCursor::Stop{turn, node.id, side}, frames);
        } else {
          for (const HexRules::PhaseNode& child : node.children) {
            walk(child, side, turn, activeP, frames, out);
          }
        }
        frames.pop_back();
      }
      return;
    }

    std::vector<Walked>
    walkTurn(const HexRules::RuleSet& rules, int turn, const PhaseCursor::ActiveFilter& activeP)
    {
      std::vector<Walked> out;
      std::vector<PhaseCursor::Frame> frames;
      for (const HexRules::PhaseNode& root : rules.phases()) {
        walk(root, std::nullopt, turn, activeP, frames, out);
      }
      return out;
    }

    void
    index(const HexRules::PhaseNode& node, std::optional<PhaseId> parent, std::vector<const HexRules::PhaseNode*>& nodes,
          std::vector<std::optional<PhaseId>>& parents)
    {
      if (nodes.size() <= node.id.value) {
        nodes.resize(node.id.value + 1, nullptr);
        parents.resize(node.id.value + 1);
      }
      nodes[node.id.value] = &node;
      parents[node.id.value] = parent;
      for (const HexRules::PhaseNode& child : node.children) {
        index(child, node.id, nodes, parents);
      }
      return;
    }

    [[noreturn]] void
    notAStop(const PhaseCursor::Stop& stop)
    {
      throw std::invalid_argument("PhaseCursor: the position's phase is not a stop of turn " + std::to_string(stop.turn));
    }

  }  // namespace

  PhaseCursor::PhaseCursor(const HexRules::RuleSet& rules) : rules_(rules)
  {
    for (const HexRules::PhaseNode& root : rules_.phases()) {
      index(root, std::nullopt, nodes_, parents_);
    }
  }

  std::vector<PhaseCursor::Stop>
  PhaseCursor::turnStops(int turn, const ActiveFilter& activeP) const
  {
    std::vector<Stop> out;
    for (const Walked& walked : walkTurn(rules_, turn, activeP)) {
      out.push_back(walked.first);
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
      notAStop(current);
    }
    const auto following = here + 1;
    if (stops.end() != following) {
      return *following;
    }
    return first(current.turn + 1, activeP);
  }

  std::vector<PhaseCursor::Frame>
  PhaseCursor::lineage(const Stop& stop, const ActiveFilter& activeP) const
  {
    for (const Walked& walked : walkTurn(rules_, stop.turn, activeP)) {
      if (walked.first == stop) {
        return walked.second;
      }
    }
    notAStop(stop);
  }

  std::vector<HexModel::PhaseId>
  PhaseCursor::ancestry(HexModel::PhaseId phase) const
  {
    std::vector<HexModel::PhaseId> out;
    std::optional<HexModel::PhaseId> at = phase;
    while (at) {
      (void)node(*at);
      out.push_back(*at);
      at = parents_[at->value];
    }
    std::reverse(out.begin(), out.end());
    return out;
  }

  const HexRules::PhaseNode&
  PhaseCursor::node(HexModel::PhaseId phase) const
  {
    if (nodes_.size() <= phase.value || nullptr == nodes_[phase.value]) {
      throw std::invalid_argument("PhaseCursor: phase index " + std::to_string(phase.value) + " outside the rules");
    }
    return *nodes_[phase.value];
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
