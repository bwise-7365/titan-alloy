// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The tactic of seizing locations (doc/2026-11-04-seize-locations-tactic.md): discrete Voronoi regions
// around locations, by terrain cost and ignoring enemy units; the fringe, the unheld locations whose
// regions hold a unit of the side; and the allocation of its forces among targets, closest pair first.
// The game says what a location is, who holds it, what a step costs and what a force is.
// ----------------------------------------------
#pragma once
#include "Tactics.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace HexAi {

  // What it costs to step from a hex across a hexside; nullopt where no one can go. It knows only the
  // terrain: a path may run through the enemy, who is to be fought and not avoided.
  using StepCost = std::function<std::optional<int>(HexIndex from, Direction)>;

  // The cheapest cost from each hex to the nearest source, and which source that is: the sources'
  // discrete Voronoi regions. Searched outward from the sources along the arcs reversed, so each cost is
  // that of travelling toward the source.
  class CostField {
  public:
    CostField(const Board&, std::span<const HexIndex> sources, const StepCost&);
    std::optional<int> cost(HexIndex) const;            // nullopt: no source can be reached from here
    std::optional<std::size_t> sourceOf(HexIndex) const;  // an index into the sources

  private:
    std::vector<int> cost_;
    std::vector<int> source_;
  };

  // Those of `locations` whose regions hold one of `positions`, in the order of `locations`. With the
  // regions drawn around unheld locations only, a unit's way to the location of its own region crosses no
  // other, and its way to any other location leaves its own region: this is the fringe.
  std::vector<HexIndex> occupiedRegions(const Board&, std::span<const HexIndex> locations,
                                        std::span<const HexIndex> positions, const StepCost&);

  // The strength standing in each location's region; parallel to `locations`.
  std::vector<double> strengthByRegion(const Board&, std::span<const HexIndex> locations, std::span<const Piece> pieces,
                                       const StepCost&);

  // A force is what moves together. One that is not free cannot move this phase.
  struct Force {
    HexIndex hex;
    double strength = 0.0;
    bool freeP = true;
  };

  // A location to send forces to, and its claim on them against the other targets of its kind: the enemy's
  // strength for a fringe target, the location's value for a rear one.
  struct Target {
    HexIndex hex;
    double weight = 0.0;
    bool fringeP = false;
  };

  // The target of each force; nullopt when no target can be reached from it. fringeShare of the side's
  // strength goes to the fringe and the rest to the rear, or all to the kind there is when one is empty.
  // A force that is not free counts first against the nearest fringe target. Each fringe target then gets
  // one force at least, since an empty location is the easiest to take. The rest go to their quotas in
  // order of cost, closest pair first, and any left over go to their nearest target.
  //
  // With `previous` (each force's target last time, parallel to the forces), a force stays with its target
  // rather than changing its mind each phase. It is the first choice for that target's one force, and it keeps
  // the target while that leaves the target no more than keepMargin of its quota over the quota. A target with
  // no quota, such as a city the side has now taken, keeps no one.
  using TargetCost = std::function<std::optional<int>(std::size_t target, HexIndex)>;
  std::vector<std::optional<std::size_t>> allocate(std::span<const Force>, std::span<const Target>, double fringeShare,
                                                   const TargetCost&,
                                                   std::span<const std::optional<std::size_t>> previous = {},
                                                   double keepMargin = 0.0);

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
