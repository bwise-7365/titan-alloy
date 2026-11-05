Copyright Ben Paul Wise. All Rights Reserved.

# The seize-locations tactic for scripted agents

Status: built for Panzergruppe Guderian (PGG) on hexgames branch `12-pgg-agents`. The recommendations for
The Russian Campaign, Deluxe Fifth Edition (TRC5), in section 5, are not built. The balance test this
serves is in `doc/2026-11-04-pgg-balance-plan.md`.

## 1. Purpose

The aim is a minimal level of semi-competent play: a baseline that a real AI should beat. The first
agents ported HexKrieg's two tactics, attack nearest and attack or consolidate. They ignore locations, so
in 44 of 48 games the Germans scored exactly 15 points, from the two cities nearest where they enter.

Ben's idea is that seizing key locations may be the only movement tactic these games need, because
fighting follows when both sides make for the same places.

## 2. How it behaves

### 2.1 Locations and regions

The locations are PGG's Victory Point hexes, as the map draws them: thirteen, since each of Smolensk's two hexes scores. A region is a discrete Voronoi region: the hexes
nearer that location than any other. Nearness is by terrain cost, and it ignores enemy units and zones of
control, because a path may run through an enemy who is to be fought. Lakes cannot be crossed. Each side
uses one cost table, for its commonest kind of unit: motorised for the Germans, foot for the Soviets.

### 2.2 The German player

1. **Held cities.** A city is held if the Germans occupy it or were the last to occupy it (the engine's
   control of the hex).
2. **The fringe.** Draw regions around the *unheld* cities only. A city is on the fringe if a German unit
   stands in its region. Before any German unit has entered, the entrance areas of the next arrivals
   count as their positions.
3. **Force to match.** Draw regions around the fringe cities alone, which puts every Soviet unit in the
   region of some fringe city. Each city's quota of German strength is in proportion to the Soviet strength
   in its region.
4. **Allocation.** See section 2.4.

Once a city is taken it leaves the fringe, the regions are redrawn, and its units move on. Nothing stays
behind as a garrison. If the Soviets retake a city, it is unheld again and returns to the fringe.

### 2.3 The Soviet player

- **The fringe** is the German fringe plus every city the Germans hold. A city no one has occupied counts
  as Soviet, as the victory rules score it.
- **The share.** Between 1/2 and 2/3 of Soviet strength goes to the fringe, shared in proportion to the
  German strength in each city's region. The share is drawn once for each game, from the game seed, by a
  generator of its own.
- **The rest** is set out over the other cities in proportion to their Victory Points. Three hexes in the
  far east, Kaluga, 5907 and 5915, hold 60 of the 150 points.

### 2.4 The allocation

The allocation is made afresh each phase and works on *forces*. A force is a German Panzer or Motorized
division, or any other unit alone. In order:

1. A force that cannot move this phase counts against its nearest fringe city.
2. Each fringe city with no force yet gets its nearest free force.
3. The remaining forces go to cities with quota left, closest pair first.
4. Any left over go to their nearest city.

### 2.5 Moving and fighting

- **Moving.** A force moves to the reachable hex nearest its city, and only if that is nearer than where it
  stands. A division's first stack makes for the city and the others follow it, so a division split by a
  retreat or an entry comes back together. A division avoids ending in a hex with another division's units.
- **Attacking.** The attack rule of the first agents is unchanged: every enemy hex is attacked by every
  eligible adjacent unit, at 1-1 or better as far as the agent knows. An untried Soviet division counts at
  the mean strength of its arm.
- **Advancing.** After a battle, the strongest attacker for which the emptied hex is nearer its city goes
  in, and goes further only if that takes it nearer still.
- **Keeping targets.** A force stays with its city while the city's quota has room for it, up to 1/4 over the
  quota. It is also the first choice for that city's minimum force. A city with no quota, such as one just
  taken, keeps no one. In a combat phase no one can move, so nothing is reallocated.
- **Leaders** move last, after their divisions, each to the hex that brings the most units within its radius
  that no Leader already placed this phase covers. A unit in an enemy zone counts three times, since it is the
  one that can attack now. With nothing to gain in reach, a Leader makes for the centre of the uncovered units.
  It stays out of enemy zones of control. Distance is in hexes, so this estimates the Line of Communications
  (11.22), which a zone can block.
- **Unchanged rules.** The first Soviet Movement Phase (rules 5.0 and 5.2), air interdiction and the
  reinforcement entries work as before.
- **Left out** until a later version: overrun, Soviet rail movement and the South-Western Front.

## 3. Problems avoided

| Problem | What would have happened | What was done |
|---|---|---|
| A held city's region blocks the way | A unit that has just taken a city stands in that city's region, so every path "passes through another city's region". The advance stops at the first city. | Regions are drawn around unheld cities only. |
| An empty city gets no force | Allocating by enemy strength sends nobody to an undefended city, the easiest one to take. | Every fringe city gets at least one force. |
| Combat does not follow by itself | A unit stops on entering an enemy zone of control and may not leave (6.22, 8.13), and PGG does not force attacks. The two sides would sit adjacent for the rest of the game. | The attack rule stays. Seizing is the only *movement* tactic. |
| Units held in a zone use up quota | A unit that cannot move would be counted as going somewhere it cannot go. | Forces that cannot move are counted first, against their nearest fringe city. |
| Germans are off the map on turn 1 | With no German positions, there would be no fringe. | The next arrivals' entrance areas stand in for positions. |
| The defender only reacts | With a fringe defined only by what it does not hold, the Soviets would have no fringe on turn 1 and would never move to a city before it is attacked. | The Soviet fringe includes the German fringe. |
| Splitting divisions | Allocating single counters would break divisional integration, which doubles strength (7.3). | Divisions are allocated, moved and entered as one. |

## 4. Results

48 games, 12 for each pairing, on the same seeds as 48 games of the first tactics. All finished. The only
commands refused were reinforcements when the Soviet pool of that type had run out, which is correct.

Net is German points minus Soviet points. In the "Levels" column, S and G are the winning side and Dec,
Str and Mar are Decisive, Strategic and Marginal. The historical result was a German Marginal Victory, a net
of 50 to 79.

| Set-up | German | Soviet | German points | Soviet points | Net | Net range | Levels |
|---|---|---|---|---|---|---|---|
| Fixed | consolidate | consolidate | 15.9 | 5.9 | 10.0 | 5 to 26 | 11 S Str, 1 S Mar |
| Drawn | consolidate | consolidate | 15.9 | 3.8 | 12.2 | 7 to 25 | 12 S Str |
| Drawn | consolidate | nearest | 15.8 | 4.5 | 11.2 | 2 to 21 | 12 S Str |
| Drawn | nearest | nearest | 15.9 | 5.3 | 10.6 | 6 to 24 | 12 S Str |
| **Fixed** | **seize** | **seize** | **45.0** | **17.8** | **27.2** | **2 to 48** | **8 S Mar, 4 S Str** |
| **Drawn** | **seize** | **seize** | **38.3** | **15.3** | **23.0** | **-20 to 47** | **5 S Mar, 6 S Str, 1 S Dec** |
| Drawn | seize | consolidate | 59.2 | 18.7 | 40.6 | 9 to 65 | 5 G Mar, 3 S Mar, 4 S Str |
| Drawn | consolidate | seize | 35.8 | 1.7 | 34.1 | 14 to 54 | 1 G Mar, 9 S Mar, 2 S Str |

With 12 games a row, the standard error of a mean net is about 4 to 6 points, so only large differences mean
anything yet.

**What the agents did, with both sides seizing (fixed and drawn set-ups):**

| Measure | Result | For comparison |
|---|---|---|
| German attacks with an integrated division | 59% and 62% | 4% with consolidate |
| Battles a game | 78 and 72 | 74 in 2 games of consolidate |
| Soviet recapture points a game | 12.8 and 9.7 | 0 in 2 games of consolidate |
| Soviet units beyond a working Leader's radius | 53% and 54% | 67% in 2 games of consolidate |

**What this shows:**

- **The tactic works as a baseline.** With both sides seizing, cities change hands and the score spreads
  across the victory levels instead of sitting at one value.
- **The German seize is much stronger than the German consolidate,** against either Soviet tactic.
- **The Soviet seize is stronger against a seizing German** (net 23.0 against 40.6). Against a consolidating
  German it does worse than the Soviet consolidate (net 34.1 against 12.2). A likely cause, not yet checked,
  is that consolidating Soviets stay massed in the Germans' path, while seizing Soviets send part of their
  strength to the rear.
- **The Germans can be smothered.** In the one game with no German points (drawn, game 7), the Soviets
  poured onto the entry area, 99 units against 27 by turn 12. The Germans held Orsha, but with no line to the
  west edge it scored nothing.
- **This is not yet a measure of balance.** The figures describe these agents, and the options left out
  (overrun, rail) are large. The agents favour the Soviets compared with the historical result.

The tables above and in section 4.1 are from before the Victory Point hexes were corrected, and those games can no longer be
played again. Section 4.2 has the same batches on the corrected hexes.

### 4.1 Keeping targets and covering with Leaders

Two weaknesses showed in the results above: about half the Soviet units could not attack because no Leader was
near, and units changed target often. Both fixes are parameters, named in `pgg_play`, so that each one's effect
can be measured. These are 60 more games, both sides on the same tactic, on the same seeds; all finished.

The earlier table, and a figure of 62% to 65% for Soviet units changing target that I gave at first, overstated
that measure. The allocation also ran in combat phases, when no one could move, and overwrote what each unit
last aimed for. The table below is measured correctly: only units free to move count.

| Tactic | Set-up | German points | Soviet points | Net | Levels | Beyond a Leader's radius | Changing target: Soviet | German |
|---|---|---|---|---|---|---|---|---|
| `seize-first` | Drawn | 38.3 | 15.3 | 23.0 | 5 S Mar, 6 S Str, 1 S Dec | 54% | 35% | 54% |
| `seize-keep` | Drawn | 48.3 | 14.7 | 33.7 | 2 G Mar, 6 S Mar, 4 S Str | 60% | 20% | 49% |
| `seize-cover` | Drawn | 24.6 | 19.8 | 4.8 | 1 S Mar, 4 S Str, 7 S Dec | 22% | 33% | 50% |
| `seize` (both) | Drawn | 27.2 | 16.9 | 10.2 | 2 S Mar, 7 S Str, 3 S Dec | 24% | 19% | 49% |
| `seize` (both) | Fixed | 31.2 | 20.8 | 10.4 | 3 S Mar, 5 S Str, 4 S Dec | 23% | 18% | 50% |

`seize-first` reproduced its earlier games exactly, with the same fingerprints, so the fixes are the only change.

**What this shows:**

- **Covering with Leaders** more than halves the Soviet units that cannot attack, from 54% to 22%, and shifts the
  game sharply to the Soviets (net 23.0 to 4.8).
- **Keeping targets** nearly halves Soviet changes of target (35% to 20%), and on its own it helps the Germans
  (net 23.0 to 33.7). With Leaders covering, it raises the net by about 5. With 12 games a row, that is within the noise.
- **German changes of target hardly move,** staying near 50%. I have not found out why. One likely cause is that the
  German fringe changes as cities fall, while the Soviet fringe changes less.
- **The agents now favour the Soviets more,** with a mean net of about 10 against a historical 50 to 79. The Soviet
  agents have been improved and the German agents have not had the equivalent, so this says more about the agents
  than about the game.

**What I would try next:**

1. **Overrun and rail,** as planned for the second version, each as a player parameter. Overrun should do the most for
   the Germans, since it is how they get out of a zone of control and past a line.
2. **Why German targets change,** by watching games in `pgg_trc_viewer`.
3. **Attacks.** Attacks are still chosen one hex at a time in hex order, with no concentration on a chosen breakthrough.

### 4.2 On the corrected Victory Point hexes

On 2026-11-05 Ben ruled that the maps in HexRules `examples/PGG` are definitive for both engines. Smolensk now scores on 2216
(13 points) and 2217 (12), Roslavl on 2626, Mogilev on 0525, Gzhatsk on 4607, Vyazma on 4015 and Kaluga on 5921. Yelnya, which
the Cyrillic map omitted, is on the map at 3122. All twelve batches were played again, on the same seeds; all finished.

| Set-up | German | Soviet | German points | Soviet points | Net | Recapture points |
|---|---|---|---|---|---|---|
| Fixed | consolidate | consolidate | 15.0 | 5.9 | 9.1 | 0.0 |
| Drawn | consolidate | consolidate | 15.8 | 3.8 | 12.1 | 0.0 |
| Drawn | consolidate | nearest | 15.9 | 4.5 | 11.4 | 0.0 |
| Drawn | nearest | nearest | 17.8 | 5.3 | 12.4 | 0.2 |
| Fixed | seize-first | seize-first | 41.0 | 15.3 | 25.7 | 11.7 |
| Drawn | seize-first | seize-first | 35.7 | 15.2 | 20.4 | 10.8 |
| Drawn | seize-first | consolidate | 56.5 | 18.1 | 38.4 | 2.5 |
| Drawn | consolidate | seize-first | 33.2 | 1.7 | 31.6 | 0.2 |
| Drawn | seize-keep | seize-keep | 34.9 | 14.3 | 20.6 | 8.8 |
| Drawn | seize-cover | seize-cover | 28.2 | 21.0 | 7.2 | 13.5 |
| Drawn | seize | seize | 23.9 | 18.5 | 5.4 | 11.3 |
| Fixed | seize | seize | 23.5 | 17.2 | 6.2 | 10.0 |

With 12 games a row, the standard error of a mean net is about 4 to 6 points.

The run summaries are in `doc/pgg-runs/`.

## 5. Recommendations for TRC5

These come from reading the TRC engine and have not been tried. The regions, fringe and allocation in
`hexai/Seize.h` have no dependence on PGG, and they carry over. TRC5 differs in ways that matter:

- **Supply: limit movement to avoid leaving it.** This is the most important difference. In PGG a unit out of
  supply is halved. In TRC5 it is *eliminated* in its own end phase, and a trace is only 8 hexes (4 in snow) to
  a friendly city or a friendly rail chain. A rush for a city would destroy its own spearheads.
  - Compute once per phase the hexes from which a trace would succeed, by the rules' own range for the current
    weather, and allow a unit to end only in those hexes.
  - Railheads advance before supply is checked in the end phase, so a rail hex converted this turn can count.
    Start without this and add it if the advance is too slow.
  - A unit in that set can still be cut off by the enemy's move. Accept that, and report how many units are
    lost this way.
- **Mandatory attacks.** TRC5 refuses to end a combat phase while a unit owes an attack, and a unit that cannot
  make one surrenders. The agent should ask `TrcMandatory::check` before ending the phase, and the allocation
  should treat units in a zone as not free, as here.
- **Locations change with the date, and there are no points.** The targets are the sudden-death cities of the
  next check (turns 5, 11, 17 and 23), plus Berlin and Moscow at all times, since each can end the game at once.
  With no points to weigh the rear by, use equal weights, or more for Moscow and Berlin.
- **A front from the first turn.** The sides start adjacent, so most units are in zones and not free at once,
  and off-map entrance positions are not needed. Expect the early game to be decided mostly by attacks.
- **Units that must not go to the front.** Stalin and Hitler never move. In particular, Stalin must never go to
  the Off-Map box: with Moscow lost, that is an immediate Axis win. HQs and Stavka stay out of the allocation
  and follow their units, since air power needs a target within 8 hexes of them. Workers stay in their cities.
- **Rail matters more.** Without it the Russians cannot move reserves across a map this size. Add it early, one
  unit at a time, because the capacity check does not count a whole stack (a known engine fault).
- **Forces.** TRC5 has no divisional integration, so a force is a single corps. Stacking is checked at the end of
  the phase, so test room before moving.
- **The score.** With only a winner and a condition, report how many of the year's six cities each side holds at
  each check, as the TRC5 plan says.
- **Limits of any result.** The scenario has no reinforcement schedule, and 104 of the 202 counters never arrive.
  A balance figure means little until that data is entered.

## 6. Where the code is

| File | Contents |
|---|---|
| `hexai/Seize.h`, `.cpp` | Game-neutral: `CostField` (regions by terrain cost), `occupiedRegions` (the fringe), `strengthByRegion`, `allocate` |
| `hexai/Tactics.h` | `SeizeLocations`, the third tactic, with its fringe share |
| `games/pgg/agent/PggAgentSeize.cpp` | PGG's part: the cities, who holds them, the groups, the plan for a phase, the moves, the advance |
| `games/pgg/agent/PggGame.cpp` | Draws the Soviet share for a game; gathers what the agents did |
| `games/pgg/agent/PggPlay.cpp` | `pgg_play --german seize --soviet seize`, with the measures in section 4 |

Copyright Ben Paul Wise. All Rights Reserved.
