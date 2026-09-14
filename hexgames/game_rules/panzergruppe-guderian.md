Copyright Ben Paul Wise. All Rights Reserved.

# Panzergruppe Guderian — Rules Digest for Library Design

**Source.** *Panzergruppe Guderian: The Battle of Smolensk, July 1941*, Simulations
Publications, Inc., 1976. Design: James F. Dunnigan. Primary source is the coordinator's
`pdftotext` extraction of `PGG rules counters map v1.pdf` (19 pp., a 1976 photocopy scan),
supplied page by page as `page-01.txt`..`page-08.txt` (the rules text; the remaining pages are
charts, counter sheets and the map). A hand-typed, non-OCR consolidation of the Combat Results
Table, Terrain Effects Chart and Supply summary (`crt.txt`) was used for the CRT/TEC values in
preference to the OCR, which mangles the tables badly. Also read: `amendments.txt` (Tim
Alanthwaite's "Panzergruppe Guderian II" fan variant for the Avalon Hill reprint, 2012) and
`errata.txt` (the publisher's Clarifications, Errata and Addenda). The OCR misreads German words
("Gennan" for German), rule punctuation ("0,()..6" for "0-0-6") and "Tum" for "Turn"; these are
corrected silently below. Map: `map_graphics/xml/panzergruppe-guderian.xml`. Counters:
`unit_graphics/xml/panzergruppe-guderian.xml`.

This is a summary written for someone designing a reusable C++ game library. It records
structure, not every clause. Where a rule is condensed, that is stated. Rule numbers in
parentheses refer to the 1976 rulebook; "errata:" and "amendments:" cite the two supplements.

---

## 1. Game Overview

### 1.1 Subject and scale

Panzergruppe Guderian is a regimental/divisional-level simulation of the German drive to cross
the Dnepr and take Smolensk in July 1941, on the road to Moscow (1.0). Each turn is two days of
real time; each hex is 10.5 kilometres.

| Quantity | Value |
|---|---|
| Hex | 10.5 km across |
| Game turn | two days |
| Campaign length | 12 game turns |
| Unit | German regiment or division; Soviet division or army-level leader (HQ) |
| Map extent | a flat-topped hex grid, printed ids `{col:02}{row:02}` |

### 1.2 Players

Two players, German and Soviet. The asymmetry is total, not a set of local exceptions as in TRC:
the two sides use different unit granularities, different movement networks (only the Soviet uses
rail), different supply models (German traces to a map edge, Soviet through Leaders), and a
one-sided hidden-information mechanic (Soviet units start Untried; German units never do). The
German Player has no combat units at all at the start of the game (2.0) — he enters entirely by
reinforcement.

### 1.3 Sequence of play (4.0)

Twelve game turns, each one Soviet Player-Turn followed by one German Player-Turn.

**A. Soviet Player-Turn**
1. **Movement Phase** — check Supply; move any or all units; conduct Overruns; bring in
   Reinforcements; remove German Air Interdiction Markers at the end of the Phase.
2. **Combat Phase** — Soviet units may attack adjacent German units.
3. **Disruption Removal Phase** — remove Disruption markers from Soviet units.
4. **Soviet Interdiction Phase** — on at most three turns in the game, place a Soviet
   Interdiction Marker.

**B. German Player-Turn**
1. **Initial Movement Phase** — check Supply; move units; bring in Reinforcements; conduct
   Overruns.
2. **Combat Phase** — German units may attack adjacent Soviet units.
3. **Mechanized Movement Phase** — Panzer, Mech, Motorized and Cavalry units may move again,
   including further Overruns. Infantry may not.
4. **Disruption Removal Phase** — remove Disruption markers from German units.
5. **Air Interdiction Phase** — place German Air Interdiction Markers for the following Soviet
   turn; remove any Soviet Interdiction Marker.

**C. Game-Turn indication.** After both Player-Turns, the Game-Turn marker advances.

The two-phase German Player-Turn (a second, mechanized-only movement phase after combat) is the
structural analogue of TRC's two impulses, but asymmetric: only the German side has it, only
fast unit types may use it, and there is exactly one combat phase per Player-Turn, not one per
sub-cycle.

### 1.4 Victory (15.0)

Not immediate: checked once, after Game-Turn Twelve. Each side accumulates Victory Points
through the game (German for occupying cities and listed hexes, and for the Soviet Player's use
of South-Western Front reinforcements; Soviet for eliminating German divisions and for
recapturing cities). The Soviet total is subtracted from the German total, and the result is
read off a six-band Level of Victory schedule from Soviet Decisive to German Decisive (15.2).
Errata 15.2 records that the historical outcome was a German Marginal Victory, and suggests
treating anything below German Strategic as a draw if that reading is wanted.

### 1.5 What is structurally distinctive

First, **Untried Soviet units** (12.0): every Soviet combat unit is placed face-down with its
true Attack/Defense/Movement values hidden from both players, and is flipped only at the instant
it is first committed to combat (or Overrun); some flip to reveal zero Combat Strength and are
then removed. This is a genuine fog-of-war mechanic, not merely a display convenience, and it
interacts with Leader movement, retreat, and stacking. Second, **Overrun as movement, not
combat** (6.5): a moving unit may spend three extra movement points to attack a target hex at
half strength, without ending the phase, and success lets it keep moving; failure or a partial
result stops it and can Disrupt the defender. Third, **asymmetric supply and movement
networks**: only the Soviet Player has Rail Movement and only the Soviet Player has Leaders who
coordinate supply and command radius; the German Player traces supply directly to a map edge and
has no analogous command-radius constraint on attacking.

---

## 2. Standard Elements

### 2.1 The hex grid

A single grid, printed ids of the form `{col:02}{row:02}` (four digits: two-digit column, two-
digit row), e.g. `2117` for Smolensk, `0120` for the German supply road hex on the west edge.
Both digits are needed to name a hex; there is no letter component. Hex `4015`, `2216`, `1414`,
etc. are named directly in the setup rules (5.1) — the id is a plain coordinate pair, unlike
TRC's row-letter-plus-number scheme.

### 2.2 Counters

A unit counter carries (3.4): unit-size symbol (`III` regiment, `XX` division), unit
designation, Combat/Attack/Defense Strength (a German unit shows Attack-Defense-Movement or a
single Combat Strength depending on type; a Soviet unit shows either its true values or a
question mark for Untried), and Movement Allowance.

Regiment designations combine the regiment's own number with its parent division's, `20/12`
meaning the 20th Panzer Regiment of the 12th Panzer Division (2.0). This is visible in the
counter ids of the sheet, e.g. `s2-6-3-arm-4-10` for a regiment of a division coded `6/3`.

German units never have an Untried state. Soviet units, other than Leaders, always start
Untried.

### 2.3 Zones of control (8.0)

Textbook, single-layer, symmetric. The six hexes around a unit are its ZOC. All units project a
ZOC at all times, except Disrupted units, which have none (8.11). Friendly units (not Friendly
ZOC) negate an enemy ZOC for supply and leadership-radius tracing and for retreat, but not for
movement (8.15). A unit must stop the instant it enters an enemy-controlled hex and may not
leave voluntarily (6.22, 8.13); Friendly units may leave only as a result of combat, or by
Overrun (8.13, 6.5). ZOCs extend across River hexsides but not across all-Lake hexsides (8.17) —
no other terrain blocks a ZOC. There is no stacking of effect from multiple ZOCs on one hex
(8.18).

### 2.4 Movement (6.0)

Each unit spends Movement Points from its Movement Allowance, one per Movement Phase (German
mechanized units get a second Movement Phase and a fresh allowance in it, 6.23). Points may not
be saved, banked or transferred (6.12, 6.23). Terrain costs are read from the Terrain Effects
Chart (6.7 below); moving into an enemy-controlled hex costs no extra (8.12) but ends the move.

**Rail Movement** (6.3) is Soviet-only. Up to eight combat units (each Soviet armour/mechanized
division counting as three) plus any number of Leaders may move by rail in a turn, thirty hexes,
hex-to-connected-rail-hex only, starting and ending on a Railroad hex, never through an enemy
ZOC. Terrain has no cost effect on rail movement; German Air Interdiction can raise the per-hex
cost (13.31). Units do not need to be in Supply, or within a Leader's radius, to use the full
rail bonus (6.34).

**Special Soviet first-turn restrictions** (5.2): the 16th and 19th Armies may move on turn 1
only if a die roll (1-3) permits; if not, they may shuffle at most one hex to satisfy stacking.
The 13th and 20th Armies must spend their full Movement Allowance on turn 1, may not end the
phase in the westernmost two hexrows, may not re-enter a hex already visited, may move only
north/south/east, and may not use rail movement that turn.

**Special Soviet movement restriction, turns 1-6** (6.4): no Soviet unit may voluntarily or
involuntarily enter the westernmost two hexrows; a unit forced to retreat there is eliminated
instead; Soviet units may still trace Supply and Leadership radius through those hexrows.

### 2.5 Overrun (6.5)

During (and only during) a Movement Phase, the Phasing Player may Overrun an adjacent
enemy-occupied hex: move the participating units, all from one hex, adjacent to the target; pay
three extra Movement Points; halve the Attack Strength (after all other modifiers, fractions
dropped) and resolve combat immediately using the CRT. If the target hex is vacated, the
Overrunning units must move into it at no further cost and may continue moving (and Overrunning
again) if they have Movement Points left and are not in an enemy ZOC. A split or Engaged result,
or any result forcing the attacker to lose steps or retreat, halts the Overrunning unit(s) for
the rest of the phase (6.52). Overrunning units ignore the ZOC of units influencing the hex they
Overrun *from*, but not other enemy ZOCs (6.56). Overrun units may not Advance After Combat
(6.57, 9.88) — the vacated-hex entry is free movement, not an advance. A Soviet unit that begins
the Movement Phase outside a Leader's radius may not Overrun (6.55). Soviet Leaders may be
Overrun like any unit but may not Overrun by themselves, having no Attack Strength (6.53).

### 2.6 Disruption (6.6)

A unit **defending** against an Overrun that suffers any loss or retreat (not an Engaged result)
becomes Disrupted: it may not attack or move and projects no ZOC, but defends normally (6.61,
6.62). Disruption is removed automatically in the owner's next Disruption Removal Phase (6.63).
Disruption applies only to Overrun, never to ordinary combat. A Disrupted Soviet Leader may not
function as a Leader (coordinate supply, aid attacks) but still fights as a combat unit (6.64).

### 2.7 Stacking (7.0)

The Soviet Player may have at most three combat units in a hex at the end of his Movement Phase
and throughout any Combat Phase, or up to four if at least one is a Leader (7.1). The German
Player is capped at three combat units under the same timing (7.2). Informational markers never
count. Excess units at the enforcement points are eliminated, the owner's choice.

**German Divisional Integration** (7.3): if every regiment of a German Panzer or Motorized
Infantry Division is stacked together, with no unit from any other division present, the
stack's Combat Strength is doubled for both attack and defense. Each Panzer Division has one
Panzer and two Panzer Grenadier regiments; Motorized Infantry Divisions have two regiments,
except Das Reich, which has three. German Infantry and Cavalry Divisions never qualify. Losing
any regiment of the division permanently disqualifies it from the bonus (until the eliminated
regiment is regrouped/rebuilt, if the rules elsewhere provide for that — not found in the text
supplied).

### 2.8 Discrete terrain types (6.7, from `crt.txt`)

| Terrain | Movement | Combat |
|---|---|---|
| Clear | 1 MP | — |
| Forest (Woods) | Infantry 1 MP, all others 2 MP | Defender x2 |
| River hexside | Germans +2 MP, Soviets +1 MP to cross | Defender x2 if attacked solely across rivers |
| Major City | 1 MP | Defender x2 |
| Minor City | 1 MP | — |
| Swamp | 2 MP | May trace supply into, not through |
| Lake hexside | may not cross | no combat allowed |
| Road hex | Infantry (not motorized) and Cavalry 1 MP, all others 1/2 MP | German supply must trace along roads (11.13, corrected from the misprint 11.14 in errata) |
| Railroad hex | depends on other terrain in the hex (6.3) | no effect |

Terrain effects are cumulative by addition of multiples minus one (9.33): e.g. Forest (x2)
attacked across a River (x2) gives x3, not x4.

### 2.9 Supply (11.0)

Checked at the start of each Movement Phase (for movement) and at the instant of combat or
Overrun (for combat).

**German supply** (11.1): a German unit is in supply if it is within twenty hexes of a road that
eventually reaches the road hex `0120` on the west map edge, uninterrupted by enemy units, enemy
ZOCs or impassable terrain (Friendly units negate enemy ZOC for this purpose); or if it can trace
twenty movement points, by its own type's costs, directly to any west-edge hex. Roads are
considered to cross rivers for supply tracing even though normal movement does not use roads to
cross rivers (11.13, corrected by errata to read "11.13" rather than the rulebook's misprinted
cross-reference "11.14"; errata further clarifies the river-crossing cost still applies to the
20-movement-point trace even where a road is present).

**Soviet supply** (11.2): a Soviet combat unit must trace a Line of Communications, no longer
than the coordinating Leader's Radius, to a Leader; that Leader must in turn trace a Line of
Supply of any length to the east map edge. Neither line may pass through enemy units, enemy
ZOCs, or impassable terrain (Friendly units again negate enemy ZOC). Soviet units may not trace
directly to the east edge, bypassing a Leader (11.23). Leaders are not automatically supplied —
they need their own line to the east edge, but do not need to reach another Leader (11.24).

**Effects of being out of supply** (11.3): Movement Allowance and Combat Strength are halved,
fractions dropped, never below one. Units entering play are in supply for their first
Player-Turn regardless. Being out of supply is never itself fatal — units may remain out of
supply indefinitely.

### 2.10 Combat and the CRT (9.0)

Combat is voluntary and occurs only in the Phasing Player's Combat Phase, between adjacent units,
attacker-vs-defender fixed by who is phasing (not by battlefield posture). All units in a target
hex are attacked as one Defense Strength; a defending stack cannot be split (9.21). Every
attacking unit must be adjacent to every defending unit it is grouped against (9.23), and no unit
may fight, or be fought, more than once per Combat Phase (9.14).

Odds are the ratio of total Attack Strength to total Defense Strength, reduced and **rounded in
the defender's favour** (9.4) — the same convention TRC uses. Combat Results Table, six rows (die
1-6) by ten odds columns (1-3 up to 10-1; attacks worse than 1-3 or better than 10-1 are treated
as the nearest end column, per `crt.txt`).

| Die | 1-3 | 1-2 | 1-1 | 2-1 | 3-1 | 4-1 | 5-1 | 6-1 | 7-1 | 8-1 | 9-1 | 10-1 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | A1 | D1*/A1 | D1* | D2* | D2* | D2* | D2* | De/A1 | De | De | De | De |
| 2 | A1 | Eng | D1*/A1 | D1* | D2* | D2* | D2* | D2* | De/A1 | De | De | De |
| 3 | A1 | A1 | D1*/A1 | D1*/A1 | D1* | D2*/A1 | D2* | D2* | D2* | De/A1 | De | De |
| 4 | A2 | A1 | Eng | D1*/A1 | D1*/A1 | D1* | D2*/A1 | D2* | D2* | D2* | De/A1 | De |
| 5 | Ae | A2 | A1 | Eng | D1*/A1 | D1*/A1 | D1* | D2*/A1 | D2* | D2* | D2* | De |
| 6 | Ae | Ae | A2 | A1 | Eng | Eng | D1*/A1 | D1* | D2*/A1 | D2* | D2* | D2* |

(`crt.txt` renders the 7-1 through 10-1 columns tightly; the ten-column shape above is
transcribed as printed, confirmed against `page-08.txt`'s parallel — badly OCR'd — copy of the
same table; the page image was not needed to resolve any doubtful value because `crt.txt` is a
clean typed transcription, not OCR.)

**Results** (9.6, `crt.txt`): `Ae`/`De` all attacking/defending units eliminated, opponent may
advance one or two hexes (the first hex must be the vacated one). `A1`/`A2`, `D1`/`D2` the
affected side chooses to lose that many steps from any one unit, or retreat the whole stack that
many hexes (never both). `Eng` (Engaged) each side loses one step and stays in place, no retreat
or advance. Split results (e.g. `D1*/A1`) resolve the defender's result first, then the
attacker's; only the attacker may then advance, and only if the defending hex was vacated
(9.66). The `*` marks a Defender result earned during an **Overrun**: the defender becomes
Disrupted in addition to any loss or retreat (`crt.txt`; consistent with 6.61).

**Steps** (9.61-9.63): Soviet units are always one step (eliminated on any loss). German Panzer,
Motorized, Mech and Cavalry units are two steps, the second printed on the counter's reverse.
German Infantry Divisions are four steps: `9-7`, `4-7`, `2-7`, `1-7`.

### 2.11 Retreat and advance (9.7, 9.8)

Retreats are always optional in place of a step loss (9.71, 9.65) and are routed by the *opposing*
Player, even for split results (9.72). A unit may not retreat through an enemy unit or enemy ZOC
unless the hex holds a Friendly unit, nor across a Lake hexside. Retreat prefers a vacant hex; if
none is available it may pass through a Friendly-occupied hex, but a retreat that would violate
stacking eliminates the excess unit instead (9.73). A unit retreated onto a Friendly-occupied hex
adds nothing to that hex's defense, and is automatically eliminated if that hex later suffers any
combat result (9.75).

Victorious units may Advance After Combat into the vacated hex or along the enemy's Path of
Retreat, ignoring enemy ZOCs, stopping wherever they choose, immediately and before other combat
is resolved; they may not then attack or be attacked that phase (9.81-9.85). If an entire
defending stack is eliminated, the advance may be up to two hexes, the first of which must be the
vacated hex (9.86). Advance After Combat never applies to Overrun (9.88, 6.57) — Overrun's
vacated-hex entry is ordinary movement.

### 2.12 Soviet Leaders (10.0)

A Leader counter represents an army-level logistics and command organisation. Its printed rating
serves three purposes at once (10.1): the **Radius**, in hexes, within which Soviet combat units
must trace a Line of Communications to be in supply and to be eligible to attack; the maximum
**Combat Points** it may add to units stacked directly with it; and its own **Defense Strength**.

Leaders move like Motorized units on roads but like Infantry in Forest, with a Movement Allowance
of ten; they may use rail by themselves and do not count against the eight-unit rail cap (10.21).
They project a ZOC and are treated as ordinary combat units except that they have no Attack
Strength (10.22). A Leader may enter an enemy ZOC only accompanied by a regular attack-capable
unit; if an Untried unit it entered with is later revealed to be one of the zero-strength types
(`0-0-6`, `0-1-0`, `0-1-6`), the Leader must retreat back the way it came, or is eliminated if it
cannot (10.23). A Leader counts as one full step for combat losses and may be chosen to absorb a
step loss for its stack (10.24). Leaders coordinate supply for any number of units, with no cap
(10.32); Rail Movement's thirty-hex bonus does not need a Leader or supply at all (10.33). A
Leader stacked directly with an attacking unit may add its full Leadership Value to that unit's
attack, never exceeding the unit's own (post-modifier) Strength, and the bonus may not be split
across units (10.36). Optionally (10.4), the Soviet Player may evacuate one surrounded Leader per
turn, twice per game, off the map, returning later through Entrance Hex `X`.

### 2.13 Untried units (12.0)

Every Soviet combat unit — set-up or reinforcement — is placed Untried, chosen by type from the
counter pool without either player knowing its actual Strength, and stays that way until it is
first committed to combat or Overrun, at which instant (and not before) it is flipped for both
players to see (12.1, 12.2). Units that reveal as zero-Attack/zero-Defense — `0-0-6` — are removed
from play at that instant; two specific `0-1-6` Rifle Divisions remain in play but may only ever
defend (never attack) and, if forced into an attack before revelation, must retreat like a Leader
under Case 10.23 (12.3). While Untried, a zero-strength unit still projects a ZOC and blocks
supply and retreat lines. Eliminated Untried units return, still Untried, to a "dead pile" the
Soviet Player may draw further reinforcements from once the counter pool proper is exhausted
(12.4) — the counter-count ceiling is absolute regardless of source.

### 2.14 Interdiction (13.0)

**German Air Interdiction** (13.1-13.3): three markers, no ZOC or combat value, placed during the
German Air Interdiction Phase (and once, before the game, for the first Soviet turn). Placement
is limited to no farther east than hexrow 4000 unless the German Player holds both Smolensk
hexes and has traced a Line of Communications to the west edge, one turn after taking the city;
losing Smolensk reverts the restriction. On a Railroad hex a marker adds four points to the Rail
Movement cost; on any other hex it adds one Movement Point for Soviet units entering. All markers
are removed at the end of the following Soviet Movement Phase; they never affect Soviet supply
tracing.

**Soviet Interdiction** (13.4): on at most three turns (never the last), the Soviet Player may
place one Interdiction Marker anywhere, uncounted against stacking, at the end of his Player-Turn.
It has the same movement-cost effect on German units as the German marker has on Soviet units, for
the duration of both German Movement Phases, and it also blocks German supply tracing through its
hex. It may not be placed on a German-occupied hex, and is removed at the end of the German
Player-Turn.

### 2.15 Reinforcement (14.0)

**Soviet Provisional Reinforcements** (14.1): each turn, one die roll selects one of six numbered
Entrance Hexes/Areas; one Untried Rifle Division arrives there, in addition to the scheduled
reinforcements. If the rolled area is fully German-occupied or ZOC'd, the unit enters at the
nearest open hex toward the east edge instead. These may not be saved or accumulated.

**Soviet South-Western Front Reinforcements** (14.2): from turn 2, the Soviet Player may
voluntarily divert up to five Rifle Divisions per turn, ten for the whole game, entering on any
southern-edge Railroad hex at or east of Entrance Hex `Z`. Each of the first five costs the German
Player one Victory Point; each of the next five costs two — fifteen total if all ten are used
(errata corrects the rulebook's cross-reference here from "13.23" to "14.23"). No Leader
accompanies them.

**Scheduled Reinforcements** (14.3, detailed in 16.0) arrive on a fixed turn, in a named
hex/area, and may be delayed at the owning Player's discretion but not accelerated.

---

## 3. Unique Exceptions

### 3.1 Rail cut (errata 6.37, "ADDITION TO AH VERSION")

Not in the original SPI rulebook text supplied, but present in the counter mix (a `DIS` marker's
reverse, and the game's own note in the resume line, shows "Out-of-Supply/RAIL CUT" markers) and
formalised by the AH-reprint errata: a German combat unit moving through a Soviet Railroad hex may
cut the line, marked with a Rail Cut marker (six provided); Soviet Rail Movement may not use a cut
hex; the marker is removed only by a Soviet combat unit entering it during ordinary (non-rail)
movement, and repaired rail may carry Rail Movement again starting the following turn. The German
Player may also voluntarily move a unit back over his own cut to free the marker for reuse
elsewhere.

### 3.2 The corps attachment code

German regiments' ids embed their parent division (`20/12` = 20th Panzer Regiment / 12th Panzer
Division, 2.0), and reinforcement groups arrive as named Panzer Corps carrying a fixed list of
divisions (16.2, e.g. "39th Panzer Corps (7th Pz, 12th Pz, 14th Mot Inf, 20th Mot Inf, 20th Pz
Divs.)"). This is an authored grouping list, not a computed one — a corps is simply the set of
division ids scheduled to enter together on one turn in one Entrance Area — and it recurs on the
counter sheet as SS regiment codes bundling a parent division abbreviation into the id (`4(DE)`,
`3(DR)`, `2(GR)` for Das Reich / Grossdeutschland-family formations).

### 3.3 Army HQs as "(n)*10" Soviet units

The Soviet Leader counters are printed with a Leadership Value and a fixed Movement Allowance of
ten (10.21), which the counter sheet's ids encode as `s1-<leader>-<n>th-army-hq-N-10` — the
army-level headquarters is simultaneously a combat unit (10.22), a supply/command-radius source
(10.1, 10.31), and a Victory-relevant target the German Player may eliminate or force to retreat
(10.23, 10.24). A library should not model a Leader as a pure marker; it needs Attack-less combat
participation plus the three-fold Radius/Combat-Points/Defense-Strength rating in one object,
exactly as `unit-type kind="leader"` with an attached rating does in `hexrules.xsd`.

### 3.4 Untried Soviet reveal as a first-class state transition

Untried units are not simply "face-down counters" — the moment of reveal is pinned precisely to
the instant of combat or Overrun (12.2), the two zero-strength outcomes have different fates
(`0-0-6` removed outright; `0-1-6` kept but defend-only with a Leader-style forced retreat, 12.3),
and revealed-dead units re-enter a separate recycling pool with priority ahead of running out of
the counter mix entirely (12.4). This needs a genuine tri-state per unit (Untried / Revealed /
Removed), with the reveal event itself carrying rule force (it can trigger an immediate forced
retreat under 10.23), not just a rendering flag.

### 3.5 Overrun disruption vs. ordinary combat disruption

Disruption (6.6) is scoped narrowly: it can only be inflicted on a *defender* in an *Overrun*
(never in ordinary combat, and never on the attacker), and it is marked by the `*` suffix on the
relevant CRT results in `crt.txt` rather than being a separate CRT outcome code. A library that
treats the CRT as a pure lookup table needs a side channel — "this cell, only when the attack was
an Overrun, also disrupts the defender" — layered on top of the ordinary result codes.

### 3.6 German Divisional Integration doubling order (amendments 9.35, 11.37; errata is silent)

The fan-variant amendments spell out an order of operations the base rulebook leaves implicit:
Divisional Integration doubling is applied **before** terrain multipliers and before the Overrun
or out-of-supply halving (9.35, 11.37) — the same "operations do not commute, so the order is part
of the rule" lesson TRC's combat-supply halving teaches (see TRC digest 2.6). Case 6.55's own
worked example (a Panzer Division worth 8, doubled for integration to 16, then halved for
out-of-supply to 8, then halved again for Overrun to 4) is consistent with this order and is taken
as authoritative even where the amendments restate it.

### 3.7 Optional bidding for side, and a step-loss Victory Point variant

The amendments add two optional systems worth flagging for the library even though neither is
enabled by default: **secret VP bidding for who plays the German** (amendments 15.3), and a
finer-grained **per-step-loss Victory Point award** replacing the flat per-division award of the
base rules (amendments 15.12; a separate errata variant 15.13 awards VP per step loss too, plus
minor-city control, with Yelnya as a special case). These are alternate `victory`/`condition`
scoring schemes over the same underlying event stream (division eliminated, step lost), not new
mechanics — a library's victory-condition layer should be able to select among scoring functions
without touching the event model.

---

## 4. Map and Coordinate Notes

### 4.1 Hex numbering

Grid is **flat-topped**, offset odd, printed id format `{col:02}{row:02}` — a plain two-digit
column then two-digit row, no letter component (unlike TRC's row-letter scheme). Examples: `2117`
Smolensk, `0513` Vitebsk, `4606` Gzhatsk, `3915` Vyazma, `0420` Orsha, `0424` Mogilev, `2625`
Roslavl, `5619` Kaluga, `3901` Rzhev (the nine Major Cities); `0120` the German west-edge supply
road hex; `4015`/`2216`/`1414` the Soviet 24th/16th/19th Army setup hexes (5.1).

### 4.2 Hex-centred features

- Terrain type: Clear, Forest (Woods), Swamp, Lake — drawn as sheet fills.
- Major City / Minor City: not a distinct sheet terrain fill; drawn as a `city-major` glyph (the
  nine cities of the Victory Point schedule) or a `town` glyph (four named towns: Velizh, Bely,
  Mstislavl, Krichev — the rules' "Minor City"), overlaid on the underlying (Clear) terrain.
- Railroad presence and Road presence: centre-to-centre links (network, not hex attribute — see
  4.4), so a hex's road/rail membership is derived from its incident links, not a per-hex flag.
- Setup identifiers: the initial Soviet army deployment hexes/areas (5.1) and the six Provisional
  Reinforcement entrance areas and lettered Entrance Hexes `V`,`W`,`X`,`Z` (14.1, 16.1) and German
  Entrance Areas `A` through `H` (16.2).
- Victory-point hexes named without a city glyph: `5907`, `5915` (15.11).
- Yelnya (10 Victory Points, 15.11) is hex `3122` on the original 1976 map. The Cyrillic map the
  sheet XML was traced from does not mark it, so the sheet has no glyph or name there; the rules
  XML's occupation list names the hex directly.

### 4.3 Hexside (edge) features

- **Rivers.** Drawn hexside to hexside (`line="river"` on the sheet), the rules' River hexside;
  crossing costs an extra Movement Point (Soviet +1, German +2) and, if every attacker crosses a
  River hexside, doubles the defender (9.32).
- **Lake hexsides.** Impassable to movement and to combat entirely (no attack may be resolved
  across one) — stronger than TRC's "ZOC does not extend" treatment of all-water hexsides; here
  the hexside blocks combat itself.

No blocked-hexside, coastline or district-border line types were found on the sheet (`edge
line="..."` values are only `river`, plus `rail`/`road`, which are links, not hexsides — see 4.4).
Unlike TRC and Dai Senso, PGG has only one hexside-terrain type.

### 4.4 Centre-to-centre link features

- **Railroads.** Soviet-only movement network (6.3), mutable via the rail-cut errata addition
  (6.37, see 3.1) — a German unit passing through a hex can cut the line; only ordinary Soviet
  movement (not rail movement) repairs it; repair takes effect the following turn.
- **Roads.** Used for Movement Point cost by unit type (6.7) and, distinctly, required for German
  Supply tracing (11.1, 11.13): a German unit is in supply only if a Road, not merely open
  terrain, leads to hex `0120`.

Both networks are drawn centre-to-centre on the sheet (432 links total), matching TRC's model of
the rail network as a link graph rather than a hex attribute.

### 4.5 Region features (sets of hexes)

- The **westernmost two hexrows** (`0100`, `0200`), named directly in the text as a movement- and
  retreat-forbidden region for the Soviet Player through turn 6 (6.4), and a movement-mandatory
  region for the 13th/20th Armies on turn 1 (5.23) — an authored region, not a printed layer on
  the sheet, so the rules XML must name it by column rather than by a sheet region-layer.
- **Provisional Reinforcement entrance areas** (six, numbered 1-6, die-selected, 14.1) and
  **German entrance areas** (lettered A-H, 16.2) and **Soviet entrance hexes/points** (`V`, `W`,
  `X`, `Z`, 16.1) — authored, scenario-specific, not general map regions.

### 4.6 Off-map spaces

- The Soviet "dead pile" of eliminated Untried units, from which further reinforcements may be
  drawn once the counter pool is exhausted (12.4) — a returning pool, like TRC's replacement
  pools, but populated only by combat losses, never emptied by a scheduled draw.
- No Off-Map Units Box, surrendered-units box, or air-unit boxes exist in PGG; German Air
  Interdiction and Soviet Interdiction markers are placed and removed directly on the map, not
  staged through a space.

---

## 5. Library Implications

### 5.1 Queries the rules force

| Query | Where | Notes |
|---|---|---|
| Adjacency, hex distance | ZOC, Overrun, combat, Leader Radius | ordinary |
| Bounded-length blocked path search | German supply (20 hexes/points), Soviet Line of Communications (Leader's Radius) | two different trace kinds, both `blocked-by-zoc` |
| Path search on a separate graph, to a fixed sink hex | German road-based supply to hex `0120` | sink is one named hex, not an edge or region |
| Path search on a mutable separate graph | Soviet Rail Movement, subject to Rail Cut markers | mutability event-driven (errata 6.37), not turn-scheduled like TRC's railhead |
| Region membership by raw coordinate range | the westernmost two hexrows (6.4, 5.23) | authored by column number, not a sheet layer |
| Tri-state per-unit reveal | Untried / Revealed / Removed (12.0) | reveal is an event with side effects (10.23 forced retreat) |
| Odds ratio with defender-favourable rounding | 9.4 | integer ratio reduction, same convention as TRC |
| CRT cell with a conditional side-effect | Overrun-only Disruption on defender results (6.61, `crt.txt` `*`) | not a plain lookup; needs "was this an Overrun" context |
| Ordered multiplier/divisor chain | Divisional Integration doubling before terrain multiplier before Overrun/out-of-supply halving (9.35, 11.37, 6.55) | order does not commute |
| Constraint over unit-type counts, with a bonus predicate | stacking caps (7.1, 7.2) and Divisional Integration eligibility (7.3: "every regiment of the division, no foreign unit") | integration test is a set-equality check, not a count |
| Recycling pool populated by elimination events | the Soviet "dead pile" (12.4) | distinct from TRC's scheduled replacement pools |
| Alternate scoring functions over one event stream | base per-division VP vs. amendments per-step-loss VP (15.12 vs. amendments 15.12/15.13) | victory layer should be pluggable, not hard-coded to one schedule |

### 5.2 Data structures

- **Hex attribute table**: terrain (clear/woods/swamp/lake), major-city flag, minor-city flag,
  setup/entrance codes. No river/rail/road hex flags — those are derived from incident hexside and
  link membership.
- **Edge attribute table**: river flag only (one hexside-terrain type; lake hexsides block both
  movement and combat and need no separate flag beyond the lake hex terrain itself, since combat
  cannot cross into or out of a lake hex).
- **Two link graphs**: rail (Soviet-only, mutable via cut/repair) and road (fixed, load-bearing
  for German supply as well as movement cost).
- **Unit state**: side, type, step count/level (Soviet always 1; German Panzer/Mech/Motorized/
  Cavalry 2; German Infantry 4, with named printed strengths per level), Untried/Revealed/Removed
  tri-state, Disrupted flag, current face.
- **Leader state**: Radius, Combat-Points cap, Defense Strength, all three read off one printed
  rating (10.1) — not three independent attributes to author separately.
- **Pools**: the Soviet counter mix (finite, draw-by-type), the dead pile (returning), scheduled
  reinforcement lists keyed by turn and named entrance hex/area (16.0), and the six-marker Rail Cut
  supply.
- **Turn state**: turn index (1-12), German Smolensk-control flag gating Air Interdiction
  placement range (13.2), Soviet Interdiction uses-remaining counter (max 3, never turn 12),
  South-Western Front divisions-used-this-game counter (max 10, 5/turn).

### 5.3 What this game contributes to a shared design

PGG is smaller and more asymmetric than TRC, and contributes three things a "standard elements"
library needs to support as first-class, not as PGG-only bolt-ons:

- **Overrun as a movement-phase combat variant** that can chain (further movement, further
  Overruns) on success and halts on partial failure — a different shape from TRC's
  automatic-victory-during-movement mechanic (which removes the defender outright at 10-1 with no
  further attack roll), yet structurally related: both are combat resolved inside the movement
  phase, with different unlocking effects on continued movement.
- **A tri-state hidden-unit lifecycle** (Untried/Revealed/Removed) with a reveal event that can
  itself trigger further rule consequences (forced retreat under 10.23) — this is a strictly
  richer requirement than a simple `hidden` boolean on `unit-type`.
- **Supply that traces to a single named hex**, not a region or edge (German supply's sink is
  literally hex `0120`), alongside a Leader-mediated two-segment trace (unit-to-Leader, then
  Leader-to-edge) on the Soviet side — two genuinely different `trace` shapes coexisting in one
  game, exactly the kind of case `hexrules.xsd`'s `trace`/`segment` pair was built to express.

### 5.4 Ordering hazards

- Divisional Integration doubling happens **before** terrain multiplication and **before** halving
  for Overrun or out-of-supply (9.35, 11.37, worked example at 6.55).
- Reveal of an Untried unit happens at the **instant** of combat or Overrun, not at the start of
  the Combat Phase — a preceding combat in the same phase can change what a later attack in that
  phase sees (parallel to TRC's mid-phase supply cut, 11.0 in this document, 17.0/2.6 in TRC's).
  German supply for combat purposes is likewise judged at the instant of the attack, not at the
  start of the phase.
- German Air Interdiction removal happens at the **end of the Soviet Movement Phase**, not the end
  of the whole Soviet Player-Turn — the markers are in effect for the Soviet Movement Phase only,
  even though they are placed the German turn before.
- Rail Cut repair (errata 6.37) takes effect only the turn **after** the repairing movement, not
  immediately — a one-turn latency worth encoding explicitly rather than as an instantaneous flag
  flip.

Copyright Ben Paul Wise. All Rights Reserved.
