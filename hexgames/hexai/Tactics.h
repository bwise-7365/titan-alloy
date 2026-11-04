// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// HexAi -- game-neutral tactics for scripted agents, ported from HexKrieg's AttackNearestMove and
// AttackOrConsolidateMove (doc/2026-11-04-pgg-balance-plan.md). A tactic says where one unit wants to
// go; the game's agent says which units may move, what they know, and how that becomes a command.
// Attack nearest always closes with the nearest enemy. Attack or consolidate closes when the strength
// near the unit favours it, and otherwise rallies on the friendly units near it.
//
// Differences from HexKrieg: the centre of the rally is a point in ABC, not a mean of offset rows and
// columns, and nearness to the enemy is in hexes, not in the movement cost of one unit.
// ----------------------------------------------
#pragma once
#include "hexcoord/Abc.h"
#include "hexengine/Session.h"
#include "hexmodel/Board.h"

#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace HexAi {

  using HexModel::Board;
  using HexModel::Direction;
  using HexModel::HexIndex;

  struct AttackNearest {};
  struct AttackOrConsolidate {  // the defaults are HexKrieg's game window
    int attackRadius = 3;
    double attackRatio = 1.0;
    int consolidateRadius = 4;
  };
  using Tactic = std::variant<AttackNearest, AttackOrConsolidate>;
  Tactic tacticNamed(std::string_view);  // "nearest" or "consolidate"; throws naming the text

  // A unit as the deciding side sees it.
  struct Piece {
    HexIndex hex;
    double strength = 0.0;
  };

  struct Hold {};            // no hex is better than another
  struct CloseWithEnemy {};  // the fewer hexes from an enemy the better
  struct Rally {             // the nearer the centre of mass the better; the centre is sum / count
    HexCoord::Abc sum;
    int count = 0;
  };
  using Goal = std::variant<Hold, CloseWithEnemy, Rally>;

  struct LocalForce {
    double friendly = 0.0;
    double enemy = 0.0;
  };
  LocalForce localForce(const Board&, HexIndex at, int radius, std::span<const Piece> friends,
                        std::span<const Piece> enemies);
  // True with no enemy near, as in HexKrieg: a unit alone goes looking for one.
  bool favourableP(const LocalForce&, double ratio);

  Rally rallyOf(const Board&, HexIndex at, int radius, std::span<const Piece> friends);
  // The goal of the unit `me`; `friends` includes it. Hold when no enemy is on the map.
  Goal goalFor(const Board&, const Tactic&, const Piece& me, std::span<const Piece> friends,
               std::span<const Piece> enemies);

  // Hexes from the nearest enemy, flooded from the enemy's hexes through the crossable hexsides.
  class EnemyField {
  public:
    using Crossable = std::function<bool(HexIndex, Direction)>;
    EnemyField(const Board&, std::span<const HexIndex> enemyHexes, const Crossable&);
    std::optional<int> steps(HexIndex) const;  // nullopt: no enemy can be reached from here

  private:
    std::vector<int> steps_;
  };

  // How far a hex leaves the unit from its goal, in units only comparable within one goal; nullopt
  // for Hold. A hex no enemy can be reached from ranks last.
  std::optional<long long> goalDistance(const Board&, const EnemyField&, const Goal&, HexIndex);

  // The entry of `reach` to move to: the nearest the goal, then the cheapest, then the lowest hex index,
  // among the hexes allowedP accepts. Unless mustLeaveP, it must be strictly nearer the goal than `from`,
  // so that a unit moves only to gain something; under Hold only mustLeaveP moves it.
  std::optional<std::size_t> chooseDestination(const Board&, const EnemyField&, const Goal&,
                                               const HexEngine::Reachability& reach, HexIndex from, bool mustLeaveP,
                                               const std::function<bool(HexIndex)>& allowedP);

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
