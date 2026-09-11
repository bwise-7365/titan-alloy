# Axis Empires: Dai Senso! — Rules Digest for Library Design

**Source.** *Axis Empires: Dai Senso!*, Alan Emrich, Thomas Prowell and Salvatore
Vasta, Decision Games, 2011. Primary source:
`Dai Senso  Living_Rules_February_2014.pdf` (Living Rules of 1 February 2014,
67 pp.), which already incorporates `DS Errata October 2012.pdf`. Terrain charts
transcribed from the map: `Dai Senso game map adjusted.png`. Counter formats from
`pic1753887.png`.

*Dai Senso!* is the sister game to *Totaler Krieg!*, which covers Europe; the two
can be played together as the combined game *Axis Empires*. This digest covers the
**Pacific game alone**. Rules that apply only to the combined game are marked in the
rulebook with a symbol that `pdftotext` renders as a replacement character, and they
have been excluded here except where noted.

This is a summary written for someone designing a reusable C++ game library. It
records structure, not every clause. The rulebook is 67 pages and this digest is
selective; where a rule is condensed, that is stated. Rule numbers in parentheses
refer to the Living Rules.

---

## 1. Game Overview

### 1.1 Subject and scale

The game covers the Asia/Pacific theatre of the Second World War from 1937 to V-J
Day. The play area is **two 22 × 34 inch maps**: a West Map reaching from India to
Japan, and an East Map reaching from Japan to Hawaii.

| Quantity | Value |
|---|---|
| Hex | about 120 to 300 miles across, depending on latitude |
| Game turn | 30 to 60 days |
| Year | four seasons; Spring begins Mar–Apr, Summer May–June, Autumn Aug–Sept, Winter Nov–Dec |
| Seasonal turn | the first turn of each season; extra phases are performed only then |
| Ground unit | battalion to army |
| Components | 560 counters, three option-card decks (Axis 76, Western 80, Soviet 44) |

### 1.2 Players

**Three factions**: Axis, Western, and Soviet. This is the game's most consequential
structural choice. The Western and Soviet factions are allies against the Axis but
are **enemies of each other**: "Anything related to one faction is considered an enemy
to both other factions at all times" (13.1). Combat between Western and Soviet units
is legal unless a specific rule forbids it. At the end of the game the two Allied
factions score both collectively and individually.

Below the factions sit **countries**, which are the real political actors. Countries
are Major or Minor; Minor countries have their own status, alignment and posture, and
can change sides. The game therefore has three levels of ownership — faction,
country, and dependent/region — and almost every rule is written against one of them
specifically.

### 1.3 Sequence of play

A game turn is a Seasonal Victory Phase, then three faction turns in fixed order —
Axis, Western, Soviet — then an End of Game Turn Phase. Each faction turn is:

1. **Seasonal Phase** *(seasonal turns only)*
   1. Option Card Segment — reveal the Pending Card as the new Current Card, perform
      its actions, select the next Pending Card
   2. Logistics / Partisan Segment
   3. Replacements Segment
2. **Initial Administrative Phase**
   1. Political Events Segment
   2. Support Segment — return support units to base, maintain beachheads, place
      support units and convoy markers, place Blitz markers
   3. Organization Segment — combine units, convert fortresses and garrisons,
      voluntary elimination, breakdown, detachments
3. **Operational Movement Phase** — including overruns
4. **Combat Phase**
   1. Blitz Combat Segment — airdrop, blitz combat, beachhead landing, airdrop
      landing, carrier strike returns
   2. Regular Combat Segment
   3. Marker Segment
5. **Reserve Movement Phase**
6. **Final Administrative Phase**
   1. War & Peace Segment — declarations of war, truce countdown
   2. Conditional Events Segment

and then, once per game turn:

7. **End of Game Turn Phase** — Delay Segment, Turn Marker Segment.

Two movement phases per faction turn, with the combat phase between them, is the
same exploitation shape as a two-impulse turn, but here the second movement phase is
explicitly a *reserve* phase with different eligibility: unsupplied units may move in
it, but overruns and exploitation may not occur.

### 1.4 Victory

Victory is measured in **Strategic Hexes**, which are printed on the map in three
colours — Western (green), Soviet (red) and Axis (orange).

At each Seasonal Victory Phase (0.1) the **Current Strategic Value** is computed:

1. Count Soviet and Western Strategic Hexes under Axis control, plus Allied Collapse
   markers in the Strategic Warfare Box.
2. Subtract Axis Strategic Hexes under Allied control, plus Military Takeover markers.
   Axis Strategic Hexes in an active Policy Affected Country do not count.
3. Add a bonus derived from the European Strategic Value track if any Axis Strategic
   Hex inside Japan is under Allied control.

The resulting integer positions a VP marker on a VP Track, on one of two faces —
*Rising Sun* or *Allied Crusade*. Additional markers on the same track (Japanese
Mandate, Hakko Ichiu) cap or modify the score.

The game ends by **automatic victory** — Asian Domination, Japanese Surrender, or
European Theatre Collapse — or when the **V-J Day marker** is removed from the turn
track, whichever comes first. On a tie, the Axis wins.

### 1.5 What is structurally distinctive

Three things. First, **option cards drive the game**: each faction plays one card per
season, chosen a season in advance from its own deck, and that card supplies
reinforcements, political events and conditional events. Second, **politics is a
first-class subsystem**: countries have status, alignment and posture; policies and
truces constrain movement, combat and supply; countries are conquered, liberated and
reactivated; and the map's political geography changes during play. Third, the
**naval layer is zonal, not hex-based**: the sea is partitioned into Naval Zones, each
with three holding boxes, and fleets, air units and convoys occupy the boxes rather
than the sea hexes — so sea control is a region-level attribute that gates port
access, movement and supply.

---

## 2. Standard Elements

### 2.1 The hex grid

Two map sheets, each with its own four-digit hex numbering, disambiguated by a
prefix letter: `w` for the West Map, `e` for the East Map. Tokyo is `e4904`; Hailar
is `w5625`; Singapore is `w3218`. A hex identifier is therefore a **(sheet, number)
pair**, and the two sheets are geometrically continuous but not numbered continuously.

Two hex kinds:

- **All-Sea hex** — contains only water. Ground units may not enter, except onto a
  friendly Beachhead marker.
- **Land hex** — contains any amount of land, including mixed land-and-water hexes.

An All-Sea hex with a Beachhead marker is *not* a Land hex, but it can be occupied,
retreated into, and used as a movement source.

Range is counted including the destination hex but not the origin hex.

### 2.2 Counters

**Ground units** are the only counters with three numbers along the bottom:
**Attack Factor – Defense Factor – Movement Allowance** (for example 3-3-1, 7-6-2,
0-1-1). They also carry nationality colour, unit-type symbol, step count, and
optionally a Delay Stripe, an Exp (expeditionary) marking, a Colonial marking (white
type box), or an Elite marking.

Unit types with distinct rules: infantry, armour-type, mechanized, marine, airborne,
cavalry-mechanized, fortress, garrison, HQ. HQ units are ground units with the three
numbers, but they have their own stacking, retreat and support rules.

**Support units** are fleets, submarine fleets, carrier strike units, air force units
and bombers. They occupy Naval Zone Boxes or hexes rather than participating in
ground stacking.

**Markers** are everything else, and they are numerous and load-bearing: Beachhead,
Airdrop, Detachment, Logistics, Partisan Base, Blitz, Totsugeki, Convoy (Escort,
Scratch, Standard, Fleet Train, Supply, Troop), Devastation, Influence, Policy,
Truce, Government, Collapse, Delay, and the various track markers.

Several markers behave as **quasi-units**: a Detachment or Logistics marker can be
attacked, controls a hex, blocks supply, and creates an Air or Naval Base. A
Beachhead marker occupies an All-Sea hex and carries a **facing**. An Airdrop marker
is a unit turned inside out.

### 2.3 Zones of control

A ground unit projects a ZOC into every adjacent hex (8.0), with four exceptions:

- not across a **Mountain hexside**;
- not across an **All-Sea or Strait hexside**, except across a **Beachhead hexside**;
- not into a hex inside a **Policy Affected Country**;
- a unit belonging to a Policy Affected Country projects none at all.

The presence of a **friendly ground unit or Airdrop marker** in a hex negates all
enemy ZOC in that hex, immediately and for as long as it stays. Logistics, Partisan
Base and Detachment markers do not negate ZOC.

Weather turns this off. In Mud a unit may not move *out* of a hex containing an EZOC
at all, and friendly units do not negate it; in Storms or Snow a unit must stop after
moving into or out of such a hex, again with no negation (11.3–11.5).

So the EZOC predicate is genuinely three-valued in context: *no EZOC*, *EZOC negated
by a friendly unit*, and *EZOC not negatable because of weather*. Three separate rule
families read it differently.

### 2.4 Movement

Three movement procedures, and **a unit may use only one per movement phase** (3.1.4).

**Hex-to-hex** (3.1.1). Pay the MP cost of each hex entered plus the cost of the
hexside crossed. A unit may exceed its allowance to enter the *first* hex of a phase,
but must then stop. Overruns are permitted (3.2).

**Port-to-port** (3.1.2). A unit starting in a Port hex, on a Beachhead, or in an
Off-Map Box may cross a Naval Zone to any friendly **Open Port** in that zone, if a
friendly Troop Convoy marker with sufficient capacity is in that zone's Convoys Box.
This expends the entire movement allowance regardless of how many zones are crossed,
and chaining through Multi-Zone Ports is possible. Convoy capacity is one one-step
unit per marker, and markers cannot be combined — only a US Fleet Train can carry a
multi-step unit.

**Off-map box to off-map box** (3.1.3). Direct transfer between adjacent boxes as
printed on the map; the unit must stop.

Additional single-purpose moves: a **Marine** may cross one All-Sea hexside for its
entire allowance; a ground unit may move onto a friendly **Beachhead** for its entire
allowance; a unit on a Beachhead may move off it **only across the Beachhead
hexside**; Soviet units may enter or leave the Eastern Europe Box from a named range
of map-edge hexes.

Only *supplied* units may move in the Operational Movement Phase; unsupplied units may
move in the Reserve Movement Phase (10.3).

### 2.5 Discrete terrain types

The printed Terrain Effects Chart is **split explicitly into hex terrain and hexside
terrain**, with separate movement cost and combat-shift columns for each. This is the
most direct statement of the hex/edge distinction of the three games examined.

**Hex terrain:**

| Hex terrain | MP cost | CRT column shift |
|---|---|---|
| Clear (including Desert) | 1 MP | no effect |
| City or Capital | 1 MP | 1 left |
| Port | 1 MP | no effect |
| Town | pay the cost of the other terrain in the hex | no effect |
| Rough (Hills, Forest, Swamp) | 2 MP | 1 left |
| All-Sea (without Beachhead marker) | prohibited | prohibited |

**Hexside terrain:**

| Hexside terrain | MP cost | CRT column shift |
|---|---|---|
| Mountain | +2 MP | +2 left |
| River | +1 MP | +1 left |
| Road or Rail (one-step unit) | ½ MP | no effect |
| Road or Rail (multi-step unit) | 1 MP | no effect |
| Strait (connected) | as Road or Rail | +2 left |
| Strait (not connected), or Beachhead-2 | entire MA | +2 left |
| Beachhead-1 | entire MA | +1 left |
| SNLF Beachhead-0 | entire MA | no effect |
| All-Sea or Lake | Beachhead or Marine only | Marine only (+2 left) |

Two notes from the chart carry real weight.

**Roads and rails are hexside costs.** They are drawn on the map as lines running
centre to centre, but the chart charges them as a *hexside* crossing: "movement along
a Road or Rail negates any other terrain MP costs," and the unit instead pays a
road/rail hexside cost that depends on whether it is a one-step or multi-step unit.
A road is thus simultaneously a centre-to-centre link and an edge attribute.

**Hexside shifts do not accumulate.** A hexside shift is added to the hex shift, but
"only one hexside shift is applicable, and it must be the **lowest** hexside shift out
of all the different hexside terrain types being attacked across." A defender attacked
across both a mountain hexside and a river hexside receives one shift, for the river.

The chart also states that ZOCs do not extend across mountain, all-sea or strait
hexsides — the same fact appearing in two places.

### 2.6 Supply

Supply is a layered trace with three distinct segments (10.0), each with its own
rules, and this is one of the most demanding graph problems in the three games.

**Sources**: a Home Country City for units of that country; a Partisan Base marker for
units of the Minor Country it sits in; a Western or Soviet Off-Map Box for that
faction's counters. A city or Partisan Base cannot be a source if its hex holds an
enemy unit, Airdrop, Detachment or Logistics marker.

**The trace** proceeds outward from the unit's hex:

1. **Two Hex Free Trace.** The first two hexes may cross any terrain not otherwise
   prohibited.
2. **Road/Rail Trace.** On reaching a hex containing a Road or Rail symbol, the line
   may follow connected Road/Rail symbols any distance to a source. Road or rail
   symbols adjoining a **Connected Strait** count as connected. But the line may cross
   only **one** Connected Strait *or* **one** stretch of contiguous Road hexes; once it
   has used one, a second is barred. Rail is unlimited.
3. **Naval Zone Trace.** The line may cross a Naval Zone from Open Port to Open Port,
   and continue through any number of zones, provided **each** zone contains a friendly
   Supply Convoy marker in its Convoys Box.

**Restrictions**: no tracing into an EZOC hex unless a friendly unit is there; none
into a hex with any enemy unit, Airdrop, Detachment or Logistics marker; none into a
hex with an enemy Partisan Base and no friendly ground unit; none into an
enemy-country City or Port hex not under friendly control; none into a Neutral Minor
Country; none across an All-Sea hexside unless it is a Strait hexside or belongs to a
Beachhead hex.

Supply is checked **at the moment it is needed**, not once per turn (10.2), and is a
precondition for a long list of separate activities: operational movement, airdrops,
beachhead landings, HQ ranged support, HQ combat shifts, marine shift reduction,
advance after combat, exploitation, unit combination, and acting as an Air or Naval
Base. Being unsupplied is not fatal; it is a wide-ranging capability loss.

An **Open Port** is itself a compound predicate (see §3.4) that depends on enemy fleet
and air placement in Naval Zone Boxes — so the supply graph's edges are gated by the
naval situation.

### 2.7 Combat and the CRT

Attacks are declared one hex at a time. A hex may be attacked only once per combat
segment, and **all units in it must defend** — the attacker may not pick off individual
units, and the defender may not withhold any (4.2.1). A phasing unit attacks at most
once per segment. Units in the same hex may attack different hexes.

The attack sequence (4.2.2):

1. Attacker declares attacking units, and optionally one HQ providing ranged support.
2. Defender may declare one supplied HQ providing ranged support, if none is in the hex.
3. Sum Attack Factors and Defense Factors; express as a ratio from the CRT's columns;
   round in the defender's favour. Odds are clamped to the range **1-3 to 9-1**.
4. Determine column shifts, add attacker shifts and subtract defender shifts, apply the
   net to the column from step 3, clamping again to 1-3 and 9-1.
5. Roll one die and cross-index.
6. Resolve retreats.
7. Apply attrition, attacker first then defender.
8. Advance after combat or exploit.

There is a minimum: an attack may not be declared unless the attacker can muster a raw
1-3 ratio before shifts.

**Results** come in two kinds, retreats applied first:

| Code | Meaning |
|---|---|
| `Ad` | Attacker Defeated — all attacking ground units retreat one hex, or one attacking unit takes a step loss; attacker chooses if retreat is possible |
| `Ex` | Exchange — as `Ad`; if the attacker takes the step loss instead of retreating, the defenders must then retreat one hex or take a step loss |
| `Dr1`, `Dr2`, `Dr3` | Defender Retreat the stated number of hexes; unsatisfiable retreat converts one-for-one into step losses |
| `#/#` | Attrition — step losses, attacker's number first |

The numeric body of the table is on the player aid cards, which are not in this source
folder, and is not transcribed here.

### 2.8 Modifiers — column shifts only

Dai Senso modifies the odds column, never the die roll, in ground combat. Shifts are
**cumulative** across sources: sum attacker shifts, subtract defender shifts, apply the
net once.

| Source | Direction | Notes |
|---|---|---|
| Air unit (4.2.3.1) | one per unit, either side | must be in or adjacent to the defending hex, and of the same nationality as a participating unit; suppressed by Mud, and by Storms/Snow if the air unit is merely adjacent |
| Airdrop marker (4.2.3.2) | one per marker, attacker | Blitz Combat Segment only; nationality irrelevant |
| Armour (4.2.3.3) | one, attacker | Blitz only; one shift total regardless of how many armour units |
| Fortress unit (4.2.3.4) | one, defender | need not be supplied |
| HQ (4.2.3.5) | one, either side | including an HQ providing ranged support; the HQ also contributes its factor |
| Marines (4.2.3.6) | one, attacker | supplied, Blitz-enabled, attacking across a Beachhead, Strait or All-Sea hexside |
| Mud (4.2.3.7) | one, defender | Storms and Snow give no shift |
| Terrain (4.2.3.8) | defender | hex shift plus **at most one** hexside shift, and that the lowest applicable |

**HQ ranged support** deserves its own note. A supplied HQ may contribute its factor
and a column shift to any combat within **two hexes**, on either attack or defence,
provided the combat includes a ground unit of the same nationality. The support path
may cross enemy units and All-Sea hexes but not Neutral or Policy Affected countries.
An HQ that participated in a combat may not support another later in the same segment,
which makes "soaking off" an enemy HQ by attacking its hex a real tactic.

### 2.9 Retreat and advance

Retreat is one hex at a time, re-evaluating the priorities at each step (4.2.5):

- **Priority 1**: the hex entered must be **farther from the defending hex** than any
  hex retreated into so far, and must contain no EZOC.
- **Priority 2**: if no such hex exists, it must still be farther away, and must contain
  a friendly ground, Airdrop or Air unit.
- If neither exists, the force cannot retreat.

The "farther away" test is strict and geometric: a unit may not double back or move
laterally, even if no EZOC is present, and terrain does not excuse it. This makes the
retreat search a **monotone-distance walk**, not an ordinary shortest-path.

Restrictions: a force cannot retreat if it includes a unit with movement allowance 0
that fought, or an HQ that fought; no retreat into an Off-Map Box, into a hex with an
enemy unit, Airdrop or Beachhead marker, into a Neutral Minor Country or a Policy
Affected Country, or across an All-Sea hexside unless it is a Strait or Beachhead
hexside.

A retreat into a hex with friendly ground units **sweeps those units into the retreating
stack**, and they count as having participated in the combat — a rout. Stacking limits
may be violated during and at the end of a retreat, but must be repaired at the end of
the segment.

Unsatisfiable retreat converts one-for-one into step losses (**mandatory conversion**),
and the defender may *choose* to convert when all adjacent attackers are attacking
across Strait, Beachhead or Mountain hexsides, or when the defending hex contains a
City and no Blitz marker (**voluntary conversion**).

**Advance after combat** places any number of supplied attacking units into a vacated
hex, ignoring movement costs but not movement legality. **Exploitation** (4.2.8) is a
further post-combat move, blocked into Mud hexes and unavailable after an automatic
success against markers alone.

### 2.10 Stacking

Three ground units and six steps per hex; three ground units and **three** steps in a hex
or on a Beachhead marker carrying the Limited Stacking symbol (9.2). Only friendly
ground units count; every other counter is free.

Restrictions beyond the count (9.1): no two HQ units together; no two Fortress units
together; and ground units of **different Minor Countries** may not stack, with
exceptions for one-step Exp units and for Chinese pan-national HQs. The composition
rule is: any number of the faction's Major Country units, plus one Minor Country's
units, plus Exp units from other Minor Countries.

Limits are enforced **at the end of every phase and segment**, and violations are
repaired by eliminating steps.

Off-Map Boxes have no stacking limits at all.

### 2.11 Reinforcement and replacement

Reinforcements arrive almost entirely through **option cards** rather than a fixed
schedule (see §3.1). A card's Option Card Segment lists counters to place in the Force
Pool, the Delay Box, or the Strategic Warfare Box. Minor Country units listed on a card
arrive only if that country is currently active and aligned with the playing faction.

**Replacements** are steps placed at Replacement Locations: a supplied HQ in its home
country, or a Logistics marker able to trace supply — an overland supply line, for the
Soviet faction.

The **Delay Box** is a general-purpose recycling mechanism. Any counter with a Delay
Stripe that is removed from the map goes there rather than to the Force Pool, and
returns to availability after a number of turns determined by a Delay Result die roll
(7.1). Delay results are notably exempt from the usual 1–6 clamping of modified rolls.

### 2.12 Weather

Adverse weather turns are **printed on the turn track** — they are not rolled for. Three
kinds: Mud, Storms, Snow.

Weather applies to **Weather Areas**, which are defined as sets of countries rather than
map regions: Central, Desert, North, North Monsoon, South, South Monsoon. For weather
purposes a country's area membership uses its *printed* disposition, so a territory
ceded to Japan keeps its original weather. Many Pacific and Indian Ocean islands belong
to no weather area and never suffer weather effects at all. An All-Sea hex holding a
Beachhead marker belongs to the weather area its Beachhead hexside points into.

Effects, summarised:

| | Mud | Storms | Snow |
|---|---|---|---|
| Movement out of an EZOC hex | prohibited | must stop | must stop |
| Friendly units negate that EZOC? | no | no | no |
| Blitz attack against such a hex | prohibited | prohibited | prohibited, except Blitz-enabled Russians |
| Defender column shift | 1 left | none | none |
| `Dr` results | `Dr3`→`Dr2`, `Dr2`→`Dr1`, `Dr1`→`Ex` | same | same |
| Air unit shift | none at all | none if adjacent | none if adjacent |
| Exploitation into the hex | prohibited | allowed | allowed |
| Retreat, advance after combat | unaffected | unaffected | unaffected |
| Naval side effects | support units barred from named All-Sea hexes | — | Ice hexes lose Naval Base and Open Port status; support units barred from named northern zones |

---

## 3. Unique Exceptions

### 3.1 Option cards as the engine

Each faction has its own deck and plays exactly one card per season. The mechanism is
**pipelined**: at each Seasonal Phase a faction reveals its **Pending Card** as the new
**Current Card**, performs its actions, and then chooses the next Pending Card from its
hand. Selection is final and binding a full season ahead.

Card selection is constrained several ways at once:

- **Selection Requirements** printed on the card — conditions on played cards, country
  postures, the state of the Japanese Government marker, or the European war.
- **War State gating by colour**: blue cards are Pre-War, gray Limited War, red Total
  War. A faction may play only cards up to the current war state.
- **Sequential restrictions** — certain card types may not be played back to back.
- **Annual restrictions** — certain card types once per calendar year.
- **Production Value budgets** for the Soviet faction, shared across the two maps in the
  combined game.

A faction that cannot legally select any card says so and reveals no Current Card next
season.

Each card has several independent sections read at different points in the sequence:
Option Card Segment actions, Political Events, and Conditional Events. The Soviet cards
additionally carry a Production Value that adjusts the European Strategic Value track.

For a library, this is a **constraint-checked card selection problem with a one-step
lookahead pipeline**, not a shuffled deck. The cards live in a hand, not a draw pile;
what matters is legality, not randomness.

### 3.2 Countries, dependents, regions, and posture

The political model has four layers.

- **Faction** — Axis, Western, Soviet.
- **Country** — Major (Japan, US, Britain, Russia…) or Minor (the seven Chinese
  countries, Siam, the Netherlands East Indies, and so on). A Minor Country has a
  **status**: *Active*, *Neutral*, or *Conquered*. Major Countries are always active.
- **Dependent** — a possession of a country (Korea is a Japanese Dependent; Indochina a
  French one). Dependents are explicitly *never* "a friendly Country," which matters for
  Open Port and supply rules.
- **Region** — a sub-area of a country that can be ceded separately (the Mongol
  Frontier, Inner Mongolia).

On top of status sits **alignment** (which faction a country is aligned with) and
**posture** (belligerent, Policy Affected, or Truce Affected). A **Policy marker** in a
country's Posture Box makes it a **Policy Affected Country**, and a PAC is close to
inert: its units project no ZOC, no ZOC is projected into it, movement and attacks into
it are restricted, supply may not be traced into it, and its Strategic Hexes are not
counted for victory.

Countries are **conquered** and **liberated** during play by Conditional Events, and a
conquered country can be reactivated under the *other* Allied faction — so map ownership
of whole territories is mutable, and it changes what counts as friendly terrain.

The practical consequence for a library: a hex's political attributes are not fixed map
data. Country, dependent and region membership are static, but *status*, *alignment* and
*posture* are mutable state consulted by dozens of rules.

### 3.3 Naval Zones — a region layer with its own state

The sea is partitioned into **Naval Zones**, each defined by **Naval Zone Border
Hexsides** printed on the map and consisting of All-Sea hexes *plus* coastal Land hexes.
Some coastal hexes belong to more than one zone at once. Two zones are **adjacent** if
they share a border hexside.

Each Naval Zone has three boxes: **On Station**, **Convoys**, and **Used**. Fleets,
submarine fleets, carrier strikes and air force units go into On Station; convoy markers
into Convoys, and then to Used once spent. The Naval Zone Boxes are printed *over* All-Sea
hexes on the map, and those hexes remain in play beneath them.

Naval Zone state gates a great deal:

- **Port-to-port movement** requires a friendly Troop Convoy in the zone's Convoys Box.
- **Supply tracing across a zone** requires a friendly Supply Convoy in that box.
- An enemy Fleet in a zone's On Station Box closes **every** port in the zone.
- An enemy Air unit in the box closes ports within three hexes of a Naval Base of that
  same faction, and only for activities performed within that zone.

There is a symmetry rule — **Open Port Mutual Interdiction** — under which, if two
factions each deny the other the same pair of hexes, neither has an Open Port.

This is a genuine second spatial layer: a partition of hexes into named regions, each
carrying mutable occupancy state that acts as a gate on movement and supply through the
region.

### 3.4 Compound hex predicates

Dai Senso builds several named predicates out of combinations of hex contents, and then
uses those names throughout the rules. They are worth listing because a library will want
them as memoised derived properties rather than open-coded tests.

| Predicate | Definition (condensed) |
|---|---|
| **Air Base** | a Land hex with a City, Port, Road or Rail, and a supplied ground unit, Detachment or Logistics marker; no enemy Air unit present |
| **Naval Base** | a Land hex with a Port and a supplied ground unit, Detachment or Logistics marker; not an All-Sea Beachhead hex; not a snowbound Ice hex; no enemy Air unit |
| **Open Port** | a Port hex with a friendly ground unit or marker, *or* a Port hex in a friendly active Country, *or* an All-Sea hex with a friendly Beachhead, *or* a designated Off-Map Box — minus the enemy-presence and enemy-naval restrictions above |
| **Control** | a three-level priority: (1) an enemy Strategic Hex with a friendly Devastation marker; (2) a hex holding a friendly ground unit, Detachment or Logistics marker; (3) a hex in a friendly active Country or Dependent, or a Conquered enemy Minor Country. Lowest priority number wins |
| **Blitz-enabled** | within two hexes of a friendly Blitz or Totsugeki marker, including the marker's own hex |
| **Island** | a Land hex surrounded entirely by All-Sea, Strait or Beachhead hexsides |
| **Border** | two countries share a border if at least one Border Hexside separates them — and a Border Hexside may be entirely across a Strait or All-Sea hexside |

The **Control** predicate is a clean example of a rule that is naturally a priority list
rather than a boolean, and it is the one a shared library is most likely to get wrong by
simplifying.

### 3.5 Beachheads — a marker that carries a facing

A **Beachhead marker** sits in an All-Sea hex and has an arrow pointing at one hexside,
its **Beachhead Hexside**. It is one of only a handful of directional objects in the three
games examined.

Its behaviour is deliberately asymmetric. Ground units may move onto a Beachhead from any
adjacent Land hex, crossing All-Sea hexsides freely — friendly Beachheads "act like
bridges" — but may leave **only across the Beachhead hexside**. ZOCs project across the
Beachhead hexside but not across the other five. Supply may be traced through *any*
hexside of a Beachhead hex, unlike movement. Units may attack across a Beachhead hexside,
and a Beachhead Landing places a Blitz-enabled unit directly into the adjacent Land hex,
ignoring movement entirely.

Beachheads come in strengths — Beachhead-2, Beachhead-1, and the Japanese SNLF
Beachhead-0 — which differ in the column shift they give the defender and in whether they
confer Blitz-enabled status.

### 3.6 Overruns and blocked overruns

A multi-step armour-type unit moving hex-to-hex may enter a Land hex containing a
**single one-step** enemy ground unit and eliminate it at no extra MP cost, ignoring that
unit's EZOC (3.2). It may do this repeatedly in one phase.

The interesting part is the block list. An overrun is forbidden if the target could
receive **any** defender's CRT column shift — from terrain, Mud, a Fortress, an Air unit,
or an **HQ within ranged-support distance**. That last clause means a single well-placed
HQ can protect an entire two-hex neighbourhood from overrun, and the same HQ can block
more than one overrun in the same phase. Overruns are also barred in the Reserve Movement
Phase and during exploitation.

So the overrun predicate is "would a hypothetical combat here give the defender a shift?"
— a counterfactual evaluation of the combat-shift machinery during movement.

### 3.7 Airdrops as a two-stage object

An airdrop is not a movement. A supplied, Blitz-enabled airborne unit is **flipped over to
its Airdrop marker side** and placed within two hexes. The Airdrop marker then exists on
the map with its own properties: it provides a combat column shift, it negates enemy ZOC
for friendly units, and it blocks enemy retreat paths and supply lines. It may be placed
in a hex containing enemy units.

Later in the same segment (4.1.4) the marker either becomes a one-step infantry unit — if
supplied and not stacked with an enemy unit — or goes to the Delay Box.

The airborne unit is therefore in one of three states: a unit, a marker, or delayed. That
is a small state machine attached to a counter, not a movement rule.

### 3.8 Detachments, Logistics markers, and Partisan Bases

Three marker classes that behave partly like units:

- A **Detachment** or **Logistics** marker controls its hex, blocks supply, can create an
  Air or Naval Base, and can be attacked — an attack against a hex containing only such
  markers **automatically succeeds** with no die roll, and the marker goes to the Delay
  Box. Neither negates enemy ZOC.
- A **Logistics** marker is a Replacement Location if it can trace supply — overland
  supply, for the Soviets.
- A **Partisan Base** marker is placed in a Rough or City hex of a Conquered Allied Minor
  Country and is a supply source for that country's units. It blocks enemy supply tracing
  unless a friendly ground unit shares the hex. Only one may be placed per conquered
  country per segment, and none may go into a ceded Region.

### 3.9 War State as a one-way ratchet

Three war states — **Pre-War**, **Limited War**, **Total War** — with one-way transitions
triggered by named card plays or by any Allied country adopting a posture of War. There is
no return to an earlier state.

Pre-War is very restrictive: no unit may move or attack outside its own country or
dependent; no support units or airdrop markers may be placed; only blue cards may be
selected. Limited War lifts most of that but bars Allied declarations of war. The
transition to Total War executes a ten-step script that removes policy markers, schedules
the V-J Day marker sixteen seasonal turns out, and queues four Axis conditional events.

### 3.10 Blitz Combat as a separate combat segment

Every faction turn has two combat segments. In the **Blitz Combat Segment**, only
**Blitz-enabled** attacking units and markers may attack or provide shifts — where
Blitz-enabled means within two hexes of a friendly Blitz or Totsugeki marker. Defenders
need not be Blitz-enabled, and a defending HQ can provide ranged support regardless.

Three column shifts — Airdrop, Armour, and the marine shift's full value — exist **only**
in Blitz combat. Weather blocks Blitz attacks against hexes in Mud, Storms or Snow, with a
single exception for Blitz-enabled Russian units in Snow.

A faction with no Blitz marker on the map simply skips the segment. So the same underlying
combat resolver runs twice per turn under two different capability masks.

### 3.11 Attacker first-loss ordering

When an attacking force takes losses, the **first** step lost is constrained (4.2.6.1): if
the force includes a marine attacking across an All-Sea, Beachhead or Strait hexside, the
first loss must be a marine step; if it includes an armour-type unit, the first loss must
be an armour step. If both apply, the attacker chooses which. After the first loss, the
remainder is free.

This is a small rule with an outsized shape: loss assignment is not "owner chooses" but
"owner chooses subject to an ordered constraint derived from the composition of the
attacking force."

---

## 4. Map and Coordinate Notes

### 4.1 Hex numbering

**Sheet prefix plus four digits**: `w5422`, `e4904`. `w` is the West Map, `e` the East
Map. The two sheets are laid side by side and are geographically continuous, but their
numbering is independent. Named ranges of hexes appear in the rules as inclusive spans
along a map edge — `w5311` to `w6011` for the Russian western edge.

A coordinate in this game is therefore a **(sheet, first pair, second pair)** triple. Which
half of the four digits indexes north–south is not established here. The library also needs
a notion of sheet-crossing adjacency distinct from the numbering, since the two sheets are
geometrically continuous but independently numbered.

### 4.2 Hex-centred features

From the printed Terrain Key:

- Terrain: Clear (Normal), Clear (Desert), Rough (Hills), Rough (Forest), Rough (Swamp),
  All-Sea, Ice.
- Settlement: Town, City, Provisional Capital (City), Capital.
- Port, Multi-Zone Port, Key Port.
- Rail symbol, Road symbol (hex-level presence, used by the supply trace).
- Oil hex.
- Limited Stacking hex.
- Aid or Lend-Lease hex.
- US Impact hex.
- Strategic hex, in three colours — Western (green), Soviet (red), Axis (orange).
- Country, Dependent and Region membership.
- Weather Area membership, derived from the *printed* country, not the current one.
- Naval Zone membership — a Land hex may be coastal and belong to one or two zones.

### 4.3 Hexside (edge) features

Nine distinct hexside types appear between the Terrain Key and the Terrain Effects
Chart, and most of them carry their own movement cost and combat shift:

- **Mountain hexside** — +2 MP, +2 defender shift, blocks ZOC.
- **River hexside** — +1 MP, +1 defender shift.
- **Lake hexside** — blocks movement and retreat.
- **All-Sea hexside** — blocks movement except onto a Beachhead or by a Marine, blocks
  ZOC, blocks supply, blocks retreat; gives a +2 defender shift against a Marine assault.
- **Strait hexside**, in two varieties, **connected** and **not connected** — a connected
  strait is crossed at road/rail cost and joins road/rail networks for supply; an
  unconnected one costs the entire movement allowance. Both give +2 to the defender. Both
  block ZOC.
- **Naval Zone Border hexside** — bounds the naval regions and defines zone adjacency.
- **Country or Dependent Border hexside** — defines what "shares a border" means, and a
  border hexside may lie entirely across a strait or the open sea.
- **Region Border hexside** — the same, one level down.

The Beachhead marker adds a runtime hexside attribute: its **Beachhead Hexside** is the
one hexside of an All-Sea hex through which units may exit, ZOC projects, and attacks may
be made.

### 4.4 Centre-to-centre link features

- **Roads** and **Rails**, drawn hex centre to hex centre. They serve two roles at once.
  As a **network**, connected road and rail symbols carry the supply trace any distance,
  with roads limited to one contiguous stretch per line and rails unlimited. As a
  **hexside attribute**, moving along a road or rail replaces the destination hex's MP
  cost with a road/rail hexside cost that depends on the unit's step count.

That dual role is the sharpest illustration in these three games of why a coordinate
system that can name both hexes and edges is useful: the same drawn line is a graph edge
between two hex centres *and* a modifier attached to the hexside it crosses.

### 4.5 Region features (sets of hexes)

- **Countries** and **Major/Minor** classification.
- **Dependents** — sub-territories that can be ceded and become separate dependents.
- **Regions** — sub-areas of countries that can be ceded independently.
- **Weather Areas** — six, each defined as a list of countries: Central, Desert, North,
  North Monsoon, South, South Monsoon. Some hexes belong to none.
- **Naval Zones** — bounded by naval zone border hexsides, containing both All-Sea and
  coastal Land hexes, with a defined adjacency relation and three state boxes each.
- **Strategic Hex sets** — three, by colour, used for victory scoring.
- Named **map-edge spans**, such as `w5311`–`w6011`.

Note that the region layers **overlap**: a coastal Land hex can be in a country, a
dependent, a region, a weather area, and one or two naval zones simultaneously. These are
not a partition; they are five independent labellings.

### 4.6 Off-map spaces

Named Off-Map Boxes: **Western US**, **Panama Canal**, **French Polynesia**,
**Europe/Africa**, **Eastern Europe**. Each belongs to an Allied faction and only that
faction's units may enter.

These are unusual because they are **partly spatial**. An Off-Map Box is considered part
of a Naval Zone if port-to-port movement can reach it, and adjacent to another Off-Map Box
if direct transfer is permitted — both relations stated on the printed box. The Eastern
Europe Box is treated as coextensive with a named span of map-edge hexes for supply
tracing. So Off-Map Boxes need to participate in the adjacency and region machinery, not
sit outside it.

Other non-hex spaces: the Force Pool (one per faction), the Delay Box and Naval Warfare
Delay Box, the Strategic Warfare Box, the Ceded Lands Box, the Posture Display with its
per-country Posture Boxes, the War State Display, the European War Boxes, the Government
Holding Box, and the tracks — Turn Track, VP Track, USCL Track, Current ESV Track, plus
three boxes per Naval Zone.

The Turn Track is again used as a **register**: adverse weather turns are printed on it,
and the V-J Day marker, Russian Entry marker, Enforced Peace markers and evacuated units
are all stored by placing a marker on a future turn space.

---

## 5. Library Implications

### 5.1 Queries the rules force

| Query | Where | Notes |
|---|---|---|
| Adjacency filtered by edge predicates | ZOC (8.0), movement, supply, retreat | mountain, all-sea, strait, lake hexsides each block a different subset |
| Weighted path search with hex **and** edge costs | hex-to-hex movement (3.1.1) | the TEC is explicitly two-part |
| Monotone-distance walk with priority fallback | retreat (4.2.5.1) | each step must be strictly farther from the origin |
| Layered supply trace with per-segment budgets | supply (10.2) | two free hexes, then a road/rail network with a one-stretch road limit, then zonal sea legs |
| Region-gated traversal | Naval Zone supply and movement | each zone must hold a friendly convoy marker |
| Region adjacency via shared border edges | Naval Zone adjacency, country borders | region graph derived from an edge labelling |
| Priority-ordered attribute resolution | Control (glossary) | lowest matching priority wins |
| Compound derived predicates | Air Base, Naval Base, Open Port, Island, Blitz-enabled | worth memoising with explicit invalidation |
| Counterfactual combat evaluation during movement | overrun blocking (3.2) | "would the defender get any shift?" |
| Range-2 support with a path constraint | HQ ranged support (4.2.1.2) | path may cross enemy units but not certain countries |
| Cumulative shift with a min-selection sub-rule | terrain shifts (4.2.3.8) | only the **lowest** hexside shift applies |
| Constraint-satisfying selection from a hand | option card selection (1.1.3) | requirements, colour gating, sequential and annual limits, budgets |
| Ordered-constraint loss assignment | attacker first loss (4.2.6.1) | derived from force composition |
| Multiset constraint with nationality classes | stacking (9.1, 9.2) | count, step total, and a country-composition rule |

### 5.2 Data structures

- **Hex attribute table** keyed by (sheet, number): terrain, settlement class, port class,
  road and rail flags, oil, limited stacking, strategic-hex colour, aid/impact flags.
- **Edge attribute table**: mountain, river, lake, all-sea, strait (connected /
  unconnected), naval zone border, country border, region border. Eight distinct labels —
  the largest edge table of the three games.
- **Road/rail graph**: an explicit network over hexes, plus the connected-strait links
  that join otherwise separate components.
- **Region tables**, five independent labellings over hexes: country, dependent, region,
  weather area, naval zone (possibly multi-valued).
- **Political state**: per country — status, alignment, posture, policy markers, truce
  markers, government marker side. Mutable, and read by dozens of rules.
- **Naval Zone state**: three boxes per zone, each a multiset of support units and convoy
  markers.
- **Card state**: per faction — deck, hand, Current Card, Pending Card, discard pile, plus
  the record of which cards have been played, which drives selection requirements.
- **Counter state**: ground units with three factors and a step count; support units;
  markers with their own type-specific state, including the Beachhead marker's facing and
  the Airdrop marker's origin.
- **Boxes and tracks**: Force Pool, Delay Box with per-counter countdown, Strategic
  Warfare Box, Ceded Lands Box, Posture Display, Turn Track carrying several markers as
  scheduled events.

### 5.3 Where the game will not fit a shared abstraction

Dai Senso will stress a general library in three places.

**The political layer has no analogue in an ordinary hex wargame.** Country status,
alignment and posture change during play and are consulted by movement, ZOC, combat,
supply and victory. A library cannot bake "which hexes are friendly" into the map. What it
can offer is a general **mutable per-region attribute table** with a query interface, and
let the game supply the semantics.

**The naval layer is a second spatial system.** Fleets and convoys do not occupy hexes;
they occupy region-level boxes whose contents gate traversal of the region. The general
shape is "a partition of the map into regions, each with mutable occupancy that acts as an
edge gate on paths crossing it." That is worth having in the library, because it
generalises to any game with sea zones, weather zones or air-superiority zones.

**The card system is selection, not shuffling.** Cards live in a hand and are chosen under
constraints a season ahead. A deck abstraction built around drawing at random — which is
what a Tarawa-style game needs — is the wrong shape here. The two want a common *card*
type but different *container* types.

### 5.4 Ordering hazards

- Retreat results are always applied **before** attrition results (4.2.4), and the rulebook
  italicises them on the CRT as a reminder.
- Attrition is applied to the attacking force **first**, then the defending force (4.2.2
  step 7).
- Within the Blitz Combat Segment, all airdrops finish before any blitz combat, and all
  blitz combat before any beachhead landing (4.1).
- Stacking is enforced at the end of **every** phase and segment, not only at end of turn.
- Supply is evaluated at the moment it is needed, so a unit that cannot move early in the
  Operational Movement Phase may become able to move later in the same phase (10.2).
- A defending HQ that provided ranged support to an earlier combat still contributes its
  factor and shift when its own hex is attacked (4.2.3.5), but may not support a *third*
  combat.
- Units swept into a retreating stack count as having participated in the original combat,
  which can retroactively disqualify an HQ from supporting later combats (4.2.5).
