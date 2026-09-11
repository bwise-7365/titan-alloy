# D-Day at Tarawa — Rules Digest for Library Design

**Source.** *D-Day at Tarawa*, John Butterfield, Decision Games, 2014.
Primary source: `DDAT-Rules_V13.pdf` (rules booklet, 32 pp.), with corrections
from `DDaTarawaAddenda111616.pdf` (11/16/2016 addenda to the first printing)
applied throughout. Charts taken from `DDaT_Panel.pdf`; phase summaries checked
against `DDaT_Flipbook_v1.3.pdf`. Maps: `map-1.jpg` (basic game) and `map-2.jpg`
(extended game); both show the same terrain, differing only in the printed
sequence-of-play boxes.

This is a summary written for someone designing a reusable C++ game library. It
records structure, not every clause. Where a rule is condensed, that is stated.
Rule numbers in parentheses refer to the rules booklet.

---

## 1. Game Overview

### 1.1 Subject and scale

The game covers the United States Marine Corps assault on Betio Island, Tarawa
Atoll, 20–21 November 1943. The map shows most of Betio plus the coral reef
lagoon to its north.

| Quantity | Value |
|---|---|
| Hex | 100 yards across |
| Turn (1–10) | 30 minutes |
| Turn (11–30) | 60 minutes |
| Turn 16 | a single 11-hour overnight turn (19.0) |
| US unit | infantry company, or a tank, engineer, artillery or HQ element |
| Japanese unit | a garrison force holding one fortified position |
| Map extent | roughly twenty-odd hexes on each axis; about half the hexes are water |

### 1.2 Players

The game is **solitaire**. One person plays the United States. The Japanese side
is run entirely by the rules, driven by a 54-card deck and a printed action
matrix. There is no Japanese player and no hidden Japanese decision-making — but
there *is* hidden Japanese information, since Japanese units and their depth
markers sit face down until revealed.

This asymmetry runs through every subsystem. The two sides do not share a common
unit model, a common combat procedure, or a common notion of control.

### 1.3 Sequence of play

Basic game, Turns 1–10 (4.0):

1. **US Amphibious Operations Phase** — assign LVTs, run landing checks, wade
   units ashore, recover LVTs, stage next turn's arrivals.
2. **Event Phase** — draw one event card, apply the event listed for the current
   turn.
3. **Japanese Fire Phase** — draw one fire card; every Japanese position whose
   colour appears on the card fires or acts; then artillery fire; then remove
   Japanese disruption.
4. **US Action Phase** — three actions plus unlimited free actions; then resolve
   all close combats.
5. **End of Turn** — clear the card track, advance the turn marker, reshuffle if
   the discard pile visibly exceeds the draw deck.

Extended game, Turns 11–30 (15.1), adds two phases:

1. US Amphibious Operations Phase
2. **First** Event Phase
3. Japanese Fire Phase
4. **Second** Event Phase
5. **US Engineer and HQ Phase** — collect support markers, spend them, convert
   HQs to command posts, advance command range
6. US Action Phase
7. End of Turn

Turn 16, the overnight turn, replaces this with a reduced sequence in which most
phases are skipped, fields of fire shrink to one hex, and eliminated Japanese
units are partially recycled into the reserve pool (19.0).

### 1.4 Victory

Victory is measured in **secured Japanese position hexes** (14.2). A position is
secured when all three of the following hold:

- the position hex is occupied by a US unit or a garrison marker;
- every position hex projecting *intense* fire into that hex is also occupied by
  a US unit or garrison;
- US communication can be traced from the position.

Thresholds vary by scenario: 10 positions at the end of Turn 10; 15 at Turn 15,
with at least one projecting fire onto the southern beach; a zone-coverage
condition at Turn 30. The game ends immediately in defeat on **US catastrophic
loss**: at any time after Turn 1, no infantry unit on the map has at least three
steps (14.1).

### 1.5 What is structurally distinctive

Three things set this game apart from a conventional hex wargame. First, there
are **no dice**; every random draw comes from one 54-card deck, and each card
carries three unrelated result sections of which only one is read per draw.
Second, the defender is not a set of mobile stacks but a fixed lattice of **named
positions** with **hand-drawn fields of fire printed on the map** — the defensive
geometry is authored data, not a computed range. Third, US movement is
**action-budgeted** rather than movement-point-budgeted: a unit either spends one
of three actions to move up to two hexes, or it moves for free because of its
command status.

---

## 2. Standard Elements

These are the parts that recur across hex-and-counter wargames, stated in terms
of what a library would have to store and compute.

### 2.1 The hex grid

A single contiguous hexagonal grid covering land and water alike. **Units may
enter water hexes** — unusual, and the water portion of the map is nearly as
large as the land portion.

Hexes carry **four-digit identifiers**, of which the two halves index the two grid
axes: `1538`, `2336`, `0126`, `0230`. Position A14 sits in hex `2336`, confirmed by
the addenda's map correction. The printed numerals run vertically along the hex edges
and are too small to read reliably at the available scan resolution, so which half is
the north–south index is **not asserted here**; settle it against the physical map
before writing any coordinate mapping.

Several rules depend on the printed ordering — for example, "check all position
hexes in order from west to east" (12.0) and "the lowest numbered position"
(9.2) — so the library needs a stable total order over hexes derived from the
printed numbering, not merely a set.

Hex distance is ordinary hex distance. Range is counted "including the target hex
but not the firing unit's hex" (8.11), so range *n* means hex-distance ≤ *n*.

### 2.2 Counters

**US units** carry:

| Field | Notes |
|---|---|
| Designation | historical only, no game effect |
| Unit type | infantry, heavy infantry, tank, engineer, artillery, HQ |
| Steps | 1–4; four-step units use two physical counters |
| Attack strength | falls as steps are lost |
| Range | blank (adjacent only), 2, 3, up to 7 for tanks, or U (unlimited) |
| Target symbol | a selector used by fire cards and events |
| Weapons | from the US Weapons Chart at full strength; printed on the counter once reduced |
| Arrival turn and beach | or `Rdc` (replacement counter), `E` (event entry), `D+2` (optional rule only) |

**Japanese units** carry: formation, elite flag, defence strength, a set of **US
weapon requirements**, an optional coastal code `C`, and for tanks a position
colour. They have an unrevealed back showing only the general type.

**Depth markers** are a second layer stacked beneath a Japanese unit. A unit plus
its depth marker is one force. The depth marker adds strength and adds further
weapon requirements, and is revealed separately from the unit it sits under.
Depth markers never occupy a hex alone (9.2).

**Markers**: turn, phase, disrupted (US, in two shades to distinguish this-phase
from prior-phase disruption), disrupted (Japanese), hero, inspired, action taken,
naval fire, support, garrison, command post range, smoke, artillery destroyed,
Japanese action letters, no-LVT-communication.

### 2.3 Zones of control — replaced by two asymmetric relations

There is no zone of control in the usual sense. Two different relations do the
work, and they are not each other's mirror image.

**US Control** (11.1). A US unit controls the hex it occupies. A US unit that is
either infantry with two or more steps, or a tank of any step level, *also*
controls all six adjacent hexes, even when disrupted. Single-step infantry,
engineers, artillery, HQs, command posts and garrisons control only their own
hex. A US unit stacked with a Japanese unit exerts no control at all (11.11), and
a Japanese unit adjacent to a US unit negates that unit's control of the adjacent
hex (11.21).

**Japanese field of fire** (6.2). A position projects fire into a fixed,
irregular set of hexes printed on the map as coloured dots. Dots come in two
grades, *intense* and *steady*. The set is not derived from range; it is drawn to
match the arcs of the real bunkers. Fields of fire of the same colour never
overlap, though they may abut; a dot is printed on the side of the hex nearest the
position that projects it, which resolves ownership visually (6.22).

Three modifiers extend a field of fire at runtime:

- a machine-gun action adds every hex adjacent to an intense-fire hex (6.31);
- a mortar action treats every hex within four hexes as steady fire (12.4);
- a Japanese tank adds every hex adjacent to a hex already in the field (6.24, 13.1).

### 2.4 Movement

Movement is an **action**, not a point budget (7.3). A unit spends one action to
move up to two hexes on land (three from Turn 11, 15.31). Units in water move two
hexes (three from Turn 11) automatically during the Amphibious Operations Phase
and must always move closer to a beach hex (5.21).

A land move ends immediately when the unit enters:

- a hex in the *intense* field of fire of a non-disrupted Japanese position, or
- a hex adjacent to any Japanese unit, even a disrupted one (7.31).

There is therefore a stopping predicate on hex entry, but no per-hex cost. The
only terrain that touches movement at all is the rough/crater hex, which
non-infantry units may enter but must stop in, and the pier hexside, which wading
units may not cross (5.22).

### 2.5 Discrete terrain types

| Terrain | US infantry / engineer / HQ | All other US units | Japanese defence |
|---|---|---|---|
| Beach, Clear, Airstrip | yes | yes | — |
| Palm Trees | yes | yes | halves the strength of US infantry firing *through* the hex at range |
| Building | yes | yes | unit strength doubled; depth not doubled |
| Fortified Building | yes | yes | unit **and** depth strength doubled |
| Rough / Crater | yes | yes, but must stop | unit strength doubled; depth not |
| Seawall **hexside** | yes | yes | unit strength doubled; depth not |
| Water, Coral Reef | wading or LVT only | wading or LVT only | — |
| Pier **hexside** | may not be crossed by wading units | may not be crossed | — |

Three notes from the map legend matter for implementation. Terrain effects on
Japanese defence are **not cumulative** — a unit in a crater attacked across a
seawall is doubled once, not twice. Japanese tank units gain no terrain benefit at
all (13.3). And a hexside defensive benefit applies only if **every** adjacent
attacking US unit is attacking across such a hexside.

That last clause is the one that matters most for the library: the defensive
modifier is a property of the *set of attack directions*, not of the target hex.

### 2.6 Supply — two directed reachability problems

Neither side has supply in the usual sense. Both have a **communication** test,
and the two tests are structurally different graph searches.

**Japanese communication** (11.2). A Japanese position is in communication if a
path of hexes of any length can be traced from it to **at least two other**
Japanese-occupied positions or position groups. The path may not pass through any
hex occupied or controlled by US units, and may not pass through a **beach hex**.
A position group counts as a single position for this purpose. Communication
status is frozen at the start of the Japanese Fire Phase and does not change
during it (11.23), but is evaluated at the moment of resolution during a US
attack or close combat.

This is not a single-source reachability query. It is "can this node reach two
distinct members of a target set through a blocked graph" — a counted
reachability, and the threshold of two makes it genuinely different from ordinary
supply tracing.

**US communication** (11.3). A hex is in US communication if a path of hexes of
any length can be traced from it to any beach hex in the US beachhead, defined
positionally as a beach hex adjacent to or west of position D6 and adjacent to or
north of position F13. The path may not pass through any hex occupied by, **or in
the field of fire of**, a Japanese unit — even if a US unit occupies that hex
(11.31). An empty Japanese position projects no field of fire for this purpose,
but a *disrupted* occupied one still does.

So the US blocking set is the union of all live fields of fire, which changes as
positions are taken and as disruption comes and goes.

### 2.7 Combat resolution

There is no odds ratio and no die roll. A US attack is resolved on the **US Attack
Results Chart**, a two-way lookup:

- **Row group**: whether the attacking force possesses *all* the weapons required
  by the revealed Japanese unit and depth marker — a boolean.
- **Row**: the comparison band between total US attack strength and total Japanese
  defence strength — *less*, *equal*, *greater but not double*, *at least double*.
- **Column**: the disposition of the target — *unit alone*, *unit with unrevealed
  depth marker*, *unit with revealed depth marker*.

Results are qualitative, not numeric: `Japanese defeated`, `Japanese disrupted`,
`Japanese gain depth`, `Depth marker eliminated`, `Attackers disrupted`, `No
effect`, and `Reveal the depth marker and consult again`. That last is a
**re-entrant result**: revealing the depth marker changes both the strength total
and the weapon requirements, and the chart is consulted a second time in the
column one step to the right.

Selected cells, condensed:

| Weapons? | Strength comparison | Unit alone | Unit + unrevealed depth | Unit + revealed depth |
|---|---|---|---|---|
| No | less or equal | attackers disrupted, Japanese gain depth | attackers disrupted | attackers disrupted |
| No | greater, not double | Japanese gain depth | attackers disrupted | no effect |
| No | at least double | Japanese disrupted | Japanese disrupted | Japanese disrupted, optional attrition |
| Yes | less | Japanese gain depth | attackers disrupted | no effect |
| Yes | equal | Japanese disrupted | no effect | Japanese disrupted |
| Yes | greater, not double | Japanese defeated | reveal depth, consult column to the right | depth eliminated, unit disrupted |
| Yes | at least double | Japanese defeated | reveal depth, consult column to the right | Turns 1–10: depth eliminated, unit disrupted. Turns 11–30: depth eliminated, unit defeated |

*Optional attrition* lets the attacker remove a step from an adjacent attacking
unit to eliminate the depth marker — a player-chosen trade offered by a result
code.

The weapon requirement set is the interesting part. Each Japanese unit and each
depth marker lists required weapons drawn from `BZ MG BR MO RD DE FT AR NA SP FL
CC`. The attack reaches the lower half of the chart only if the union of the
attackers' weapons covers the union of the defenders' requirements, with three
special cases:

- `FL` (flanking) is not a weapon at all. It is satisfied by attacking from at
  least two hexes adjacent to the target but **not adjacent to each other**; if
  both the unit and its depth marker require `FL`, three adjacent hexes are
  needed, and those three may be mutually adjacent (8.22).
- `CC` (close combat) cannot be satisfied by any attacking unit. A Japanese unit
  requiring `CC` can only be beaten by entering its hex (8.23).
- A hero, or from Turn 11 a spent support marker, is a wild card standing in for
  any one requirement except `FL` and `CC` (8.24, 17.5). `MG` also satisfies `BR`.

Japanese fire is resolved on a separate **Japanese Fire Chart**, indexed by the
fire-dot grade in the target hex crossed with the reveal state of the firing
position:

| Fire level | Fire by revealed position | Fire by unrevealed position | Ambush by unoccupied position |
|---|---|---|---|
| Intense (priority 1) | any US unit, any target symbol, loses a step | any US unit loses a step **and** is disrupted | one US unit with the target symbol loses a step |
| Steady (priority 2, includes water) | non-armoured US units with the target symbol lose a step | same, and on land also disrupted | one US unit with the target symbol is disrupted |
| Machine gun (priority 3) | non-armoured US units with the target symbol are disrupted | same, but lose a step | not applicable |

### 2.8 Modifiers

Because there is no die, there are no die-roll modifiers. What a conventional game
would express as a DRM appears here as one of:

- a **strength multiplier** from terrain (doubling, halving);
- a **column shift** — the depth-marker reveal moves the lookup one column right;
- a **flag on the fire card** — armour bonus, leader star, action letter;
- a **hit-limit integer** — a position may hit at most as many US units as it has
  non-disrupted units plus depth markers, raised by one from Turn 11 (6.31, 15.2);
- a **target-symbol override** — a hex holding five or more US steps is a
  *concentrated target* and every unit in it counts as matching the card's symbol
  regardless of what is printed on the counter (6.35).

### 2.9 Step reduction and disruption

US units lose steps one at a time; a four-step unit swaps to a two-sided
replacement counter after its second loss (2.21). A US unit may not lose more than
one step per Japanese Fire Phase in the basic game (6.34); in the extended game
that limit becomes one step *per firing position* (15.2).

Disruption is a binary status on either side. A disrupted US unit may perform no
action except removing its own disruption marker (7.6). A disrupted Japanese unit
does not fire and projects no field of fire — except for the purpose of blocking
US communication, where it still does (6.4, 11.31). Japanese disruption is removed
at the end of the Fire Phase for every position whose colour appeared on that
turn's fire card.

### 2.10 Retreat and advance

There is **no retreat and no advance after combat** (8.34). A defeated Japanese
unit is removed and the attackers stay where they are. The only combat-caused
movement is the withdrawal of survivors out of a close combat hex (8.62, 8.63),
and the withdrawal of a defeated *elite* Japanese unit in communication to the
reserve box rather than to the eliminated box (8.33; the addenda restricts this to
Turns 1–16).

### 2.11 Reinforcement and replacement

US units enter from a turn track: each unit is printed with its arrival turn and
its beach. During the Amphibious Operations Phase of turn *n*, units scheduled for
turn *n+1* are placed in beach approach hexes. Some units instead carry `E` and
enter on an event, and events can move units forward or back along the track
(*Expedited Arrival*, *Transport delayed*).

Japanese reinforcement is a **pool draw**, not a schedule. A reserve box holds
face-down units; the Reinforce, Muster, Infiltrate and Tunnels mechanisms each
draw from it and place into a position hex chosen by a deterministic priority
rule. Depth markers are drawn from two separate pools, coastal and inland, with a
fallback when one empties and a terminal state when both do (9.23).

### 2.12 Weather

None. There is no weather system. The only environmental variation is the
overnight turn.

---

## 3. Unique Exceptions

These are the mechanisms a naive shared model would break on.

### 3.1 A card deck in place of dice

All randomness comes from one 54-card deck. Each card carries three independent
sections — Landing Results, Event, and Fire — and a single draw reads exactly one
of them. The remaining sections are ignored. Because cards are discarded and only
reshuffled when the discard pile visibly exceeds the draw deck, the deck is
**sampling without replacement over an unknown horizon**: draws are correlated,
and a card seen this turn will not reappear soon.

A library that models randomness as "roll a die and apply modifiers" cannot express
this. What is needed is a shuffled deck object with several orthogonal result
fields per card and a caller-chosen projection.

### 3.2 Positions, position groups, and printed fields of fire

The Japanese defence is a fixed lattice of **positions**, each with:

- a unique ID of the form *zone letter* + *priority number*, zones `A`–`F`,
  numbers 1–17 — so `E13`, `A14`, `F7`;
- one of six **colours**, which is the activation key;
- a classification as *coastal* (in or adjacent to a beach hex) or *inland*;
- optionally an artillery class `L`, `M` or `H`;
- optionally a setup code `C`, `I`, or an armour symbol.

A **position group** is two or more position hexes joined on the map by a printed
**connector line**. The group is treated as a single position for firing, for the
hit limit, and for communication tracing — but is attacked one hex at a time
(8.32), and reinforced into only its lowest-numbered eligible hex (12.31).

Each position's field of fire is drawn on the map as coloured dots in surrounding
hexes, graded intense or steady. This is authored data. There is no formula.

### 3.3 Water fire zones and fire arc lines

Coastal positions on the north and west shores project fire into **arcs of water
hexes** bounded by printed *fire arc lines* (6.23). A single square steady-fire dot
in one water hex stands for every water hex in the same arc — that is, every water
hex reachable from it without crossing a fire arc line. The arc is a connected
region delimited by a set of hexsides, and membership is decided by region
flood-fill, not by distance.

This is the clearest argument on this map for hexside addressing: the fire arc
lines are drawn *on hexsides*, and the region they bound is what the rule actually
refers to.

### 3.4 Hidden information on the defending side

Japanese units and depth markers are placed face down. An unrevealed unit still
projects its field of fire and still fires; firing does not reveal it (6.0). It is
revealed only by a US attack or close combat. A unit's depth marker is revealed
separately and later, when an attack is strong enough to force it.

Two consequences: the Japanese Fire Chart has separate columns for fire by a
revealed position and by an unrevealed one — fire from an unrevealed position
additionally disrupts — and the attack chart has three columns keyed to reveal
state. Concealment is therefore a **defensive asset with a mechanical value**, not
merely an information-hiding convenience.

Concealment is also restorable, but only once: the overnight *loss of contact*
step flips all revealed Japanese non-tank units and depth markers back to their
unrevealed side (19.0).

### 3.5 Amphibious landing: facing, drift, and hexrow runs

The landing procedure (5.1) is the only place in the game with **unit facing**.
Units in a beach approach hex are oriented along a printed arrow. Two cards are
drawn per LVT stack:

1. **Drift** — `No drift`; `1L` or `1R`, shifting one hex left or right while
   retaining facing, clamped at the named boundary hexes `0139` and `0120`; or
   `PR`, pivoting in place to face the next hexside to the right. `PR` applies only
   to Beach Red 2 and is treated as no drift elsewhere.
2. **Landing result** — a step count for the carried units, a step count for the
   LVT, and one of five **locations**: `Reef` (stay put), `Water` (run until the
   first LVT-wreck hex), `Beach` (run until the first beach hex), `Inland 1` and
   `Inland 2` (run to one or two hexes past the beach, disregarding intervening
   Japanese units).

Each location result is a **ray cast along the facing direction** until a predicate
is satisfied. Beach Red 1 is the exception: its run follows a **hex spine** "first
to the left, then to the right in a zig-zag pattern" rather than a straight hexrow;
and three named hexes — `1138`, `1237`, `1238`, the "bird's beak" — have hard-coded
inland destinations `1338` and `1437` per the addenda.

A landing that ends in a Japanese-occupied hex triggers close combat immediately.

### 3.6 Close combat by opposed card piles

Close combat (8.6) is resolved without reference to strength at all. Each side
builds a face-down pile:

- **US**: one card per step to a maximum of four, plus one if any engineer with a
  flamethrower is present, plus one if any hero is present.
- **Japanese**: one card per unit and per depth marker, plus one if the Japanese are
  the attacker, plus one if total Japanese strength is 4 or more, plus one per `CC`
  requirement listed, plus one if a tank or if a night turn is underway — one card
  if both apply, not two.

Cards are then revealed alternately, Japanese first. A revealed card counts as a
**hit** if it shows the colour of the position in which the combat is occurring. A
Japanese hit removes a US step and discards a US card; a US hit removes a depth
marker, or the unit if there is none, and discards a Japanese card. Cards may also
carry a close-combat event: *Conscripts surrender*, *Heroism*, *Naval artillery
blast*, *Reinforce*, *US withdrawal hit*.

Combat ends by elimination or by mutual exhaustion of both piles; survivors are
disrupted and pushed back to an adjacent hex. Terrain has no effect on close
combat. A disrupted side spends its reveal removing disruption instead of turning a
card. Undrawn cards return to the top of the deck (8.66).

This is a stochastic race, not a table lookup. It shares nothing with the attack
chart.

### 3.7 Lettered actions unlocked over time

The Japanese repertoire grows during the game (12.0). Five action markers `R`, `M`,
`A`, `P`, `I` sit face down on a track; one is revealed at the start of Turns 3, 5,
7 and 9, and `I` is revealed on Turn 11. Once a letter is revealed, any fire card
showing that letter beside a position colour causes positions of that colour to
perform a lettered action instead of firing.

The action performed depends on a **classification of the position**: coastal or
inland; occupied or unoccupied; US units in field of fire or not — plus a separate
column for armour units.

| Letter | Occupied, US in FoF | Occupied, no US in FoF | Unoccupied, in communication, within 2 hexes of a US unit |
|---|---|---|---|
| `R` | Re-Supply — place a depth marker, or fire if all units already have one | Redeploy — move to an unoccupied position within 3 hexes and closer to the nearest US unit | Reinforce — place a reserve unit and a depth marker |
| `M` | Machine-gun fire — extends the field of fire to hexes adjacent to intense-fire hexes | Mortar fire — treat all hexes within 4 as steady fire | Muster — place a reserve unit |
| `P` | Patrol — disrupt every US unit in the field of fire, no fire | Patrol — disrupt one US unit within 4 hexes | — |
| `A` | Assault — move into the nearest US-occupied hex in the field of fire and conduct close combat | Artillery fire, if a coastal position with guns | Ambush — one step loss in the intense field of fire, else one disruption in the steady field |
| `I` | — | — | Infiltrate — place a reserve unit in a position **not** in Japanese communication |

Armour units have their own five entries: *Fire or Advance*, *Overrun*, *Multiple
Fire*, *Advance and Fire*, *Double Advance or Fire* (13.2). All three tank units
begin the game disrupted; their first action is to remove that disruption.

Evaluation order is fully specified and matters: positions are checked colour by
colour in the left-to-right order printed on the fire card, and within a colour from
**west to east** (12.0). Each Japanese unit performs at most one action per Fire
Phase. From Turn 11 several actions gain an additional fire component.

### 3.8 Deterministic tie-breaking everywhere

Because there is no opposing player, every Japanese choice is resolved by an
explicit priority list. Examples:

- **Depth-marker placement** (9.2): closest to a US unit; then coastal before
  inland; then lowest ID number; then lowest ID letter.
- **Artillery target** (6.5): in a beach approach hex; then in the water; then in a
  beach hex; then most steps; then player choice.
- **Assault target** (12.8): nearest US-occupied hex in the field of fire; then
  least US strength; then player choice.
- **Fire hit selection** (6.31): intense-fire hexes first, then steady, then
  machine-gun hexes; then most steps; then closest to the firing position; then
  player choice.
- **Tactical reinforcement placement** (9.3): nearest unoccupied inland position;
  then closest to a US unit; then lowest number, then lowest letter.

A library supporting this game needs a general **ordered tie-break chain**
evaluated over a candidate set, with "player chooses" as a legitimate terminal rule.

### 3.9 Leaders, support and garrisons

- **Heroes** are markers assigned to a unit. They grant a free action, a weapon wild
  card *or* +1 strength, and an extra close-combat card. When killed they flip to
  *inspired*, which keeps the free action but loses the rest. Major Ryan is a
  special hero who grants free actions to his whole hex and may lend his wild card
  to any unit stacked with him (10.17). Exactly eight hero markers exist and that is
  a hard cap.
- **HQ units** grant free actions to every unit in their hex and adjacent hexes at
  the start of the Action Phase, count as a radio, and enable tank and artillery
  ranged fire. An HQ that has moved stops granting free actions for the rest of the
  phase (7.33) — an ordering-dependent effect that a scheduler must respect.
- **Command posts** (16.0, Turn 11 onward) are immobilised HQs whose command range
  grows one step per turn along a printed track, up to five hexes, but only while
  the CP hex is out of every Japanese field of fire. Range may be counted through
  Japanese fields of fire but not through Japanese units (16.31).
- **Support markers** (17.0, Turn 11 onward) are generated one per engineer unit with
  the `SP` capability in US communication and out of enemy fields of fire, and may
  be spent for a garrison, a disruption removal, a free action, or a weapon wild
  card. They expire at end of turn.
- **Garrisons** are markers that make a position hex count as US-occupied for victory
  and for Japanese action checks, without being a unit. A garrison alone in a hex is
  removed if a Japanese unit acts into its field of fire or enters its hex.

### 3.10 US action economy

Three actions per turn, plus free actions for units with a hero, an inspired marker,
or a disruption marker; for HQ units; for any unit in HQ command at the *start* of
the phase; for any unit in command post range; and for any unit for which a support
marker is spent. A unit performs at most one action per turn whether free or not.
Two units in a stack may perform the *same* action for the cost of one (7.21).

The budget is on *decisions*, not on movement points and not on units.

### 3.11 Stacking

One or two US units may occupy a hex **at the end of the US Action Phase** only
(7.5). Limits may be exceeded freely during the phase and during every other phase.
Leaders, command posts and garrisons do not count. There is no step limit, but a hex
holding five or more steps becomes a concentrated target (7.52, 6.35). Excess units
at the end of the phase are eliminated.

The stacking rule is therefore a **phase-boundary invariant with a destructive
repair**, not a movement-time constraint.

---

## 4. Map and Coordinate Notes

This section classifies every map feature by the kind of address it needs. It bears
directly on the `hexmap` ABC/QRS coordinate system.

### 4.1 Hex numbering

Four digits, two per axis. Cited hexes run from `0120` and `0126` up to `2336`, so
each half spans roughly twenty-odd values. Which half is the north–south index is not
established here — see §2.1. Beach approach hexes carry alphanumeric labels instead —
`R1A`, `R2B`, `R3D`, and `G` for Beach Green — and sit in a fringe strip along one
map edge, each with a printed facing arrow.

The library needs a bijection between the printed ID and the internal coordinate,
plus a **total order** consistent with the printed numbering, because several rules
iterate "west to east" or select "the lowest numbered position."

### 4.2 Hex-centred features

- Terrain type: beach, clear, airstrip, palm trees, building, fortified building,
  rough/crater, water, coral reef.
- Position ID, position colour, coastal/inland classification.
- Artillery class `L`, `M`, `H`.
- Setup codes: blue `C`, blue `I`, armour symbol.
- **Fire dots**: zero or more per hex, each carrying a colour and a grade — intense,
  steady, or square steady for water. A hex may hold dots of several colours at
  once, since fields of fire of *different* colours may overlap.
- Potential LVT wreck hexes.
- The `Niminoa` shipwreck hex `0230`, which explicitly has no effect on LVT movement.
- Beach approach hexes, each with a primary and sometimes a secondary **direction
  arrow** — a per-hex facing datum.

### 4.3 Hexside (edge) features

- **Seawall hexsides.** A defensive modifier attached to the edge, applied only when
  every adjacent attacker crosses such an edge. A genuine edge attribute that cannot
  be pushed onto either hex.
- **Pier hexsides.** May not be crossed by wading units (5.22). The pier runs along a
  chain of hexsides out into the lagoon, ending at a pier head.
- **Coral reef hexsides.** Mark the reef line that LVTs cross and other transports
  cannot.
- **Water fire zone boundary lines (fire arc lines).** Drawn on hexsides; they
  partition the water into arcs, and a single printed dot colours an entire arc.
- The map edge, and the named clamp hexes `0139` and `0120` that bound lateral drift.

### 4.4 Centre-to-centre link features

- **Fire position connector lines** joining the hexes of a position group. This is an
  explicit graph over position hexes: a small set of undirected edges whose connected
  components are the position groups.

There are no roads or rail lines on this map. The connector lines are the only
centre-to-centre feature, and they are logical rather than geographic.

### 4.5 Region features (sets of hexes)

- **Position groups** — connected components of the connector graph.
- **Zones `A`–`F`** — a partition of positions used by the Turn 30 victory condition
  and by the Reinforce restriction, since coastal positions in zones A, B and C may
  not be reinforced (12.33).
- **Water fire zones** — regions of water bounded by fire arc lines.
- **The US beachhead** — a set of beach hexes defined relative to two named positions,
  used as the target set for US communication.
- **The southern beach** — used in the Turn 15 victory condition.
- Beach landing groups `R1`, `R2`, `R3`, `G`, each a small set of approach hexes with
  a shared facing convention.

### 4.6 Direction, rays and spines

Three rules need a **direction** as a first-class value rather than a mere adjacency:

1. Unit facing in beach approach hexes, set by a printed arrow and modified by the
   `PR` drift result, which rotates it to "the next hexside to the right."
2. Landing runs, which move "along the hexrow in the direction indicated" until a
   predicate holds — a ray cast in a fixed QRS direction.
3. The Beach Red 1 run, which follows a **hex spine** in a zig-zag, first left then
   right — an alternating two-direction walk. A hex spine is the line of hexsides
   shared along a column boundary; in ABC terms it is a chain of edges, and the
   zig-zag is an alternating sum of two of the three QRS basis vectors.

`hexmap`'s QRS vectors give the ray directions directly; the drift pivot needs
rotation among the six QRS directions; and the spine walk needs the ABC edge
addresses lying between two hex columns. A hex-only coordinate system can fake the
spine walk as an alternating hex sequence, but it cannot name the spine itself,
which is what the rule text describes.

### 4.7 Off-map spaces

Several spaces need node identity without hex geometry: the Japanese Reserve box; two
Japanese depth marker boxes, coastal and inland; the Japanese Eliminated Units box;
the Available LVT box; the US Infantry Loss box; the Admiral Shibasaki box; the turn
track; the card/phase track; the Japanese Allowed Actions track; and the US Command
Post Range track.

Of these, the depth pools and reserve box are **random-draw containers**; the tracks
are **ordered position holders**; the loss box is a **counter with a terminal
condition**, since filling it is catastrophic loss. None of them are hexes, and the
library should not force them to be.

---

## 5. Library Implications

### 5.1 Queries the rules force

| Query | Where it is used | Notes |
|---|---|---|
| Adjacency and hex distance | control, range, most priorities | ordinary |
| Ray cast in a direction until a predicate holds | landing runs (5.12) | needs directions as values |
| Alternating spine walk | Beach Red 1 landing (5.12) | needs edge addressing |
| Membership in an authored region | fields of fire, water arcs, zones | authored data, not computed |
| Connected components over an authored edge set | position groups (6.21) | small static graph |
| Region flood-fill bounded by hexsides | water fire zones (6.23) | edges block, hexes do not |
| Blocked reachability to *k* ≥ 2 distinct targets | Japanese communication (11.2) | not ordinary supply |
| Blocked reachability to a target set | US communication (11.3) | blocking set is a union of regions |
| Ordered tie-break over a candidate set | every Japanese decision | needs a composable chain |
| Two-key table lookup with re-entry | US Attack Chart (8.3) | a result may re-invoke the lookup |
| Set cover of requirements by capabilities | weapon requirements (8.2) | plus wild cards and `MG` ⇒ `BR` |
| Predicate over the *set of attack directions* | flanking, seawall (8.22, TEC) | not a per-hex property |
| Neighbourhood dilation of a region | machine-gun and tank field extension (6.24, 6.31) | one-ring expansion of an authored set |

### 5.2 Data structures

- **Hex attribute table** keyed by coordinate: terrain enum, position ID, position
  colour, artillery class, setup codes.
- **Fire dot table**: hex → list of (colour, grade). Multi-valued.
- **Edge attribute table** keyed by an edge address: seawall, pier, reef, fire arc
  boundary. This is the table a hex-only coordinate system cannot hold cleanly.
- **Position table**: ID → colour, coastal flag, artillery class, group ID, occupied
  hex, and the authored field of fire.
- **Region table**: name → hex set, for zones, water arcs, beachhead, southern beach.
- **Card deck**: an ordered sequence of cards, each with a landing section, an event
  section keyed by turn, and a fire section — three colour entries with optional
  star, armour and letter flags, a target symbol, and an optional artillery value —
  plus a discard pile and a conditional reshuffle.
- **Draw pools**: reserve units, coastal depth markers, inland depth markers, armour
  depth markers — each a bag with a fallback chain and an empty state.
- **Unit state**: side, type, steps, disruption, reveal state, facing (landing only),
  attached markers, stack membership.
- **Turn state**: turn number, phase, revealed action letters, available support
  markers, naval fire markers, LVT availability.

### 5.3 Where the game will not fit a shared abstraction

A generic hex-wargame library will want a `CombatResolver` that takes attacker
strength, defender strength, terrain modifiers and a die roll and returns a result.
Tarawa needs three unrelated resolvers: the attack chart, a comparison-band lookup
with a re-entrant column shift; the fire chart, a dot-grade by reveal-state lookup
with a per-position hit limit and priority-ordered target selection; and close
combat, an opposed card race. The right shared abstraction is a `CombatResolver`
*interface* with a game-supplied implementation, not a parameterised table.

Similarly, movement here is not a cost-based path search. The shared abstraction that
does fit is a **step-wise move with an entry predicate and a stopping predicate**, of
which point-cost movement is a special case.

### 5.4 Hidden state

The library must distinguish, for every Japanese unit and depth marker, between the
true state and the state visible to the player, and must let queries run against
either. Some rules read true state — fields of fire from unrevealed units — and some
read visible state, such as barrage eligibility against inland positions, which
requires revealed status (8.44). A single `revealed` flag on the counter plus explicit
query flavours is enough. A general fog-of-war system is not needed: only one side
hides information, and concealment is restored exactly once, at the overnight
loss-of-contact step.
