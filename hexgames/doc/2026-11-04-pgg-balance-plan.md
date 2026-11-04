Copyright Ben Paul Wise. All Rights Reserved.

# Plan: a balance test of Panzergruppe Guderian with two simple AIs, 1000 games on 10 threads

Status: proposal, not built. Ben answered questions 1 to 5 on 2026-11-04 (section 7). No code is
written by this plan. The code it describes goes in titan-alloy `hexgames`; this file is kept beside
the PGG example data in HexRules.

## 1. Goal

1. Play a complete, unintelligent game of PGG with the hexgames engine, using the two scripted
   tactics of HexKrieg:
   - **attack nearest** (`HexKrieg/src/AttackNearestMove.java`);
   - **attack or consolidate** (`HexKrieg/src/AttackOrConsolidateMove.java`), which attacks
     when the local force ratio is favourable and otherwise closes up on nearby friends.
2. Play 1000 such games, each one a single task, 10 at a time on 10 threads, and report the
   distribution of the result (victory points and level of victory).
3. Prove that the games do not interact: every game's result must depend only on its own task
   (its seed and its two players), and not on which thread ran it, which games ran beside it, or
   in which order the tasks were taken.

What the result means: a balance figure obtained with weak players measures the game together
with those players. It is useful for comparing variants of the rules or the scenario with the same
players, and for finding a gross imbalance; it does not say how the game plays between people.

## 2. What exists

From a survey of hexgames (branch `10-map-generator`) on 2026-11-01. File references are relative
to `titan-alloy/hexgames`.

**The engine is built for this.**
- `Session(shared_ptr<const GameDefinition>, const Policies&, Position, seed)`
  (`hexengine/Session.h:52`) is one game. `apply(Command)` advances it and throws
  `std::invalid_argument` on an illegal command, leaving the position unchanged.
- `prompt()` gives the turn, phase, side to act, whether a decision is pending, and `overP`.
- `CLAUDE.md` requires that the Board, RuleSet, Roster and policies are immutable and shared const,
  with no globals, no static mutable state and no `thread_local`. A survey of hexengine, hexrules,
  hexmodel, hexsearch, hexrecord, games/pgg and HexRules' `cpp/` found none. The only statics are
  function-local `static const` tables, which C++ initialises once and safely.
- `hexengine/test/ParallelRolloutTest.cpp` already runs 16 TRC sessions on 16 threads and checks
  that each matches its serial run. Nothing of the kind exists for PGG.

**PGG is complete enough to finish a game.**
- `Pgg::loadPackage` and `Pgg::PggPolicySet` load and run the game; the start position is
  `games/pgg/scenario/pgg-1941.xml` (174 units; the Germans start off the map).
- Twelve turns. Victory is declared by the rules step `turn-victory` after turn 12
  (`PggVictory::declare`), which emits a `victory-points` event and a `VictoryDeclared` event and
  makes `prompt().overP` true. The levels run from Soviet Decisive (0 or less) to German Decisive
  (125 or more); the historical result was a German Marginal Victory (50 to 79).
- The task file `tasks/11-pgg-engine.md` is still at `status: review`.

**What does not exist.**
- No AI player. `RandomPlayer` cannot play PGG: it never attacks, places or reinforces.
- `legalCommands()` lists only the answers to a pending decision and `EndPhase`. Moves, attacks,
  placements and reinforcements are not listed, so a player must build them itself, using
  `reachable(units, mode)` and `attackTargets(attackers)`, and must expect that PGG's own checks
  may still refuse some of them.
- No batch runner, no `Session` digest (only `Position::digest()`), and no use of the random
  streams `Player0` to `Player3`.

## 3. Design

The design keeps three things apart, as HexKrieg does: the *game* (the session), the *tactic*
(a parameterised object, HexKrieg's `HKMove`), and the *runner* (which plays many games).

### 3.1 The game task: a pure function

```
GameResult playGame(const Shared& shared, const GameTask& task);
```

- `Shared` holds what every game reads and none changes: the `shared_ptr<const GameDefinition>`,
  the start `Position` (copied by each task, never used directly), and one `PggPolicySet`.
- `GameTask` is a value: the task index, the game seed, the German tactic and its parameters, the
  Soviet tactic and its parameters, and a command limit.
- `playGame` creates its own `Session`, its own two players and its own search scratch, plays
  until `overP` or the command limit, and returns a `GameResult` value. It does not use
  `HexEngine::play`, which returns as soon as the prompt names no side; a phase with no side
  (TRC's sudden-death check) is ended by `playGame` itself with `EndPhase`. Nothing it creates
  outlives the call, and it writes to nothing but its return value.
- `GameResult` holds: the task index and seed; German VP, Soviet VP, their difference and the
  level of victory; the turn reached; the number of commands; the number of commands refused;
  whether the game finished; the fingerprint (section 4.1); and the time taken.

Each game's seed is `mixSeed(baseSeed, taskIndex)` (`hexmodel/Seed.h`), so a run is named by one
base seed, and any single game can be replayed alone from its index.

**Reading the final score.** After `turn-victory`, the clock moves on to turn 13 and that turn's
entry steps still run. The result is therefore read from the `victory-points` event (through an
`EventSink` attached to the session), not recomputed from the final position.

### 3.2 The players: two tactics as `HexEngine::Player`

Both tactics implement `HexEngine::Player::choose(const Session&, SideId)`. The game-neutral part
(where to go, whether to attack) is separate from the PGG part (which phase allows what, and how
to answer PGG's decisions), so the same tactics could later play TRC.

**What each player sees.** A player sees the position through `PositionView`, so an enemy unit
that is not yet revealed is not known by its true strength. PGG's `Units::attackOf` and
`defenceOf` return the true values whatever the unit's revealed flag says; the players must not
call them on an unrevealed enemy. An unrevealed enemy is counted at the mean strength of the
counters of its type that could be under it (from the roster), as Ben decided (question 3). A
counter's id also names its printed strengths (for example `s1-110-inf-1-1-6`), so the players
never read strengths from an id.

**Decisions taken in a fixed order.** In each phase the player works through its own units in
unit-id order, which fixes the result without any random draw. Ties (equal distances, equal odds)
are broken by hex index, then unit id. Each player still owns an `std::mt19937_64` seeded by
`mixSeed(gameSeed, Player0)` or `Player1`, for any later tactic that needs one. A player cannot
draw from the session's streams, which keeps the game's dice independent of the tactics.

**Movement phases (both tactics).** For each unit not yet moved:
1. Choose a target hex.
   - *Attack nearest:* the nearest enemy-held hex, by movement cost (one multi-source
     `dijkstraBounded` from all visible enemy hexes per phase, as HexKrieg's `Router.enemyField`).
   - *Attack or consolidate:* sum the attack strengths of friendly units and of enemy units within
     `attackRadius` hexes. If there is no enemy within the radius, or friendly is at least
     `attackRatio` times enemy, the target is the nearest enemy. Otherwise the target is the
     centre of mass of the friendly units within `consolidateRadius`, computed in ABC coordinates
     (the mean of the hex centres as `Abc` points) and snapped to the nearest hex the unit may
     enter. HexKrieg averages offset row and column here, which is not the same thing and is
     not ported.
   - With no visible enemy on the map, the unit stays where it is, as in HexKrieg, unless the
     first-turn rules below require it to move.
2. Take `reachable({unit}, mode)` and move to the reached hex nearest the target, by hex distance
   (as `Router.routeTo`). If that hex is the unit's own, it does not move.
3. If `apply` refuses the move (PGG checks that the session's search does not, such as rail
   capacity), try the next nearest reached hex, at most three times, then leave the unit.

The defaults are HexKrieg's Main window: `attackRadius` 3, `attackRatio` 1.0,
`consolidateRadius` 4. HexKrieg's `horizon` has no counterpart: a PGG unit moves as far as its
movement allowance allows.

**The first Soviet Movement Phase (rules 5.0 and 5.2).** The tactics do not apply to this phase,
which has rules of its own. `PggFirstTurn` refuses `EndPhase` while a marching unit could still move.
- The 13th and 20th Armies must spend their whole allowance, moving only north, south or east, never
  re-entering a hex and never ending in hex-columns 01 or 02. A path from `reachable` need not obey
  this, so the player builds these paths itself: from the unit's hex, the legal step that goes
  farthest east, then the lowest hex index, until no legal step is left. The check is the one
  `PggFirstTurn::mayGoOnP` makes.
- An army that the die holds in place (the 16th or the 19th) moves only to clear an overstacked hex,
  by at most one hex.
- Every overstacked hex in the set-up is cleared in this phase. Otherwise the engine removes the
  excess, newest first, which would cost the Soviets units through a fault of the player.

**Combat phases.** HexKrieg's combat is automatic fire; PGG's must be declared. For each enemy hex
in `attackTargets` of the side's units, in hex-index order, gather every adjacent friendly unit
that may still attack it and compute the odds (`PggCombat::assess`, with unrevealed defenders
counted as in "What each player sees").
- *Attack nearest* attacks when the odds are at least `minOdds` (default 1-1).
- *Attack or consolidate* attacks when the local force ratio in the radius is at least
  `attackRatio` and the odds are at least `minOdds`.
The engine resolves each attack as it is declared (`Session::apply` refuses
`ResolveNextAttack`), so the player answers the decisions each attack raises before it declares
the next.

**Second German movement phase.** Mechanised units move again by the same rule.

**Reinforcements and set-up.**
- German and Leader arrivals: `place` each in the hex of its entrance area nearest to the nearest
  enemy (or, before any enemy is visible, the entrance hex nearest the map's centre).
- Soviet scheduled divisions: `reinforce <type> <where> scheduled` in the owed area, at the area's
  hex nearest the nearest German unit. The South-Western Front is not used in the first version
  (question 4).
- The turn-1 German air-interdiction set-up, and air interdiction each turn: the Soviet stack
  nearest the German units in the entrance areas, or none if the phase allows ending without one.

**Decisions.** Answered from `legalCommands()`:
- a step loss: the weakest eligible unit (lowest defence, then unit id);
- a combat result offering retreat or a loss: retreat if the retreat is possible, else the loss;
- routing an enemy's retreat (`ChooseRetreat`, answered by the opponent): the candidate hex
  farthest from the routing side's own nearest units' targets, then hex index;
- advance after combat: advance with the strongest attacker if the vacated hex is nearer its
  target, else decline.

**Not used in the first version:** overrun, Soviet rail movement, rail cuts and their repair, and
the South-Western Front. Leaving them out makes the players weaker, not the game different. The
second version adds overrun, Soviet rail movement and the South-Western Front, each as a player
parameter (question 4).

**Divisional integration.** A German division doubles its strength only when all its regiments
stack together with no other division. Neither tactic stacks regiments on purpose, so the Germans
will seldom earn the doubling. This weakens the German player, and the report states how often the
doubling applied.

### 3.3 The runner

A small, game-neutral library (`hexbatch`) and a PGG command (`pgg_balance`).

```
pgg_balance --games 1000 --threads 10 --seed <base>
            --german nearest|consolidate[:radius,ratio,cradius] --soviet ...
            [--setup fixed|drawn]
            [--order natural|reverse|shuffled:<k>] [--only <index>] [--private-definition]
            [--scripts <dir>] --out results.csv
```

- **Tasks.** The runner builds the 1000 `GameTask` values first. The list is fixed before any
  thread starts.
- **Threads.** 10 `std::jthread` workers take the next task index from one counter (the runner's
  only shared mutable state, an `std::atomic<std::size_t>`, which belongs to the runner and never
  to the engine). Each worker writes its result into the slot of a pre-sized
  `std::vector<GameResult>` with the task's index; no two workers write to the same slot, and
  no slot is read until every worker has joined. No lock is needed.
- **Order.** `--order` changes the order in which tasks are handed out, for the isolation tests.
- **Set-up.** `--setup fixed` starts every game from `pgg-1941.xml` as written. `--setup drawn`
  redraws the Soviet identities for each game (rule 12.1): within each division type, the counters
  on the map and in `soviet-pool` are shuffled, and the `army-13`, `army-16`, `army-19` and `army-20`
  lists are rewritten to match. The draw uses a generator of its own, seeded from the game seed with
  a tag the engine does not use, so it does not disturb the session's `Setup` stream.
- **Output.** After the join, results are written in task-index order, so the file is the same
  byte for byte however the tasks were scheduled. Time taken is written to a separate file, since
  it does vary.
- **Failures.** An exception in one game is caught in its worker and recorded in that game's
  result; it never stops the other games. A game that reaches the command limit is recorded as
  unfinished. Either makes the run fail.
- **Replays.** With `--scripts`, each game's command list is written as a hexsave script
  (`HexRecord::writeRecord`), so any game can be replayed alone with
  `hexgames_cli --replay`.

### 3.4 As built (2026-11-04)

Steps 1 to 4 are done on hexgames branch `12-pgg-agents`: the module `hexai`, the agents in
`games/pgg/agent` (`PggAgent`), and a command `pgg_play` that plays games one after another. The runner
(section 3.3) is not built. These differ from section 3.2, or settle what it left open:

- **Nearness to the enemy is in hexes,** through any hexside not entering a lake, not in one unit's
  movement cost as in HexKrieg.
- **What is hidden.** Every unit's place is known; only an untried division's strength is hidden, from
  both players, as the rules say (`hidden-from="all"`). So the agents read positions directly and not
  through `PositionView`.
- **Leaders** rally on the friendly units within 8 hexes and do not enter an enemy zone of control.
- **Advance after combat:** the strongest attacker advances one hex, and no further.
- **Routing a retreat:** to the candidate hex next to the most units of the routing side.
- **Air interdiction:** the westernmost Soviet hexes in reach of the markers. The Soviet marker is not
  used.
- **Refusals.** `HexAi::playOut` reports a refused move, attack or placement to the agent, which chooses
  again. A refused phase end or decision answer stops the game as a fault.
- **No scripts yet.** A game written as a hexsave script does not replay, because saves reorder the pool
  (`BUGS.txt`, 2026-11-04), so `pgg_play` prints the events instead.

**First results.** 48 whole games finished with no fault and one refused command: 12 with the fixed
set-up, 36 with the drawn set-up, across three pairings of the tactics. All but one end in a Soviet
Strategic Victory, with the Germans on 15 points in 44 of the 48. Those points are Vitebsk (10) and
Orsha (5), the two victory cities nearest the German entry; the Soviets score about 5.

As Ben expected, the agents make no effort to take or hold a place. In the one game followed closely, the
German front moves from column 7 to column 21 in twelve turns, and Mogilev (column 4) stays Soviet.

**The seize tactic (Ben, 2026-11-04)** is built, with 48 more games on the same seeds: with both sides
seizing, the mean net is 23 to 27 points and the score spreads across the levels. Its behaviour, the problems
it avoids, its results and recommendations for TRC5 are in hexgames
`doc/2026-11-04-seize-locations-tactic.md`.

## 4. Proving that the games do not interact

### 4.1 The fingerprint

`Position::digest()` covers units, control, the clock, the pending decision and PGG's state, but
not the random streams or the events. Each game therefore keeps a running fingerprint: after every
applied command, fold in (FNV-1a or SplitMix64 mixing, as `Position::digest` does)
- the command's text;
- `Position::digest()`;
- every stream's draw count (`streams().draws(tag)`);
- the text of each event produced (`TextEventEncoder::line`, as `DeterminismTest` does).

The fingerprint at the end of each turn is also kept, so a mismatch can be traced to the first
turn that differs.

### 4.2 The checks

Each check compares the per-game fingerprints and results of two runs. Any difference fails.

| # | Check | What it would catch |
|---|---|---|
| 1 | 10 threads against 1 thread (all 1000 games serially) | any dependence on concurrency |
| 2 | 10 threads, natural order, against reverse order and two shuffled orders | dependence on which games ran before or beside |
| 3 | 10 threads against 16 threads (more threads than cores) | dependence on timing |
| 4 | the same 1000 games run twice in one process | state carried from game to game within the process |
| 5 | 20 games chosen by seed, each in its own process (`--only`), against the batch | anything a fresh process does differently, such as a lazily built table |
| 6 | one shared `GameDefinition` and policy set against one loaded per task (`--private-definition`) | hidden mutation of the shared definition or policies |
| 7 | in one worker: game A, game B, then game A again | state left behind in a worker or its scratch |
| 8 | the shared `GameDefinition`'s digest (board, roster, rules), taken before and after the run | any write to the shared data |
| 9 | the fingerprints are not all equal, and the results take more than one value | a test that passes because nothing varies |

Checks 1 and 2 prove the property asked for. The others narrow down the cause if it fails.

**Tools beyond comparison.**
- A ThreadSanitizer build on the `linux-debug` preset (`-fsanitize=thread`), running 100 games on
  10 threads, must report nothing. MSVC has no ThreadSanitizer, so this check runs on Debian.
- An AddressSanitizer build of the same run, for memory errors that could pass data between
  games.
- A hygiene check (ctest label `hygiene`) that fails on any new `static` variable that is not
  `const`, any `thread_local`, or any `mutable` in the engine libraries and games/pgg, so the
  present clean state is kept.

## 5. The balance report

For each matchup (German tactic against Soviet tactic), from its 1000 games:
- the number of games at each level of victory, and the fraction with a 95% interval;
- the mean, standard deviation and range of German VP minus Soviet VP, the mean with a 95%
  interval (with 1000 games the interval is about 0.06 standard deviations either side);
- the separate German and Soviet totals, and how often each victory city is held at the end;
- the mean turn at which each side's strength falls below half, as a measure of how the game
  went;
- the number of commands refused, which should be near zero; a high count means a player is
  proposing moves the rules forbid;
- how often divisional integration applied, and the turn Smolensk was taken and its points in each
  game (question 6).

The first run is one matchup, attack or consolidate on both sides with HexKrieg's defaults, played
twice: 1000 games with the fixed set-up and 1000 with the drawn set-up (question 2). The full report
is the four matchups of the two tactics, then a sweep of `attackRatio` (0.5, 1.0, 1.5, 2.0) for each
side.

**Limits of the result.**
- With the fixed set-up, the seed varies the dice and the later pool draws, but every game starts
  with the same hidden units. Comparing the two set-up runs shows how much that matters.
- Pool draws use the `Setup` stream (`PggArrivals.cpp`, a `TODO(decide)`), so the number of Soviet
  reinforcements chosen changes later draws. Two matchups with the same seed do not see the same
  dice, and comparisons between matchups are by distribution, not game by game.

## 6. Steps

Each step is one commit on a new hexgames branch, staging explicit paths only.

1. **Record the plan** in hexgames `doc/` and add a RESUME HERE line to `PLAN.md`.
2. **Fingerprint.** A game-neutral `sessionFingerprint` folding as in section 4.1, with a test that
   two equal sessions agree and one different command disagrees.
3. **The tactics, game-neutral part** (`hexai`, or in `hexengine` if Ben prefers): nearest-enemy
   field, local strength sums, ABC centre of mass with snapping. Unit tests on small boards,
   including the HexKrieg cases in `HKMoveTest` (a unit with no enemy stays; a unit outnumbered
   consolidates; a ratio of 0 always attacks).
4. **The PGG players** (`games/pgg/ai`): phases, combat declaration, reinforcement, decisions, with
   tests per phase on small positions, and one test that a whole game from `pgg-1941.xml` reaches
   `VictoryDeclared` with no refused command left unhandled.
5. **The runner** (`hexbatch`) and the command `pgg_balance`.
6. **The isolation tests** as gtest (labels `pgg;long`): checks 1, 2, 4, 6, 7, 8 and 9 on 40 games
   with 4 threads, so they run in a few minutes. The full 1000-game form of the same checks is a
   script under `tools/`, run by hand.
7. **The sanitizer runs** on Debian (section 4.2).
8. **The first balance run** and its report (`doc/` in hexgames, with the CSV), sent to Ben.

## 7. Decisions

Ben's answers of 2026-11-04:

1. **Where the tactics live.** A new module `hexai` (game-neutral tactics, usable by TRC later),
   depending on `hexengine` and `hexsearch` only.
2. **The Soviet set-up.** Both, as two runs: the fixed draw in `pgg-1941.xml`, and a draw per game
   from the seed.
3. **Hidden information.** An unrevealed enemy counts at the mean strength of its possible counters.
4. **Options left out.** Overrun, Soviet rail movement and the South-Western Front are left out of
   the first version and added in the second, each as a player parameter.
5. **The pool-draw stream.** Pool draws stay on `Setup`.

6. **The review state of PGG (awaiting Ben).** Ben asked whether the balance tests can run before
   all 14 open questions of `tasks/11-pgg-engine.md` are settled. They can: none of the 14 stops a
   game from running. The task file is out of date, and most of them are already closed:

   | Question | Subject | State | Effect on a balance run |
   |---|---|---|---|
   | 1 | Combat results the game settles; choices another side answers | Accepted 2026-09-14 | None |
   | 2 | Markers kept in `PggState` | Option B chosen; milestone M6h not built | None expected |
   | 3 | The turn-1 `set-up` phase | Accepted 2026-09-14 | None |
   | 4 | Order of the reveal events | Open | None: the odds already use the true values |
   | 5 | Map data gaps | Closed: `PggPackageTest` asserts that none remain | None |
   | 6, 7, 8 | Owner's choices; one target a battle; one move a unit a phase | Accepted 2026-09-14 | None |
   | 9 | A committed 0-1-6 withdraws (10.23) | Open | Small: changes play in a few battles |
   | 10 | First-turn readings (5.22, 5.23) | Open | Small: turn 1 only, but it fixes the Soviet positions the game grows from |
   | 11 | When divisional integration doubles (7.3) | Open | Small with these players, which seldom earn it |
   | 12 | Smolensk's points fall from the turn it was taken (15.11) | Open | Largest on the score, none on play |
   | 13 | Dice on the `Setup` stream | Answered above | None |
   | 14 | Supply events only for units out of supply | Open | None: log only |

   Four questions could change a result: 9, 10, 11 and 12. **Question 12 needs no rerun.** The players
   ignore victory points, so a different reading changes the score but not the play, and it can be
   recomputed from each game's recorded turn and points. Smolensk is worth up to 25 points, and the
   levels are about 24 to 45 points wide, so this reading can move a game by one level. Questions 9,
   10 and 11 change play, so settling them against the present code means a rerun.

   Recommendation: run now, state the engine commit in the report, and settle 9, 10, 11 and 12 once
   the report shows whether they matter.

   **A larger risk than the 14:** no test plays PGG past turn 1 or reaches `VictoryDeclared` on the
   real scenario. The first whole game (section 6, step 4) is likely to find faults the 14 do not
   name, so it comes before the runner, and each fault is reported rather than worked around in a
   player.

Copyright Ben Paul Wise. All Rights Reserved.
