// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "Seize.h"

#include "hexsearch/Search.h"

#include <algorithm>
#include <limits>
#include <set>
#include <tuple>

namespace HexAi {

  namespace {

    constexpr int kUnreached = -1;

    // The quota of each target: its share of its kind's share of the strength.
    std::vector<double>
    quotasOf(std::span<const Target> targets, double strength, double fringeShare)
    {
      double fringeWeight = 0.0;
      double rearWeight = 0.0;
      std::size_t fringeCount = 0;
      std::size_t rearCount = 0;
      for (const Target& target : targets) {
        (target.fringeP ? fringeWeight : rearWeight) += target.weight;
        (target.fringeP ? fringeCount : rearCount) += 1;
      }
      const double fringePool = strength * (0 == rearCount ? 1.0 : 0 == fringeCount ? 0.0 : fringeShare);
      const double rearPool = strength - fringePool;
      std::vector<double> quotas;
      for (const Target& target : targets) {
        const double pool = target.fringeP ? fringePool : rearPool;
        const double weight = target.fringeP ? fringeWeight : rearWeight;
        const std::size_t count = target.fringeP ? fringeCount : rearCount;
        quotas.push_back(pool * (0.0 < weight ? target.weight / weight : 1.0 / static_cast<double>(count)));
      }
      return quotas;
    }

  }  // namespace

  CostField::CostField(const Board& board, std::span<const HexIndex> sources, const StepCost& step)
    : cost_(board.hexCount(), kUnreached), source_(board.hexCount(), kUnreached)
  {
    std::vector<HexSearch::Source> starts;
    for (HexIndex hex : sources) {
      starts.push_back(HexSearch::Source{hex.value, HexSearch::CostHalves::zero()});
    }
    const HexSearch::HexAdjacencyGraph graph(board, [](HexIndex, Direction) { return true; });
    HexSearch::SearchScratch scratch;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::dijkstraBounded(
        graph, scratch, std::span<const HexSearch::Source>(starts),
        [&](HexSearch::NodeIndex node, std::size_t arc) -> std::optional<HexSearch::CostHalves> {
          // The arc leads away from a source, and a traveller comes the other way along it.
          const Direction out = static_cast<Direction>(arc);
          const std::optional<HexIndex> traveller = board.neighbour(HexIndex{node}, out);
          if (!traveller) {
            return std::nullopt;
          }
          const std::optional<int> halves = step(*traveller, HexCoord::opposite(out));
          return halves ? std::optional<HexSearch::CostHalves>(HexSearch::CostHalves{*halves}) : std::nullopt;
        },
        HexSearch::CostHalves{std::numeric_limits<int>::max() / 2}, [](HexSearch::NodeIndex) { return false; });
    for (HexSearch::NodeIndex node : field.reached()) {
      cost_[node] = field.distance(node).halves;
      const HexIndex origin{field.origin(node)};
      source_[node] = static_cast<int>(std::find(sources.begin(), sources.end(), origin) - sources.begin());
    }
    return;
  }

  std::optional<int>
  CostField::cost(HexIndex hex) const
  {
    return kUnreached == cost_[hex.value] ? std::nullopt : std::optional<int>(cost_[hex.value]);
  }

  std::optional<std::size_t>
  CostField::sourceOf(HexIndex hex) const
  {
    return kUnreached == source_[hex.value] ? std::nullopt : std::optional<std::size_t>(static_cast<std::size_t>(source_[hex.value]));
  }

  std::vector<HexIndex>
  occupiedRegions(const Board& board, std::span<const HexIndex> locations, std::span<const HexIndex> positions, const StepCost& step)
  {
    const CostField regions(board, locations, step);
    std::set<std::size_t> occupied;
    for (HexIndex position : positions) {
      if (const std::optional<std::size_t> region = regions.sourceOf(position)) {
        occupied.insert(*region);
      }
    }
    std::vector<HexIndex> out;
    for (std::size_t region : occupied) {
      out.push_back(locations[region]);
    }
    return out;
  }

  std::vector<double>
  strengthByRegion(const Board& board, std::span<const HexIndex> locations, std::span<const Piece> pieces, const StepCost& step)
  {
    const CostField regions(board, locations, step);
    std::vector<double> out(locations.size(), 0.0);
    for (const Piece& piece : pieces) {
      if (const std::optional<std::size_t> region = regions.sourceOf(piece.hex)) {
        out[*region] += piece.strength;
      }
    }
    return out;
  }

  std::vector<std::optional<std::size_t>>
  allocate(std::span<const Force> forces, std::span<const Target> targets, double fringeShare, const TargetCost& cost,
           std::span<const std::optional<std::size_t>> previous, double keepMargin)
  {
    std::vector<std::optional<std::size_t>> out(forces.size());
    if (targets.empty()) {
      return out;
    }
    double strength = 0.0;
    std::vector<std::tuple<int, std::size_t, std::size_t>> pairs;  // cost, force, target: closest first
    for (std::size_t f = 0; f < forces.size(); ++f) {
      strength += forces[f].strength;
      for (std::size_t t = 0; t < targets.size(); ++t) {
        if (const std::optional<int> halves = cost(t, forces[f].hex)) {
          pairs.emplace_back(*halves, f, t);
        }
      }
    }
    std::sort(pairs.begin(), pairs.end());
    const std::vector<double> quotas = quotasOf(targets, strength, fringeShare);
    std::vector<double> left = quotas;
    const auto hadP = [&](std::size_t force, std::size_t target) {
      return !previous.empty() && previous[force] == target;
    };
    std::vector<int> assigned(targets.size(), 0);
    const auto give = [&](std::size_t force, std::size_t target) {
      out[force] = target;
      left[target] -= forces[force].strength;
      assigned[target] += 1;
    };
    const bool fringeExistsP = std::any_of(targets.begin(), targets.end(), [](const Target& target) { return target.fringeP; });

    for (const auto& [halves, force, target] : pairs) {
      if (!forces[force].freeP && !out[force] && (targets[target].fringeP || !fringeExistsP)) {
        give(force, target);
      }
    }
    for (const auto& [halves, force, target] : pairs) {
      if (!out[force] && targets[target].fringeP && 0 == assigned[target] && hadP(force, target)) {
        give(force, target);
      }
    }
    for (const auto& [halves, force, target] : pairs) {
      if (!out[force] && targets[target].fringeP && 0 == assigned[target]) {
        give(force, target);
      }
    }
    for (const auto& [halves, force, target] : pairs) {
      if (!out[force] && hadP(force, target) && -keepMargin * quotas[target] <= left[target] - forces[force].strength) {
        give(force, target);
      }
    }
    for (const auto& [halves, force, target] : pairs) {
      if (!out[force] && 0.0 < left[target]) {
        give(force, target);
      }
    }
    for (const auto& [halves, force, target] : pairs) {
      if (!out[force]) {
        give(force, target);
      }
    }
    return out;
  }

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
