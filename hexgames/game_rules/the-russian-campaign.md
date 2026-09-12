# The Russian Campaign — Rules Digest for Library Design

**Source.** *The Russian Campaign*, Deluxe Fifth Edition, John Edwards and Todd
Davis, Consim Press / GMT Games, 2022. Primary source:
`TRC v5 deluxe/The Russian Campaign 5th Rules - GMT 2022.pdf` (36 pp.).
Charts from `TRC5_CRT_SOP_and_Replacements_Ref_v4.pdf` and from the map itself.
Sequence checked against `THE_RUSSIAN_CAMPAIGN_5th_Edition_rules_summary_V1.03.pdf`.
Map: `TRC v5 deluxe/TRC5 map adjusted.png` and `TRC5 map resized.png`.

Earlier editions (3rd, 1977; 4th) are present in the source folder but are not
described here.

This is a summary written for someone designing a reusable C++ game library. It
records structure, not every clause. Where a rule is condensed, that is stated.
Rule numbers in parentheses refer to the fifth-edition rulebook.

---

## 1. Game Overview

### 1.1 Subject and scale

The game covers the whole German–Soviet war, from Operation Barbarossa in June
1941 to the fall of Berlin in 1945. The map runs from Berlin and the Carpathians
in the west to Gorki, Saratov and the Caspian in the east, and from the White Sea
to the Black Sea.

| Quantity | Value |
|---|---|
| Hex | roughly 40 miles across |
| Game turn | two calendar months |
| Campaign length | 25 game turns, May/June 1941 through May/June 1945 |
| Unit | army or corps; a few divisions, plus HQs, leaders, workers, partisans, air units |
| Map extent | 43 lettered hex rows, `A` through `QQ`, running north to south |

### 1.2 Players

Two players, Axis and Russian. The asymmetry is in the order of battle and in a
long list of national rules, not in the mechanics: both sides move, fight and
supply by the same rules. Exceptions are numerous but local — the Russians may not
make automatic-victory attacks before November/December 1942 (16.4); only the Axis
convert rail westward; only the Russians have partisans, paratroops and worker
units.

This is the most conventional of the three games examined. It is the natural
baseline for what "standard elements" means.

### 1.3 Sequence of play

Each game turn is: an Axis player turn, then a Russian player turn, then a Sudden
Death check. Each **player turn** consists of **two impulses**, and each impulse is
a movement phase followed by a combat phase.

1. **Weather Phase** (Axis rolls, once per game turn, for both sides).
2. **Axis First Impulse Movement Phase** — withdrawals first; then normal, rail and
   sea movement; reinforcements; replacements; automatic-victory attacks; partisan
   removal.
3. **Axis First Impulse Combat Phase** — Stukas may be committed here only.
4. **Axis Second Impulse Movement Phase** — only units with second-impulse capability;
   no rail movement, no air, HQ replacements only.
5. **Axis Second Impulse Combat Phase.**
6. **Axis Player-Turn End Phase** — advance Axis railheads, *then* eliminate
   out-of-supply Axis units.
7. **Russian First Impulse Movement Phase** — as above, plus paratroop drops on snow
   turns and replacement purchase.
8. **Russian First Impulse Combat Phase** — Sturmoviks committed here only.
9. **Russian Second Impulse Movement Phase.**
10. **Russian Second Impulse Combat Phase** — then partisans relocate.
11. **Russian Player-Turn End Phase** — push back Axis railheads, eliminate
    out-of-supply Russian units, resolve Axis minor ally surrenders, advance the
    turn marker.
12. **Sudden Death Victory Check**, at the end of each January/February turn only.

The two-impulse structure is the spine of the game. A unit's *type* determines
whether it may move in the second impulse at all, which is what makes armour
qualitatively different from infantry rather than merely faster.

### 1.4 Victory

Two paths (25.0).

**Campaign victory.** The Axis wins immediately by controlling Moscow *and*
eliminating Stalin, or if the Russians do not control Berlin at the end of the
May/June 1945 turn. The Russian wins immediately by controlling Berlin at any
point. Either side loses immediately by failing to match units their opponent
exited off a board edge (20.10).

**Sudden death victory.** Checked at the end of each January/February turn. A player
wins immediately by holding *all* of that year's listed objectives — a fixed list of
six named city or oil-field hexes per year, or in 1945 a mix of hexes and political
events such as the surrender of Finland.

| Year | Objectives |
|---|---|
| 1942 | Kiev, Kalinin, Leningrad, Rostov, Kharkov, Stalino |
| 1943 | Maikop Oil Fields, Moscow, Stalingrad, Kursk, Leningrad, Rostov |
| 1944 | Leningrad, Smolensk, Kiev, Dnepropetrovsk, Sevastopol, Kharkov |
| 1945 (Russian) | Surrender of Finland, Rumania and Hungary; one German city; all oil wells |
| 1945 (Axis) | Prevent Rumanian and Hungarian surrender; all German cities; one oil well |

### 1.5 What is structurally distinctive

Three things. First, the **two-impulse player turn**, which makes exploitation a
structural feature rather than a special rule. Second, **rail conversion**: the rail
network is a mutable, side-owned resource whose frontier advances and retreats
during play, and it is simultaneously the movement network and the supply network.
Third, the **automatic victory**: any attack reaching 10-to-1 odds removes the
defender during the *movement* phase without a die roll, which turns overwhelming
local concentration into a movement-unblocking tool rather than merely a good
combat result.

---

## 2. Standard Elements

### 2.1 The hex grid

A single contiguous grid over land and water. Sea and lake hexes may not be
entered by ground units at all; they are crossed only by sea movement, and air
units measure range across them "as if the hexagonal pattern existed at sea"
(15.4).

Hexes are identified by a **letter and a number**: `F18`, `S11`, `T14`, `AA29`,
`EE20`, `KK19`, `QQ5`. Letters past Z are doubled (`AA`, `BB`, … through `QQ`), so
the letter component is a string, not an integer, and its ordering is by length then
lexicographically.

The rules use the **letter** to name a hex row in a north–south sense: Finnish units
"may move into but not south of the `H` hex row" (26.6.1); Italian, Hungarian and
Rumanian units may not move "north of the `L` hex row" (26.6.2); and one Russian
reinforcement per turn may enter from the **south edge**, `QQ5` to `QQ16` inclusive
(20.5). So `A` is the northern end of the letter range and `QQ` the southern, and the
numeric component indexes the other axis. Which end of the numeric range is west is
not established here — the printed hex numerals are too small to read at the
available scan resolution.

A hex row is therefore itself an addressable region, named by its letter.

### 2.2 Counters

A unit counter carries: nationality colour, formation colour, unit-type symbol,
unit-size symbol (corps `XXX`, army `XXXX`, army group `XXXXX`, high command
`XXXXXX`), unit identification, **combat factor**, **movement factor**, and either
a set-up location or a reinforcement turn.

- The **combat factor** is used unchanged whether attacking or defending (2.4).
- The **movement factor** is the number of hexes over clear terrain in clear weather
  in the first impulse (2.5).
- The **unit type** determines second-impulse movement and terrain behaviour (2.6).

Unit types: infantry, armour/panzer, motorized/mechanized, panzer grenadier,
cavalry, mountain, paratroop, Luftwaffe ground, SS, Guards, partisan, worker,
battle group, HQ, leader, air.

Markers track state that is not a unit: weather, weather DRM, railhead, RR convert,
RR move, invasion count, turn, scenario end, sudden death objectives, worker
replacement points, para range, fortress city, artillery barrage, RP bid.

Two of these are worth noting for a library. The **weather DRM marker** and the
**worker RP marker** are both stored by *placing a marker on a numbered track* — the
track space is the value. The library will want an explicit "value held on a track"
abstraction rather than pretending these are units.

### 2.3 Zones of control

Textbook, with two exceptions that are exactly the interesting ones (7.0).

A unit's ZOC is the hex it occupies plus the six adjacent hexes. It extends into
every terrain type and into hexes occupied by enemy units. **It does not extend
across an all-water hexside of a lake or sea, nor across a hexside with a white
dashed line** (a *blocked hexside*), nor across the Kerch Strait (8.5.2). Partisans
project a ZOC only into their own hex (7.3), as do battle groups under the optional
rule (26.4.1).

Effects:

- A moving unit must stop the moment it enters an enemy ZOC (7.2), except during an
  automatic victory (16.1).
- A unit may not move directly from one enemy ZOC to another (8.3). A unit starting
  its first-impulse move in an enemy ZOC may exit to a ZOC-free hex and then move
  into a ZOC again in the same impulse. A unit in an enemy ZOC at the start of the
  *second* impulse may not move at all that impulse (8.4). This is the "pinning"
  rule, and it is a genuine trap: three units can be immobilised entirely.
- A unit may not retreat into an enemy ZOC (13.5).
- Rail movement may not start in, pass through, or end in an enemy ZOC (9.3.2).
- Supply may not be traced *through* an enemy ZOC, though the traced-from unit and
  the source city may themselves be in one (17.2.3).
- Rail conversion may not occur in or through a hex in an enemy ZOC, except a
  friendly-controlled city (9.4.1).

A ZOC is **contested** when both players project into the same hex (7.4). A vacant
contested hex is controlled by neither side. An enemy unit must stop on entering an
enemy ZOC whether contested or not.

So ZOC is not one predicate but a family: *is this hex in an enemy ZOC*, *is it
contested*, *does a ZOC extend across this particular hexside*. The last of these is
edge-indexed, not hex-indexed.

### 2.4 Movement

Each unit moves a number of hexes equal to its movement factor as modified by
weather and impulse. Units move in any direction, may pass through and stack on
friendly units, and may not transfer or bank movement points (5.3).

The actual allowance is read from the **Movement Allowance Chart**, a table indexed
by *unit type* × *weather* × *impulse*. That chart is on a separate player aid card,
not present in this source folder; its structure, not its numbers, is recorded
here. Two properties are visible from the rules text: some unit types have **no**
second-impulse movement at all (Russian infantry, Russian artillery under the
optional rule), and HQ units are the reverse — they move only in the second impulse,
at full allowance, unaffected by weather (11.1).

There are three additional movement modes.

**Rail movement** (9.0). First impulse only; unlimited distance; the unit may do
nothing else that impulse. The path must be a contiguous chain of *friendly*
railroad hexes, and the unit must start and finish on one. It may not start in,
pass through, or enter an enemy ZOC. Capacity is six units per turn for the Axis
(three in snow) and five for the Russians, with arriving reinforcements and
replacements railing free on their turn of arrival. Terrain is ignored entirely
during rail movement; conversely, normal movement may not use rail lines to escape
terrain effects (9.3.3). Off-board rail movement is permitted between exit hexes on
the same friendly board edge (9.3.4).

**Sea movement** (10.0). Movement factors play no role. Two disjoint sea areas — the
Black Sea/Sea of Azov and the Baltic — with no movement between them in one
impulse, and no movement at all in the Caspian. One attempt per turn per sea area
per player. Three kinds: **transfer** port to port, **invasion** onto any
non-enemy-occupied coastal hex, and **evacuation** from any coastal hex to a friendly
port or to the Off-Map Units Box. Success requires a die roll, modified by −1 for
each friendly-controlled named port in that sea (Odessa, Sevastopol, Rostov in the
Black Sea; Riga, Tallinn, Helsinki, Leningrad in the Baltic), +1 for evacuation, and
+2 for any Russian move in the Baltic. Failed units go to the replacement pool, not
out of the game.

**Paradrop** (18.0). Russian only; snow turns only; within eight hexes of Stavka;
not into enemy ZOC, woods or mountains; no further movement that turn; and a
paratroop corps may drop only once ever.

### 2.5 Discrete terrain types

The Terrain Effects Chart, transcribed from the map:

| Terrain | Movement | Combat |
|---|---|---|
| Clear | 1 MP to enter | — |
| Woods | 1 MP; all units must **stop** on entry, except infantry, mountain, paratroop and Luftwaffe infantry | attackers and defenders do not retreat — AR and DR become C |
| Mountain | 1 MP; all **non-mountain** units must stop on entry | defending combat strength doubled |
| Swamp | 1 MP; **all** units must stop on entry; treated as clear during Snow | — |
| River | use the other terrain in the hex for MP cost | defending strength doubled if all attackers are on river hexes and the defender is not on a river hex of the same river |
| Major city | use other terrain for MP cost | defending strength doubled |
| Minor city | use other terrain for MP cost | — |
| Oil fields | use other terrain for MP cost | — |
| Blocked hexside | movement across prohibited; ZOCs do not extend across | — |
| Sea or lake hex | entry prohibited; crossed only by sea movement | — |
| Kerch Strait | units must stop in the first hex entered after crossing | defending strength doubled if all attackers are across the strait |

The important observation for a library: **every land hex costs exactly one
movement point**. TRC has no variable terrain cost at all. What terrain does instead
is (a) force a stop on entry, for some unit types, and (b) double the defender. This
is the same shape as Tarawa's movement model — a stopping predicate, not a cost
function — reached from the opposite direction.

Two further effects worth separating out. Woods block retreat, converting two combat
results into a third (14.2). And a defender's combat factor may never be more than
doubled, while an attacker's is never doubled at all (14.1.2) — so the modifiers
saturate rather than compound.

### 2.6 Supply

Two kinds (17.0).

**General supply** is checked in the owning player's end phase; every unit not in
general supply is eliminated. A unit must trace a line of at most **eight hexes** to
a supply source — reduced to **four hexes** during snow months. Exceptions:
paratroops, partisans, and units that conducted a sea invasion this turn.

Supply sources are:

- a friendly-controlled city (17.2.1); and
- a friendly rail hex, provided a continuous chain of friendly rail hexes connects
  it to a friendly-controlled city or to a rail hex on that player's own board edge
  — west for the Axis, east or south for the Russians (17.2.2).

The traced line may not pass through an enemy ZOC or an enemy-controlled city,
though the tracing unit and the source city may themselves be in an enemy ZOC
(17.2.3). Supply may be traced *into* but not *through* a partisan hex (19.2), and
may be traced across the Kerch Strait (8.5.3).

**Combat supply** is checked at the moment of combat and only bites in specific
winters. Russian and Finnish units always have it. Non-Finnish Axis units in Russia
have it during snow turns of the first two winters only if in, or adjacent to, an
Axis-controlled city; the 1942–43 version widens this by one hex through a hex free
of Russian ZOC (17.3.2, 17.3.3). A unit without combat supply has its combat factor
**halved, rounded up per unit**, and *then* doubled for terrain if applicable — so a 5
becomes 3 and then 6 (17.3.1).

The rounding order matters and is a good example of a rule a library cannot express
by keeping a single "modifier stack": halving and doubling do not commute here.

### 2.7 Combat and the CRT

Combat is compulsory: any unit beginning a combat phase in an enemy ZOC must attack
(12.1). Attacks are declared as **discrete battles**, and for a battle to be legal
every attacking unit must be adjacent to every defending unit (12.3). The attacker
must attack every defending unit whose ZOC he is in, and every one of his units in
an enemy ZOC must attack.

Odds are the ratio of total attacking combat factors to total defending combat
factors, reduced to the simplest ratio and **rounded in the defender's favour**
(13.1). An attacker may voluntarily announce lower odds before rolling. Attacks at
worse than 1-6 are illegal; a unit that cannot make a legal attack **surrenders**
(12.5). Deliberately attacking at poor odds to improve odds elsewhere is called
soaking off and is explicitly legal. No unit, attacking or defending, may fight more
than one battle per impulse (12.4).

The Combat Results Table is a nine-column, six-row table read with one die:

| Die | 1-5, 1-6 | 1-3, 1-4 | 1-2 | 1-1 | 2-1 | 3-1 | 4-1 | 5-1, 6-1 | 7-1, 9-1 |
|---|---|---|---|---|---|---|---|---|---|
| 1 | AE | AE | AE | A1 | A1 | AR | C | EX | DR |
| 2 | AE | AE | A1 | A1 | AR | C | EX | DR | D1 |
| 3 | AE | A1 | AR | AR | C | EX | DR | D1 | DE |
| 4 | A1 | AR | AR | C | EX | DR | D1 | DE | DE |
| 5 | AR | AR | C | EX | DR | D1 | DE | DE | DS |
| 6 | AR | C | EX | DR | D1 | DE | DS | DS | DS |

Results (13.3):

| Code | Meaning |
|---|---|
| `AS` | All attacking units surrender |
| `AE` | All attacking units eliminated |
| `A1` | One attacking unit of the attacker's choice eliminated; all attackers retreat one or two hexes |
| `AR` | All attacking units retreat one or two hexes |
| `C` | Contact — no loss or retreat by either side |
| `EX` | Attacker loses one involved unit of his choice, then the defender loses one of his; remaining defenders retreat one or two hexes |
| `DR` | All defending units retreat two hexes |
| `D1` | One defending unit of the defender's choice eliminated; the rest retreat two hexes |
| `DE` | All defending units eliminated |
| `DS` | All defending units surrender |

The **elimination/surrender distinction** is a real state difference, not flavour:
eliminated units go to the replacement pool and can come back; surrendered units go
to a Surrendered Units box and are gone permanently.

### 2.8 Modifiers — odds shifts, not die-roll modifiers

TRC almost never modifies a die roll in combat. It **shifts the odds column**.

- A German Stuka raises the odds by **three** columns: a 3-1 becomes a 6-1 (15.5).
  Only one Stuka per attack; each German Army Group HQ supports one.
- A Russian Sturmovik raises the odds by **one** (15.7). Any number may be applied
  to one battle.
- The optional artillery rule raises the odds by one per artillery unit (26.5.3).
- Air and artillery combined may never shift more than **three** levels total
  (15.10, 26.5.3) — the shifts saturate.

Air power requires all defenders in the battle to be within eight hexes of a single
friendly HQ (15.2); Sturmoviks trace from Stavka, or from Stalin if Stavka is gone
(15.8). Air availability varies by year and weather and is read from an Airpower
Availability Chart printed on the map: Stukas 3/1/1 by clear/light mud/mud in 1941,
falling to 1/0/0 by 1943 and none afterwards; Sturmoviks appearing in July/August
1943 at 1/0/0 and rising to 3/1/1 by 1945. Neither is available on snow turns.

Air power **can** contribute to reaching the 10-1 threshold for an automatic victory
(15.1, 16.1).

### 2.9 Retreat and advance

Retreat is the main mechanism by which ground changes hands, and its rules are
detailed (13.4–13.7):

- The **attacker** moves retreating units of *both* sides and chooses the retreat
  length within the allowed range.
- The attacker may not route the defender into elimination if an alternative exists.
- A defender retreated two hexes may end only one hex from its start — the retreat is
  a walk, not a displacement.
- Units may not retreat into an enemy ZOC, off the map, across a coastline (except
  the Kerch Strait), or across an impassable or all-water hexside.
- Retreat ignores movement costs and terrain restrictions.
- Stacked units retreat individually.
- If no legal path exists, the unit is eliminated.
- Excess units that cannot meet stacking after retreat are eliminated, chosen by
  their owner.
- Woods forbid retreat entirely, converting `AR` and `DR` into `C` (14.2).
- Leaders who retreat surrender (11.3); workers who retreat surrender (22.2);
  artillery under the optional rule is eliminated if forced to retreat (26.5.5).

There is **no advance after combat** in TRC. Ground is taken by moving into it next
impulse.

### 2.10 Stacking

Two army-sized units per hex, or three corps-sized units, but a mixed stack of corps
and armies is limited to two (6.1). Limits may be exceeded during movement but are
enforced at the end of each movement phase and after each combat is resolved (6.2).
Markers, Army Group HQs, Stavka, workers, Hitler, Stalin and the 2-7 SS Reserve have
no stacking value (6.3).

So the stacking predicate is not a simple count: it is a small constraint over the
multiset of unit sizes present.

### 2.11 Reinforcement and replacement

**Reinforcements** arrive on a schedule printed on the order-of-battle chart: a
turn, and either a named city or a board edge. Units listed for the first month of
the turn arrive in the first impulse; the rest in the second (20.0). A unit whose
named city is enemy-controlled, or whose arrival would break stacking, enters from a
board edge instead (20.4). Arrival hexes do not cost movement.

The **Off-Map Units Box** is a strategic reserve into which either player may
voluntarily divert arriving units, or rail units off a friendly board edge, or move
units by sea (20.1). Units in the OMB may enter on any later impulse but forfeit the
free rail move.

**Axis replacements** are available only on the May/June turns of 1942, 1943 and
1944, and are drawn from a pool of *eliminated* (not surrendered) units. What may be
replaced is a fixed shopping list with substitutions: one German armour corps **per
oil well the Axis controls**; all SS, Luftwaffe and HQ units; one each of the 3-4,
4-4 and 5-4 German infantry corps; one mountain corps; one motorized corps; and one
unit from each of the four minor allies. Unused replacements are lost.

**Russian replacements** are a **point economy**. The Russian replacement total each
turn is the sum of the replacement values of all available **worker units**, plus an
**Archangel** die roll representing Lend-Lease from January/February 1942 onward.
Worker replacement values double from May/June 1943 (22.7). The Russian spends points
to rebuild units from the pool whose combined combat factors do not exceed the total,
subject to a cap of one armour and one Guards unit per turn, rising to two of each
from January/February 1944. Unused points are lost.

Worker units are themselves on the map: they sit in Russian cities, cannot move once
placed, have no stacking value, project a ZOC, can attack and defend, and always
surrender rather than being eliminated (22.2). A worker in a surrounded city still
generates points. Capturing a worker's city is therefore how the Axis attacks the
Russian economy — the economic model is spatial.

### 2.12 Weather

One roll per game turn, made by the Axis player, applying to both players (4.1.1).
May/June and July/August are automatically Clear; January/February is automatically
Snow. The other three turns of the year roll on a chart indexed by month.

States: Clear, Light Mud, Mud, Snow.

The distinctive part is the **cumulative weather DRM**. Each chart entry yields both
a weather state and a modifier which is *added to a running DRM carried from turn to
turn*, and that DRM adjusts the next roll. Clear results push the DRM up (making bad
weather likelier next time); Mud and Snow results push it sharply down. Rolls are
clamped to the range 0–7. The mechanism is a self-correcting Markov chain with
memory, not an independent draw.

Weather affects: movement allowances via the MAC; rail capacity (Axis six units drops
to three in snow); air availability; swamp terrain (clear in snow); combat supply
(only in snow); supply range (eight hexes drops to four in snow); and paratroop drops
(snow only).

---

## 3. Unique Exceptions

### 3.1 The two-impulse player turn

Each player turn is two full movement-plus-combat cycles. The second impulse is
restricted:

- only units whose type grants second-impulse movement may move;
- no rail movement, no air power;
- only HQ replacements (Axis) or the Stavka replacement (Russian);
- a unit that began the impulse in an enemy ZOC may not move at all (8.4);
- HQ units move **only** in the second impulse, at full allowance, ignoring weather.

The consequence for a library is that an "impulse" is a first-class scheduling unit
with its own capability mask, and unit capability is a function of (type, impulse,
weather) rather than a fixed number.

### 3.2 Automatic victory attacks

Any attack that reaches **10-to-1 odds** removes the defender without a die roll,
during the movement portion of the attacker's turn (16.1). This is not a CRT result;
it happens before the combat phase exists.

Its purpose is to unblock movement. Units that did not take part may then move
through or onto the vacated hex; ZOC pinning evaporates; rail movement becomes legal
through the freed hex (9.3.2). The cost is that participating units may not move
further that impulse and may not attack in the combat phase (16.2), and if they used
a first-impulse AV they may not enter an enemy ZOC in the second impulse or attack in
the second combat phase.

There is a trap attached (16.3). If AV units end the first impulse adjacent to enemy
units, other friendly units **must** be brought up to attack those enemies in the
second impulse; otherwise the AV units are stuck with a mandatory attack they cannot
legally make, and **surrender**.

Russians may not conduct AV attacks before November/December 1942 (16.4).

### 3.3 Rail conversion — a mutable, side-owned network

The two sides used different rail gauges, so a rail hex is usable only after it has
been converted. A **railhead marker** marks the limit of each side's usable rail.

The Axis advances its railheads in its own end phase to the farthest rail hex
occupied or passed through by an Axis unit that turn — but only if, **both at the
moment of occupation and again during the end phase**, a path free of Russian ZOC and
Russian-controlled cities could be traced along the railroad back to an Axis city or
the west edge (9.4.4). The Russian pushes Axis railheads *back* in his own end phase
by the mirror-image rule (9.4.5).

Additional rules layer on:

- Rail hexes must have been friendly *prior to* the turn of use (9.4.1).
- The phasing player can never lose control of a rail hex during his own turn.
- Conversion may not occur in or through a hex in an enemy ZOC — **except** a
  friendly-controlled city hex, which converts anyway.
- Controlling a city converts any rail in that hex. Controlling two cities with no
  enemy units, enemy ZOCs or enemy-controlled cities on the rail line between them
  converts **the entire line** at once (9.4.3).
- A **rail junction** — a non-city rail hex where rail lines intersect — is controlled
  as if it were a city (9.4.2), and control of a city or junction can be gained
  merely by having it in one's uncontested ZOC (17.2.1).
- One named rail segment, GG19–HH21, may be used for rail movement only; normal
  movement and supply tracing may not use it (8.6).

This is the most demanding subsystem in the game for a library. The rail network is
simultaneously a movement graph, a supply graph and an ownership state that mutates
twice per game turn under a two-point-in-time validity test.

### 3.4 Control of points, gained by uncontested ZOC

A player controls a city — or an oil well, or a rail junction — if he occupies it,
**or** if it is out of enemy ZOC and he was the last to occupy it or have it in his
uncontested ZOC (17.2.1). A vacant hex in both players' ZOC is controlled by neither.
Control can change during movement or combat, in either impulse of either player's
turn, and is explicitly frozen while combat losses are being applied and re-evaluated
afterwards (13.3, `EX`).

So control is a *stateful* attribute with a last-toucher rule, not a function of the
current position. It needs a stored owner per controllable hex, updated by an event,
not recomputed from scratch.

### 3.5 Political events and national surrender

Four Axis minor allies each have their own surrender trigger (24.0):

- **Hungary** surrenders when five or more Russian units are in Hungary at the Russian
  end phase.
- **Finland** and **Rumania** surrender when their capitals — Helsinki and Bucharest —
  are Russian-controlled at the Russian end phase.
- **Italy** surrenders automatically at the start of September/October 1943.
- **Finland** additionally surrenders at the start of each Axis turn from
  September/October 1944 unless the Axis control Leningrad; on surrender, Helsinki
  becomes permanently Russian, neither player may enter Finland again, and all Axis
  units in Finland are eliminated.

Surrender removes every unit of that nation, on the map and off. Surrenders are also
victory objectives in 1945.

Alongside these are **triggered garrison releases**: a Russian unit moving within
five hexes of Bucharest from 1943 summons named Axis mountain corps to Bucharest on
the next Axis impulse (20.7); within two hexes of Warsaw from 1944 summons the 4th
SS and Hermann Goering panzer corps (20.8). These are geometric triggers with
scheduled effects — a proximity predicate that fires once and queues a reinforcement.

### 3.6 Off-board encirclement matching

From 1942, if one player exits units off an *enemy* board edge, the other player must
on his next turn remove an equal or greater number of combat factors, and may place
no arriving units until the debt is cleared (20.10.1). Failure to match loses the game
immediately (20.10.2).

This exists because the map is finite and a deep flanking manoeuvre would otherwise
be impossible to represent. It is a rare example of a rule whose state is a **scalar
debt** owed between players.

### 3.7 Leaders and headquarters

- **Hitler** and **Stalin** are leader counters that may move only by rail or sea. If
  either is eliminated, every unit of that country has a movement factor of **zero**
  for the next impulse; rail and sea are unaffected, and Axis minor allies are exempt
  (11.3). If forced to retreat, they are eliminated.
- **Army Group HQs** (three German) and **Stavka** (Russian) move only in the second
  impulse, at full allowance, ignoring weather, and may not enter an enemy ZOC unless
  a friendly non-HQ unit already occupies the destination (11.1). Each German HQ
  supports one Stuka. Stavka is the range origin for Sturmoviks and for paratroop
  drops.

HQs are therefore **range anchors** for capabilities, not command-radius modifiers in
the Tarawa sense.

### 3.8 Partisans

Three Russian partisan units, placed in Russia in an Axis-controlled city or on an
Axis rail hex, not in Axis ZOC and not within five hexes of an SS unit (19.1). A
partisan has a ZOC only in its own hex. That ZOC blocks rail movement, forces normal
movement to stop, blocks retreat into the hex, and inhibits Axis rail conversion —
but does **not** confer control of a city or junction, and does not support Russian
rail conversion (19.2). Supply may be traced into but not through a partisan hex, and
an Axis city with a partisan in it is still an Axis supply source.

Partisans cannot be permanently eliminated: any in Axis ZOC or within five hexes of
an SS unit are removed at the end of each Axis movement phase, and all are relocated
at the end of the Russian second impulse (19.3, 19.4).

A partisan is thus a **recurring, non-destructible obstacle token** whose effects are
a carefully chosen subset of the ordinary ZOC effects. A library that implements ZOC
as one boolean will not be able to express it.

### 3.9 The Kerch Strait and coastlines

Units may not cross **black coastal lines** except at the Kerch Strait, between hexes
KK19 and KK20 (8.5). Units crossing the strait move normally up to the first hex
across and must then stop for the rest of the impulse. Zones of control do not extend
across the strait; attacks across it are voluntary for the attacker; the defender is
doubled if all attackers are across it; supply may be traced across it; and retreat
across it is permitted.

This one paragraph attaches five different rule effects to a single hexside.

### 3.10 Notable optional rules

Not detailed here, but worth knowing about because each stresses a different part of
a library design:

- **Fortress cities** (26.9) make combat *voluntary* in specified hexes and add a
  "lose a unit instead of retreating" option.
- **Battlegroups** (26.4) create a replacement counter in place of an eliminated
  defender, so `EX`, `D1` and `DE` results can leave a residue on the map.
- **Strategic movement** (26.10) doubles movement for units far from the enemy —
  defined as not in an enemy ZOC and not adjacent to enemy rails, cities or oil wells,
  which is a proximity predicate over three different feature classes.
- **Industrial evacuation** (26.2) moves worker units off the east edge onto the turn
  track, where they sit for three turns before reappearing in a Urals Industry box.
- **Competitive bidding** (26.8) distributes a bid quantity of replacement points
  across turns.

---

## 4. Map and Coordinate Notes

### 4.1 Hex numbering

**Row letter + number**. The letters run `A` … `Z`, then `AA`, `BB`, … `QQ`, and name
hex rows from north (`A`) to south (`QQ`) — see §2.1 for the rule citations that
establish this. Examples from the rules: `F18`, `I15`, `S11`, `T14`, `V11`, `W13`,
`X21`, `Y22`, `Z22`, `AA29`, `EE20`, `FF21`, `GG19`, `HH21`, `JJ22`, `KK19`, `KK20`,
`QQ5`–`QQ16`.

The doubled letters are the important wrinkle: the printed identifier is a string
whose ordering is by length first, then alphabetically. Any bijection to an internal
coordinate must encode that, and **hex rows are addressable as regions** — the
optional national restrictions name the `H` and `L` hex rows directly, and the
Russian southern entry rule names the span `QQ5`–`QQ16`.

### 4.2 Hex-centred features

- Terrain type: clear, woods, mountain, swamp, sea, lake.
- River-hex flag, plus the **identity** of the river (14.1.1 requires "a river hex of
  the *same* river").
- City, and its class: major or minor.
- Oil field.
- Port (for sea movement), with the four Baltic and three Black Sea named ports
  singled out as die-roll modifiers.
- Railroad presence, and whether the hex is a **rail junction** (a non-city rail hex
  where lines intersect).
- Control ownership, for cities, oil wells and rail junctions — mutable state.
- Railhead position — a marker occupying a rail hex.
- Setup identifiers: `C` for a specific city, `R` Rumania, `F` Finland, `N`/`C`/`S`
  German army groups.
- Sea-hex range dots, which mark hexes usable for measuring air range over water but
  whose absence does not forbid it (15.4).

### 4.3 Hexside (edge) features

This map is the clearest of the three for edge-attached data.

- **Rivers.** Drawn along hexsides, following the hexside network. The combat rule is
  expressed in terms of *river hexes*, but connectivity between two river hexes is
  defined by whether "a river crosses the hexside between the two hexes" (14.1.1). So
  a correct implementation needs both the hex membership *and* the set of edges the
  river occupies, tagged with the river's identity. Partial river hexes count as
  river hexes.
- **Blocked hexsides.** Drawn as a white dashed line. Movement across is prohibited
  and ZOCs do not extend across (7.1, TEC).
- **All-water hexsides** of lakes and seas. ZOCs do not extend across these either
  (7.1). This is distinct from the sea hex itself: the *hexside* is the object the
  rule names.
- **Black coastal lines.** May not be crossed by ground movement at all, except at the
  Kerch Strait (8.5).
- **The Kerch Strait**, a single named hexside between `KK19` and `KK20`, carrying
  five distinct rule effects.
- **Country borders**, drawn as solid black lines along hexsides. They define the
  national regions used by surrender conditions and national restrictions.
- **Russian Military District boundaries**, drawn as dash-dot lines along hexsides.
  These partition the western Soviet Union into the Leningrad, Baltic, Western, Kiev
  and Odessa districts, which govern initial setup and first-turn attack permissions.

### 4.4 Centre-to-centre link features

- **Railroads.** Drawn hex centre to hex centre. This is the game's second graph, and
  it is not a subgraph of hex adjacency in any useful sense — it is a sparse network
  whose edges are what rail movement traverses and what supply chains follow. It also
  carries mutable per-hex ownership via the railhead markers, and off-map extensions
  where lines leave the board edges.

There are no roads. The rail network is the only centre-to-centre feature, and it is
central to the game.

### 4.5 Region features (sets of hexes)

- **Countries**: Germany, Poland, East Prussia (part of Germany for setup), Hungary,
  Rumania, Bulgaria, Finland, Russia. Membership drives surrender triggers, national
  movement restrictions, and worker placement.
- **Russian Military Districts**: Leningrad, Baltic, Western, Kiev, Odessa. Used for
  setup and for the first-impulse attack restriction, where each German army group
  may attack only units in its assigned district.
- **Sea areas**: Baltic; Black Sea and Sea of Azov together; Caspian (no movement).
  These are disjoint regions with their own port lists and die-roll modifiers.
- **Hex rows** addressed by letter, used by optional national restrictions.
- **Board edges**, each owned: west is Axis, east and south are Russian, north is
  neither for rail purposes.
- **Sudden death objective sets**, one per year — a named set of hexes plus, in 1945,
  non-spatial political conditions.

### 4.6 Off-map spaces

- The **Off-Map Units Box**, which behaves as a port bordering *both* sea areas and as
  a node reachable by rail from a friendly edge.
- Axis and Russian **replacement pools** (eliminated units, retrievable).
- Axis and Russian **surrendered units boxes** (permanent).
- **Stukas** and **Sturmoviks** boxes.
- **Paratroop Reserve** box.
- **Urals Industry** box (optional rule).
- The **Turn Record Track**, which does double duty: it holds the turn marker, the
  weather DRM as a track position, the worker replacement points as a track position,
  the invasion counts, the scenario-end marker, and units delayed by industrial
  evacuation.

The Turn Record Track is worth calling out. It is used as a **numeric register** in at
least three places. A library that models tracks only as "where the turn marker is"
will not cover it.

---

## 5. Library Implications

### 5.1 Queries the rules force

| Query | Where | Notes |
|---|---|---|
| Adjacency, hex distance | ZOC, range, supply | ordinary |
| Edge-blocked adjacency | ZOC across water/blocked hexsides, coastlines (7.1, 8.5) | adjacency must be filtered by an edge predicate |
| Bounded-length blocked path search | general supply, 8 or 4 hexes (17.2) | with an enemy-ZOC and enemy-city blocking set |
| Path search on a *separate* graph | rail movement and rail supply (9.3.2, 17.2.2) | the rail network, not hex adjacency |
| Reachability on the rail graph to a source set | rail supply (17.2.2) | source set is cities plus edge hexes |
| Frontier advance under a two-time validity test | rail conversion (9.4.4, 9.4.5) | validity checked at occupation *and* at end phase |
| Whole-line ownership flood | rail conversion between two controlled cities (9.4.3) | connected-segment fill |
| Region membership | countries, military districts, sea areas | authored partitions |
| Proximity predicate over a feature class | garrison releases, strategic movement, partisan placement | "within *n* hexes of any X" |
| Retreat path search with multiple exclusions | 13.5 | must also avoid creating stacking violations |
| Bipartite adjacency check | legal battle definition (12.3) | every attacker adjacent to every defender |
| Odds ratio with defender-favourable rounding | 13.1 | integer ratio reduction |
| Column shift with saturation | air and artillery (15.10) | shifts cap at three |
| Constraint over a multiset of unit sizes | stacking (6.1) | not a simple count |
| Last-toucher ownership update | city, oil well, junction control (17.2.1) | stored state, event-driven |

### 5.2 Data structures

- **Hex attribute table**: terrain, river flag and river identity, city and class, oil
  field, port, rail flag, junction flag, country, military district, setup code.
- **Edge attribute table**: river identity, blocked flag, all-water flag, coastline
  flag, country border, district border, Kerch Strait flag. The map has seven distinct
  edge-attached attributes; this table is not optional.
- **Rail graph**: an explicit undirected graph over rail hexes, with off-board edge
  terminals, plus a per-hex owner and railhead marker positions.
- **Control state**: owner per controllable hex, updated on occupation and on
  uncontested ZOC, frozen during combat resolution.
- **Unit state**: nationality, formation, type, size, combat factor, movement factor,
  and derived flags for second-impulse capability and terrain behaviour.
- **Pools**: replacement pool per side (retrievable), surrendered box per side
  (permanent), OMB, paratroop reserve, air unit boxes.
- **Turn state**: turn index, month pair, weather, cumulative weather DRM, rail
  capacity used per side, invasions used per side per sea area, off-board encirclement
  debt per side.
- **Schedules**: an order-of-battle table keyed by turn and impulse, and a set of
  event triggers keyed by geometric predicates.

### 5.3 What this game contributes to a shared design

TRC is the reference implementation of the conventional pattern, so most of its
machinery *is* the shared library:

- ZOC as a hex set derived from unit positions, filtered by edge predicates.
- Movement as a hex budget with entry costs and stopping rules.
- Supply as bounded blocked path search to a source set.
- Combat as odds-ratio into a table, with column shifts.
- Retreat as a constrained short path search.
- Stacking as a phase-boundary constraint.

Three things it needs that a minimal design would omit. **Edge attributes** are not a
convenience here; seven separate rules are edge-indexed. **A second graph** — the rail
network — is a first-class object with its own ownership state and its own search
problems. And **control as stored state** with a last-toucher rule cannot be recomputed
from the current position, so the library needs a place to keep it and an event hook
to update it.

### 5.4 Ordering hazards

Several rules are sensitive to evaluation order in ways worth encoding explicitly:

- Railheads advance **before** supply is checked in the end phase (4.1.6). Getting this
  backwards changes which units die.
- Combat supply halving happens **before** terrain doubling, and rounds up at the
  halving step (17.3.1).
- Control does not change while `EX` losses are being applied, only afterwards (13.3).
- Withdrawals are executed **first** in a movement phase, before any other activity
  (4.1.2).
- Partisan removal happens at the end of *each* Axis movement phase, both impulses.
