Copyright Ben Paul Wise. All Rights Reserved.

# hexmodel and hexsearch — test lists (contracts for W2 and W1)

## hexmodel (`hexmodel/test/*Test.cpp`, label `core`)
- `IdsTest.DistinctTagsDoNotConvert` — a `static_assert(!std::is_convertible_v<UnitId, HexIndex>)`
  plus ordering and equality of `Id<Tag>` and `Name<Tag>`.
- `QuantitiesTest.StepsInvariant` — `Steps(0,4)`, `Steps(5,4)` throw; `reduced()` walks 4→1→nullopt.
- `QuantitiesTest.MovementPointsHalves` — `whole(2) + half()` is 5 halves; ordering.
- `QuantitiesTest.OddsRoundsTowardTheDefender` — 3:2→1:1, 2:3→1:2, 10:1→10:1, 7:2→3:1 (TRC
  13.1: "rounded in the defender's favour"), 1:7 → `BelowMinimum` when the minimum is 1:6, 10:1 →
  `AboveMaximum` when the maximum is 9:1; `Rounding::Attacker` mirrors.
- `QuantitiesTest.TurnSelectorParses` — "5", "1-10", "11+", "3,5,7,9", "2+"; malformed "a-b" and
  "10-1" throw; `containsP` at the boundaries.
- `BoardTest.TrcBoardFromSheet` — built through `BoardBuilder` from the real TRC sheet + rules +
  package: hexCount, `indexOf("KK19")`, `neighbour(KK20, e)` is KK19, `edge(KK20, e)` contains the
  Kerch Strait hexside terrain, terrain of a known sea hex, features of Moscow (major-city), the rail
  network's link count > 0 and `connectedP` for a known rail pair.
- `BoardTest.FourBoardsBuild` — all four packages build a Board; hex counts match the sheets; every
  sheet terrain id bound; unknown id throws with the id in the message.
- `BoardTest.DaiSensoTwoGridsOneLattice` — a w/e seam pair are neighbours; `grids().size()==2`.
- `RosterTest.EveryCounterParses` — for each of the four counter sets, every unit/support/leader
  counter yields Strengths through the game's ValueLineReader; markers are excluded; "4-U" sets
  `unlimitedRangeP`; "(3)★10" (PGG army HQ) parses.
- `PositionTest.PlaceKeepsStacksInStep` — place/remove/relocate; `unitsAt` insertion order; moving
  the same unit twice never duplicates it; `remove` clears `where`.
- `PositionTest.ControlIsStoredNotDerived` — `setControl` then a unit leaves: control persists.
- `PositionTest.DigestIsCanonical` — equal positions built in different orders give equal digests;
  one step loss changes it; copying preserves it.
- `PositionTest.PendingDecisionRoundTrip` — set/get each variant alternative.

## hexsearch (`hexsearch/test/*Test.cpp`, label `core`)
- `ScratchTest.FieldOutlivingItsSearchThrows` — a Field read after a later search throws.
- `DijkstraTest.UnitCostsOnAToyBoard` — 5×5 synthetic board; distances equal `Board::distance`
  when every arc costs one; `pathTo` returns a shortest path.
- `DijkstraTest.CeilingAndTerminals` — nodes beyond the ceiling unreached; a terminal node is
  reached but not expanded (the "must stop on entry" hex).
- `DijkstraTest.MultiSourceWithIndividualBudgets` — two sources, different `spent`; origins correct.
- `AStarTest.FindsTheSamePathAsDijkstra` — on random costed toy boards, A* with `hexDist` heuristic
  returns a path of equal cost.
- `BfsFloodTest.DepthBounded` — reached set equals `disc(seed, depth)` when nothing blocks.
- `ComponentsTest.EdgeBlockedComponents` — a wall of blocked hexsides splits the toy board in two.
- `ReachCountTest.StopsAtThreshold` — Tarawa's two-target rule: returns 2 as soon as two targets are
  reached; returns 1 when one is walled off.
- `MonotoneWalkTest.EachStepStrictlyFarther` — every returned end hex is at distance `steps` from
  the origin; a hex that is not farther is never entered.
- `RegionFloodTest.BoundedByWalls` — a ring of wall hexsides confines the flood.
- `NetworkGraphTest.TrcRailReachability` — on the real TRC board: a known city pair is connected by
  rail; removing one link (predicate) disconnects it.

Copyright Ben Paul Wise. All Rights Reserved.
