// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "Tactics.h"

#include "hexsearch/Search.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>

namespace HexAi {

  namespace {

    constexpr int kUnreached = -1;
    constexpr long long kFarthest = std::numeric_limits<long long>::max();

    // |aA + bB + cC|^2 for unit vectors 120 degrees apart: A.B = B.C = C.A = -1/2. Unchanged by
    // adding one number to all three, as a point's name must be.
    long long
    squaredLength(HexCoord::Abc v)
    {
      const long long a = v.a();
      const long long b = v.b();
      const long long c = v.c();
      return a * a + b * b + c * c - a * b - b * c - c * a;
    }

  }  // namespace

  Tactic
  tacticNamed(std::string_view name)
  {
    if ("nearest" == name) {
      return AttackNearest{};
    }
    if ("consolidate" == name) {
      return AttackOrConsolidate{};
    }
    if ("seize" == name) {
      return SeizeLocations{};
    }
    throw std::invalid_argument("HexAi: no tactic named '" + std::string(name) + "'");
  }

  LocalForce
  localForce(const Board& board, HexIndex at, int radius, std::span<const Piece> friends, std::span<const Piece> enemies)
  {
    LocalForce force;
    for (const Piece& piece : friends) {
      force.friendly += board.distance(at, piece.hex) <= radius ? piece.strength : 0.0;
    }
    for (const Piece& piece : enemies) {
      force.enemy += board.distance(at, piece.hex) <= radius ? piece.strength : 0.0;
    }
    return force;
  }

  bool
  favourableP(const LocalForce& force, double ratio)
  {
    return 0.0 == force.enemy || force.friendly >= ratio * force.enemy;
  }

  Rally
  rallyOf(const Board& board, HexIndex at, int radius, std::span<const Piece> friends)
  {
    Rally rally;
    for (const Piece& piece : friends) {
      if (board.distance(at, piece.hex) <= radius) {
        rally.sum = rally.sum + board.centre(piece.hex).abc();
        rally.count += 1;
      }
    }
    return rally;
  }

  Goal
  goalFor(const Board& board, const Tactic& tactic, const Piece& me, std::span<const Piece> friends,
          std::span<const Piece> enemies)
  {
    if (enemies.empty()) {
      return Hold{};
    }
    if (const AttackOrConsolidate* consolidate = std::get_if<AttackOrConsolidate>(&tactic)) {
      const LocalForce force = localForce(board, me.hex, consolidate->attackRadius, friends, enemies);
      if (!favourableP(force, consolidate->attackRatio)) {
        return rallyOf(board, me.hex, consolidate->consolidateRadius, friends);
      }
    }
    return CloseWithEnemy{};
  }

  EnemyField::EnemyField(const Board& board, std::span<const HexIndex> enemyHexes, const Crossable& crossableP)
    : steps_(board.hexCount(), kUnreached)
  {
    std::vector<HexSearch::NodeIndex> sources;
    for (HexIndex hex : enemyHexes) {
      sources.push_back(hex.value);
    }
    const HexSearch::HexAdjacencyGraph graph(board, crossableP);
    HexSearch::SearchScratch scratch;
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        graph, scratch, std::span<const HexSearch::NodeIndex>(sources), [](HexSearch::NodeIndex, std::size_t) { return true; },
        static_cast<int>(board.hexCount()));
    for (HexSearch::NodeIndex node : field.reached()) {
      steps_[node] = field.distance(node).halves;
    }
    return;
  }

  std::optional<int>
  EnemyField::steps(HexIndex hex) const
  {
    return kUnreached == steps_[hex.value] ? std::nullopt : std::optional<int>(steps_[hex.value]);
  }

  std::optional<long long>
  goalDistance(const Board& board, const EnemyField& field, const Goal& goal, HexIndex hex)
  {
    if (std::holds_alternative<Hold>(goal)) {
      return std::nullopt;
    }
    if (const Rally* rally = std::get_if<Rally>(&goal)) {
      return squaredLength(board.centre(hex).abc() * rally->count - rally->sum);
    }
    const std::optional<int> steps = field.steps(hex);
    return steps ? static_cast<long long>(*steps) : kFarthest;
  }

  std::optional<std::size_t>
  chooseDestination(const Board& board, const EnemyField& field, const Goal& goal, const HexEngine::Reachability& reach,
                    HexIndex from, bool mustLeaveP, const std::function<bool(HexIndex)>& allowedP)
  {
    return chooseDestination(reach, [&](HexIndex hex) { return goalDistance(board, field, goal, hex); }, from, mustLeaveP, allowedP);
  }

  std::optional<std::size_t>
  chooseDestination(const HexEngine::Reachability& reach, const std::function<std::optional<long long>(HexIndex)>& distance,
                    HexIndex from, bool mustLeaveP, const std::function<bool(HexIndex)>& allowedP)
  {
    const long long here = distance(from).value_or(kFarthest);
    std::optional<std::size_t> best;
    std::tuple<long long, int, std::uint32_t> bestKey{};
    for (std::size_t i = 0; i < reach.hexes.size(); ++i) {
      if (!allowedP(reach.hexes[i])) {
        continue;
      }
      const long long there = distance(reach.hexes[i]).value_or(kFarthest);
      if (!mustLeaveP && here <= there) {
        continue;
      }
      const std::tuple<long long, int, std::uint32_t> key{there, reach.costs[i].halves, reach.hexes[i].value};
      if (!best || key < bestKey) {
        best = i;
        bestKey = key;
      }
    }
    return best;
  }

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
