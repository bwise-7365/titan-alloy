Copyright Ben Paul Wise. All Rights Reserved.

# Wargame rulebook outline: what to include, and in what order

Written 2026-10-07 from a reading of seven rulebooks in `C:\Library\War-Games`. The purpose is a basic outline for writing the rules of a new hex-and-counter game: what sections a rulebook needs, what each section must cover, and the order that serves a first-time reader. Each section of the outline lists the required content and then the range of practice in the seven games, so that a writer can see the choices that have already been made and why.

The six sections that the request required (introduction, map, units, movement, combat, supply) are present as Parts 1, 3, 4, 6, 7 and 8. The other parts are additions that every one of the seven rulebooks needed in some form.

## Sources

| Game | Publisher, year | Situation | Material read |
|---|---|---|---|
| Axis Empires: Dai Senso! | Decision Games, 2011 | Historical, Asia-Pacific 1937-45, three factions | Living Rules (February 2014), Living Scenarios (October 2012), errata (October 2012), map image |
| D-Day at Tarawa | Decision Games, 2017 | Historical, Betio, 20-21 November 1943; solitaire | Rules V1.3, addenda (11/16/16), player-aid panel, USMC OOB chart, position list, fan flipbook (terrain chart only) |
| Panzergruppe Guderian | SPI, 1976 | Historical, Smolensk, July 1941 | Berger 2026 edition of the rules, original SPI rules, CRT, errata and addenda, amendments, Simonsen's analysis article, counter sheets |
| Stalin Moves West | Decision Games (World at War 58), 2018 | Hypothetical past: Soviet pre-emptive attack, 1941-42 | Rules V8F e-rules |
| The Russian Campaign, deluxe 5th edition | GMT/Consim, 2022 | Historical, Eastern Front 1941-45 | 5th edition rules, a reformatted version of the same, three fan aids, map images |
| Velikiye Luki, Stalingrad of the North | Legion Wargames, 2023 | Historical, November 1942 to February 1943 | Map, terrain chart and counter scans only (see note) |
| War in the Ice | SPI, 1978 | Future (from 1978): war in Antarctica, 1991-92 | Rules v1.0 with the Situation Briefing, chart sheet, map image, counter sheet |

**Note on Velikiye Luki.** The folder holds no rulebook. It holds the map, the terrain effects chart and the counter sheets of the Legion Wargames game (design Michael Taylor, 2023), and a designer's notes file, `VLDNotes.pdf`, that belongs to a different game: Dirk Blennemann's *Velikye Luki* (Moments in History, 2000, the "T3" system). The entries below for Velikiye Luki come from the Legion components, which are enough for the map, units and combat table. Movement, ZOC and supply rules for that game could not be read. Blennemann's notes are cited separately, because they are a clear statement of design purpose.

Most of the rulebooks put their terrain chart, CRT or weather table only on the map or on separate aid cards. Where a value below came from a map image or aid card rather than the rule text, the source digests say so. Unit counts are estimates unless a rulebook states them.

The per-game digests behind this outline are in [`rulebook-digests/`](rulebook-digests/README.md), one file per game in a common template. They give the detail (costs, tables, rule numbers, counts) that the outline only summarizes.

## How the seven rulebooks are organized

All seven use decimal case numbering (e.g. 9.4.4), and the two SPI games use the SPI form of General Rule, Procedure, then numbered Cases. Within that common frame there are three orders:

1. **Classic equipment-then-mechanics order** (Panzergruppe Guderian, War in the Ice, The Russian Campaign, Stalin Moves West): introduction, components, sequence of play, movement, stacking, ZOC, combat, then the special systems (supply, leaders, air, reinforcements, weather), then victory, scenarios, options and notes.
2. **Sequence-of-play order** (Dai Senso, D-Day at Tarawa): the core rules follow the turn phase by phase. Dai Senso numbers its core sections so that section 4 is Phase 4. Rules that apply at all times ("housekeeping": ZOC, stacking, supply, weather, war state) follow the core, and alphabetical look-up entries (markers, events) come last.
3. **Basic game, then extended game** (D-Day at Tarawa): sections 1-14 are a complete short game with its own scenario; sections 15-19 add the rules for longer scenarios.

Three regularities hold across the set:

- **Supply always comes after combat** in the book, even in games where the supply check happens at the start of the turn (War in the Ice, Panzergruppe Guderian). The reader therefore meets out-of-supply effects in the movement and combat sections before supply has been defined.
- **Victory conditions are placed either very early or very late.** Stalin Moves West, War in the Ice and Dai Senso (as "Phase 0") put victory before the mechanics; Panzergruppe Guderian, The Russian Campaign and Tarawa put it near the end.
- **Charts are rarely in the book.** Only the War in the Ice chart sheet and the Tarawa panel come close to a complete set. In five of the seven games the terrain chart or the CRT exists only on the map, so the rulebook cannot be read on its own.

## Feature comparison

| | Dai Senso | Tarawa | Pz. Guderian | Stalin Moves West | Russian Campaign | Velikiye Luki | War in the Ice |
|---|---|---|---|---|---|---|---|
| Hex | 120-300 mi | 100 yd | 10.5 km | ~70 km | not stated | not read | ~130 km |
| Turn | 30-60 days | 30 min to 11 h | 2 days | 1-2 months | 2 months (2 impulses) | ~8-9 days | 15 days |
| Units | battalion to army group | company | regiment, division | corps, army | corps, army | battalion to division | task force, squadron |
| CRT | odds, column shifts, 1d6 | weapon matching, cards | odds 1-3 to 10-1, 1d6 | odds as percent, two CRTs, 1d6 | odds 1-6 to 9-1, 1d6 | odds 1-3 to 5-1, 1d6 with DRM | differential, 2d6, tactical chits |
| ZOC | stop; friendly units negate | stop next to enemy or in intense fire | rigid, sticky | stop; strong | rigid; must attack | not read | none |
| Steps | 1-3, force pool swaps | US 1-4; Japanese depth markers | 1-4 | none; armies break down | none | 1-2 | none |
| Supply | trace to sources by rail, road, convoy | communication paths | 20 hexes to road; Soviet leader radius | supply-unit radius; spent as a multiplier | 8 hexes to city or rail; else eliminated | not read | supply points counted in the hex |
| Air | support units give shifts | none | interdiction markers | airbases; air superiority | Stuka and Sturmovik shifts | none on counters | central: air combat, AA, detection |
| Sea | convoys, fleets, beachheads | amphibious landing | none | optional | one move per sea area | none | transit track; optional |
| Hidden information | secret card hands | face-down units, depth | untried Soviet units | untried Soviet units | none | not read | detection system |
| Weather | fixed by calendar | night turn only | none | rolled from turn 4 | rolled, cumulative DRM | not read | rolled, continent-wide |

---

# The outline

The parts are given in the recommended reading order. Each part lists what it must contain, then the range of practice.

## Part 0. Front matter

**Include:**
- Title, edition, date and credits.
- A table of contents to two levels.
- "How to read these rules": the numbering scheme, the meaning of *must*, *may*, *a* and *one*, the marking of errata and of examples, and a pointer to the glossary and index.
- A list of the charts and where each is printed.

**Practice:** Dai Senso's "How to Read the Rules" defines *a*, *one*, *may*, *must* and *No Result* strictly, which removes many later disputes. Stalin Moves West marks errata in red and examples in blue; the Berger edition of Panzergruppe Guderian uses color to mark the Avalon Hill additions and the corrections. Both methods let a reader see what has changed between editions.

## Part 1. Introduction to the situation (required)

**Include:**
1. **The situation depicted**, and which of three kinds it is:
   - *Historical* (Panzergruppe Guderian, The Russian Campaign, Tarawa, Velikiye Luki, Dai Senso). A short narrative of the campaign is enough.
   - *Hypothetical past* (Stalin Moves West). The premise needs to be stated, and its departure from history named: what did not happen, and what happens instead.
   - *Future* (War in the Ice). The setting has to be built: who fights, why, with what equipment, and under what limits.
2. **Scale**: hex size, turn length, unit size, and number of players (including solitaire suitability).
3. **The sides**, and what each is trying to do.
4. **The scenarios**, each with its length and coverage, and the **historical or usual course and outcome**.
5. **Interesting variations**: what-if options, alternative setups, and variant rules.
6. **The most important or unusual design features, and the purpose each serves.** This is the part most often left out, and the most useful to a new player.
7. **How to win, in brief**, with a pointer to the full victory rules in Part 14.

**Practice:**
- *Establishing a premise.* Stalin Moves West handles its hypothesis in two short scenario paragraphs. In No Barbarossa, Germany turns west and the Soviets take the opening. In Pre-emptive War, Germany is massing and Stalin strikes first. Surprise is then expressed through economics and build restrictions, not combat penalties. On turn 1 a "Shock Effect" bars the Axis from earning mobilization points or forming armies, and the Soviets always move first. A "No Stalinist Purge" variant turns off the Red Army's handicaps, so the premise can be tested. The lack of any designer's note on how these values were calibrated is a gap.
- *Building a future.* War in the Ice does it with a long fictional "Situation Briefing" placed after the rules. It contains a campaign history of 1991-92, background essays on the war's origins, the forces and the air war, and a mock archaeology article for the science-fiction variant. The history doubles as strategy advice, because the fictional commanders' mistakes are the ones a player should avoid. Victory levels are described as political consequences, up to the risk of nuclear escalation.
- *Usual course of play.* Few rulebooks say what usually happens. Panzergruppe Guderian says play is biased toward the historical German Marginal Victory and offers, as an option, scoring that result as a draw. Simonsen's article describes three stages: approach (turns 1-4), assault on the Smolensk line (4-8) and breakout (8-12). The Dai Senso scenario notes trace the historical arc: the China quagmire, the Strike North or South choice, "victory disease" in 1942 and the 1943 stalemate. They also give the historical final score. The Dai Senso errata goes further and admits that ahistorical Axis strategies win too easily.
- *Design purpose.*
  - War in the Ice lists its reasons. Resource points are both budget and score, so every loss counts. Units cannot be attacked until detected, which makes play resemble a carrier battle. Supply is counted point by point because Antarctic logistics are too important to abstract. Nuclear weapons are excluded on principle.
  - Blennemann's Velikye Luki notes give a model statement of a system's purpose: "mechanical simplicity". A three-segment turn and a fight-or-move choice give interaction without bookkeeping. Command points give control with uncertainty. Combat chits fold air and artillery into one draw.
  - The Russian Campaign's developer notes explain each change from the 1970s design by the problem it solves; rail junctions, for example, stop rail conversion ahead of the front.
  - Panzergruppe Guderian has no designer's notes, so the purposes of its untried units and second movement phase have to be inferred.

## Part 2. Components

**Include:**
- An inventory of the map sheets, counter sheets (with counts), cards, charts, dice and booklets.
- The game scale, if Part 1 does not give it.
- A short "how to read a unit" diagram, which belongs in Part 4 when Part 4 follows immediately.

**Practice:** Panzergruppe Guderian and War in the Ice have a Parts Inventory case. Tarawa lists 352 counters, Dai Senso 560 counters and 200 cards, and War in the Ice 400 counters.

## Part 3. The map (required)

**Include:**
1. **Grid and numbering**: hex size, numbering scheme, map edges and their meaning, and any sheet prefixes. Dai Senso uses a `w` or `e` prefix for its two maps.
2. **Hex terrain types**, each with its movement cost by unit class and its combat effect. The full terrain effects chart is reproduced in the book.
3. **Hexside features**: major and minor rivers, lakes, straits, mountains, escarpments, seawalls, prohibited hexsides, and borders.
4. **Center-to-center features**: roads, railroads, tracks; and whether they bridge rivers for movement, for supply, or for both.
5. **Places**: cities (by class), objectives and their values, supply sources, ports, airbases, entry hexes, and setup areas.
6. **Political geography**, where it matters: countries, borders, and neutral areas.
7. **Off-map boxes, tracks and displays**: turn track, weather, victory points, loss tracks, reserve boxes, sea zones, and delay boxes.
8. **Markers**: control, supply or out-of-supply, disruption, fortification, command, weather, turn, and victory-point markers.
9. **Special features unique to the game.**

**Practice:**
- *Terrain lists are short.* Panzergruppe Guderian has five hex types (clear, forest, swamp, major and minor city) plus river, lake, road and rail. The Russian Campaign has four hex types, all costing 1 MP; the differences come from rules on stopping, not from costs. Velikiye Luki has open, wooded, hill, town, village and lakes. Its movement costs are fractional and depend on unit class: open 1.5 MP; woods 2 MP, but 1.5 for ski and mountain units and 3 for tank, mechanized and motorized units. Effects are column shifts: woods, hill, town and major-river hexsides each shift one column left, and a minor river gives a -1 die roll modifier. War in the Ice has only snow, shelf ice and mountain or glacier, in pure and mixed hexes. Its terrain has no effect on land combat, only on movement, detection and breakoff.
- *Rivers vary in representation.* Rivers are hexsides in most of the games. In The Russian Campaign they run through hexes, and a defender is doubled when every attacker stands on a river hex. Panzergruppe Guderian and Stalin Moves West double the defender across a river only if every attacker crosses it.
- *Roads and railroads.* Panzergruppe Guderian's roads do not bridge rivers for movement, but they do for German supply. Its railroads serve only Soviet rail movement. The Russian Campaign has railroads and no roads at all. Tarawa and War in the Ice have no linear transport features.
- *Special features.*
  - Tarawa prints its Japanese defense on the map. Each position hex carries a zone letter, a priority number and a color, and is surrounded by dots for intense and steady fire. Colored water arcs mark the fields of fire, and the beach approach hexes carry arrows for the run in.
  - Dai Senso adds naval zones with On Station and Convoy boxes, restricted waterways, oil and limited-stacking hexes, and political geography.
  - War in the Ice prints primary and secondary bases for each nation, stasis complexes and the magnetic pole. Its supply track shows each base's capacity.
  - Velikiye Luki has German and Russian entry-hex columns on opposite map edges, labeled by the arriving formation, and a loss track numbered 1-15.
- *Markers.* None of the games uses an out-of-supply marker except War in the Ice ("Unsupplied"). Supply is instead checked when it matters. The common markers are turn, weather, victory point, control, disruption, fortification (Velikiye Luki prints -1L and -2L column shifts on them) and interdiction.

## Part 4. Units (required)

**Include:**
1. **Kinds of units**: combat units by branch; headquarters and leaders; support units (artillery, engineers, supply units); air and naval units; installations; and informational pieces.
2. **Unit sizes**, with the symbols used.
3. **Counts by side and by kind**, as a table. None of the seven rulebooks gives this in full, and every digest had to estimate it.
4. **Counter layout**: a labeled diagram of each counter type, naming every printed value. War in the Ice never states the order of its air-unit factors in words.
5. **Characteristics**: attack, defense (or a single combat factor), movement allowance, steps, range, quality or EW rating, support factor or radius, and arrival turn.
6. **Step reduction**: what the back side shows; or, if units do not flip, how losses are taken.
7. **Hidden or untried units**, if any: what the back shows and when the front is revealed.
8. **Nationality and formation rules**: which units may stack or attack together.

**Practice:**
- *Values printed on the counter.*
  - Two values (combat, movement): Panzergruppe Guderian's German units, The Russian Campaign and Velikiye Luki.
  - Three values (attack, defense, movement): Dai Senso, Stalin Moves West and the tried Soviet rifle divisions of Panzergruppe Guderian.
  - Four values (anti-armor, anti-infantry, anti-air, EW), with movement by type on a chart: War in the Ice.
  - Attack strength, steps, range and target symbol, with the Japanese counter listing the US weapons needed to defeat it: Tarawa.
- *Counts.*
  - Panzergruppe Guderian: about 61 German units against 78 Soviet rifle, 10 tank and 10 mechanized divisions plus 15 leaders. There are more Soviet rifle arrivals than counters, so eliminated counters are recycled.
  - Velikiye Luki, counted from the counter scan: about 31 Soviet units (4 of them Guards) and 28 German (including one Luftwaffe and one SS). About half of each side has a reduced back.
  - War in the Ice: about 35 land and 30-35 air units per superpower.
  - Tarawa: 46 Japanese units against about 28 US rifle companies plus engineers, tanks and HQs.
- *Steps.*
  - Panzergruppe Guderian: German infantry divisions have four steps on two counters; mechanized regiments two; Soviet units one.
  - Dai Senso: units swap for smaller units from a limited force pool.
  - Stalin Moves West: German armies break down into corps.
  - The Russian Campaign and War in the Ice: no steps; units are eliminated whole.
- *Hidden quality.*
  - Panzergruppe Guderian and Stalin Moves West put Soviet units face down until combat, which models the Red Army's uneven quality. Panzergruppe Guderian's untried rifle divisions range from 0-0-6 duds to 9-8-6.
  - Tarawa hides Japanese strength under face-down units and depth markers.
  - In both cases the rule is an information rule as much as a unit rule, and it belongs in Part 12 with a cross-reference here.

## Part 5. Sequence of play

**Include:**
- A one-page outline of the game turn: every phase and segment in order, with who acts and what may be done.
- It is placed before the movement rules.
- Where the game allows, the rule sections are numbered to follow it.

**Practice:**
- All seven games give the sequence before the mechanics.
- War in the Ice annotates its 13 phases so fully that the outline doubles as a play summary.
- Dai Senso's numbering (Phase 4 is Section 4) makes "read as you play" learning practical.
- Panzergruppe Guderian precedes the formal sequence with a narrative walk-through of one turn (2.0), which teaches the flow before the cases.
- Two structural ideas recur:
  - *a second movement phase after combat* for mechanized units: the Panzergruppe Guderian Mechanized Movement Phase, the Stalin Moves West Exploit phase, and The Russian Campaign's second impulse;
  - *breaking the strict alternation* of turns: alternating stacks by initiative in War in the Ice; three segments per side and a fight-or-move choice in Blennemann's T3 system.

## Part 6. Movement (required)

### 6.1 Land movement

**Include:**
1. Movement allowance and costs. The minimum move (always one hex, or not). Whether points may be saved or transferred.
2. Terrain costs by unit class, referring to the chart in Part 3.
3. Zones of control: which units exert them, the effect on entry, exit and passage between ZOC hexes, and whether friendly units negate them.
4. Stacking: limits, when they are checked, and the penalty for overstacking.
5. Road, rail and strategic movement: capacity, conditions, and gauge or railhead rules.
6. Second movement phases, exploitation, overrun and infiltration.
7. Reaction or reserve movement.
8. The effects of supply and weather on movement, cross-referenced to Parts 8 and 11.
9. Reinforcement entry, cross-referenced to Part 10.

Retreat and advance after combat are better placed in Part 7.

**Practice:**
- *ZOC strength.*
  - Rigid and sticky in Panzergruppe Guderian: a unit stops on entry and leaves only by combat or overrun.
  - Rigid in The Russian Campaign, which forbids moving from one ZOC hex directly to another.
  - Strong in Stalin Moves West, where a retreat into an enemy ZOC is fatal even into a friendly-occupied hex. The designer's note says this replaces a bonus for concentric attack.
  - Absent in War in the Ice, where stacking is unlimited and a detected unit simply stops on meeting detected enemies.
- *Stacking.* Usually 2-3 units per hex, or a mix of armies and corps. Tarawa allows 2 US units, and a stack of five or more steps becomes a "concentrated target".
- *Rail and road.*
  - The Russian Campaign: rail capacity is 6 Axis units (3 in snow) and 5 Russian per turn, at unlimited range. Axis rail converts to the Soviet gauge behind the advance, which the developer calls the most complex rule in the game.
  - Panzergruppe Guderian: only the Soviets use rail, with up to 8 units over 30 rail hexes.
  - Stalin Moves West: rail costs half the printed allowance, at unlimited range on the side's own network.
  - Dai Senso: road and rail cost 1/2 MP for one-step units and 1 MP for multi-step units.
- *Second-phase movement.* The Panzergruppe Guderian Mechanized Movement Phase lets a 10-MA panzer regiment cover 20 road hexes after combat, with overruns. This is the game's model of blitzkrieg tempo. The Russian Campaign has no advance after combat at all; its second impulse plays that part.
- *No movement points at all.* In Tarawa a move action is up to 2 land hexes (3 from turn 11). Units stop on entering an intense-fire hex or a hex adjacent to an enemy, and passing through a field of fire draws a fire card.
- *First-turn restrictions* model surprise. In Panzergruppe Guderian two Soviet armies move only on a die roll of 1-3, and two must spend full allowance moving north, south or east.

### 6.2 Sea movement

**Include:**
- Naval units and transports.
- Port-to-port movement and capacity.
- Amphibious movement and landing.
- Evacuation.
- Survival or interception rolls.
- Sea zones and boxes.

If sea movement is not modeled, the section says so.

**Practice:**
- *D-Day at Tarawa is built around the landing.* Next turn's units wait in the beach approach hexes. Each LVT carries up to 4 steps. Two cards per LVT give drift, then steps kept and landing point: reef, water, beach, or one or two hexes inland. Units left in the water wade 2 hexes per turn under fire, which represents troops that the boats left at the reef. The rules never mention tides; the reef that boats cannot cross is the only expression of water depth. The procedure is a model of presentation: a numbered phase summary, one case per step, and a worked example for a whole beach.
- *Dai Senso*: transport by convoy markers in naval-zone boxes. A surface fleet placed in an all-sea hex becomes a beachhead. Units left at sea without one are eliminated.
- *The Russian Campaign*: one move per side per sea area per turn, either transfer, evacuation or invasion, with a survival roll that improves with friendly ports.
- *Abstracted*: Stalin Moves West (optional amphibious move) and War in the Ice (a transit track for shipping time, optional sea transport and blockade).
- *Not modeled*: Panzergruppe Guderian.

### 6.3 Air movement

**Include:**
- Bases.
- Range, and how it is counted.
- Whether aircraft trace paths or are simply placed.
- Missions by phase.
- Return to base.
- Ferry or transfer between bases.
- Airborne and air-landed units.
- Weather limits.

**Practice:**
- *Aircraft are placed, not moved,* in every game that has them:
  - War in the Ice: air units fly to any hex within range and return to the same base, and each type flies only in certain phases.
  - Dai Senso: air forces are placed within 3 hexes of an air base each Support Segment.
  - The Russian Campaign: Stukas are placed within 8 hexes of an HQ.
  - Panzergruppe Guderian: air power is only interdiction markers.
- *Airborne drops* are short-ranged and weather-limited: Russian paratroops in The Russian Campaign drop only in snow first impulses, within 8 hexes of the Stavka; Stalin Moves West loses airborne units on a die roll of 1-2 (Soviet) or 1 (German); Dai Senso airdrops place units as markers within 2 hexes.

## Part 7. Combat (required)

### 7.1 Land combat

**Include:**
1. Who may attack, and whether attack is voluntary or mandatory. How often a unit may attack or be attacked.
2. Multi-hex and multi-unit attacks, and soak-offs.
3. Computation of strengths, with **the order in which modifiers apply** stated once.
4. The CRT: its type (odds, differential, other), the die, the rounding rule, and the column limits. Reproduced in full.
5. Results: every code defined, with who chooses losses.
6. Retreat: who routes it, the path rules, and the penalties.
7. Advance after combat.
8. Special combat: overrun, automatic victory, assault on fortifications, artillery, and close combat.
9. The effects of terrain, supply, weather and leaders, cross-referenced.

**Practice:**
- *Odds CRT with one d6* is the norm: Panzergruppe Guderian (1-3 to 10-1), The Russian Campaign (1-6 to 9-1), Dai Senso (1-3 to 9-1 with column shifts), Velikiye Luki (1:3 to 5:1, with die-roll modifiers that create a 0 row), and Stalin Moves West (odds expressed as a percent).
- *Other designs.*
  - Stalin Moves West lets the attacker choose an Assault or a Mobile CRT. Mobile needs a mechanized unit and is barred against cities.
  - War in the Ice computes a differential and resolves it with 2d6. Each side first picks a secret tactical option chit (Cavalry Charge, Hedgehog, Surrender, and others), and the pair of chits gives a modifier of -3 to +3.
  - Tarawa uses no die. A US attack is read on a results chart by strength ratio and by whether every weapon the Japanese counter requires is present. Close combat is a draw from card piles.
- *Mandatory attack.* In The Russian Campaign every unit in an enemy ZOC must attack, and units forced into an attack below 1-6 surrender. In War in the Ice combat is mandatory wherever both sides are detected.
- *Result sets.*
  - Velikiye Luki uses NE, A1, A2, EX, EX1, DR, D1, D1R, D2R and D3R. A German defender in Velikiye Luki may ignore a retreat by taking one more step loss.
  - Panzergruppe Guderian's A1, A2, D1 and D2 mean the owner loses that many steps or retreats that many hexes.
  - The Russian Campaign adds C ("contact", no effect) and a sequenced exchange.
- *Modifier order.* Panzergruppe Guderian applies divisional integration (doubling), terrain, supply halving and overrun halving. The order was settled only in errata. Velikiye Luki's terrain chart gives every effect as a column shift, which removes the ordering problem.
- *Automatic victory and overrun.* The Russian Campaign removes defenders at 10-1 during movement. Panzergruppe Guderian's overrun costs 3 extra MP and attacks at half strength.

### 7.2 Naval combat

**Include:** surface, carrier and submarine combat, or a statement that it is abstracted or absent.

**Practice:** None of the seven games has ship-to-ship combat. Dai Senso sends contested fleets to a Naval Warfare Delay Box, where luck decides how long they are out, as at Midway. Fleets on station deny enemy ports. Tarawa uses naval fire markers from events (strength 9) as fire support.

### 7.3 Air combat

**Include:**
- Air-to-air combat.
- Anti-aircraft fire.
- Ground support.
- Interdiction.
- Strategic bombing.
- Air superiority.
- Satellites or other reconnaissance, where relevant.

**Practice:**
- *Air as a column shift*: The Russian Campaign (Stukas +3, Sturmoviks +1, shift capped at 3, attacker only), Dai Senso (+1 per air unit), and Stalin Moves West (airbase factors, after an air-superiority roll).
- *Air as a full subsystem*: War in the Ice. Air-to-air combat gives each side "lock-ons" rolled on a hit table indexed by net EW. Anti-aircraft fire follows. Lasers fire at any aircraft, and ground support adds to land combat. Satellites detect and can be shot down.
- *Interdiction* appears in Panzergruppe Guderian (markers that cost +1 MP or 4 rail points) and as an option in Stalin Moves West.

## Part 8. Supply (required)

### 8.1 Land supply

**Include:**
1. Supply sources.
2. How supply is traced: path length, the units of measure, what blocks the path, and whether roads, rails or HQs extend it.
3. When supply is checked.
4. The effects of being out of supply, and of isolation over time.
5. Attrition.
6. Supply units or supply points, if used.
7. Special cases: units that are always in supply, reinforcements, and fortresses.

**Practice:** The seven games show five distinct models.

1. **Trace to a source.**
   - Panzergruppe Guderian: German units trace up to 20 hexes to a road leading to one west-edge road hex.
   - The Russian Campaign: units trace up to 8 hexes (4 in snow) to a friendly city, or to rail linked to one; units still unsupplied at the end phase are eliminated.
   - Dai Senso: units trace two hexes, then any distance by rail, one stretch of road, or across naval zones that hold a convoy.
2. **Trace to a leader or HQ.** Panzergruppe Guderian's Soviet units trace a line no longer than a leader's rating (2-5 hexes) to the leader, and the leader traces to the east edge. Leadership and supply are one system, although the rulebook splits it between two sections.
3. **Radius of a supply unit, spent as a multiplier.** In Stalin Moves West, general supply is being within a supply unit's radius. Supply units are also spent to double attack or movement. Being out of supply blocks only rail movement and building.
4. **Supply points counted in the hex.** In War in the Ice there are no supply lines. Each unit consumes 1-3 supply points per turn from its own hex. The points are carried by vans and transports and stored in bases with fixed capacities. Unsupplied units cannot move and lose 3 strength. The designer judged Antarctic logistics too important to abstract.
5. **Communication instead of supply.** In Tarawa, Japanese positions need a path to other occupied positions, and US units need a path to the beachhead. These paths govern depth, reserves and securing objectives.

The effects of being out of supply range from halving (Panzergruppe Guderian), through elimination (The Russian Campaign), to restrictions only (Stalin Moves West, Dai Senso). No game but War in the Ice has a gradual loss over time.

### 8.2 Sea supply

**Include:** convoys, shipping capacity, ports, interdiction of shipping, or a statement that it is abstracted.

**Practice:** Dai Senso's supply convoys let a supply line cross a naval zone between open ports. War in the Ice uses its transit track for shipping time, and supply points appear at coastal bases. Elsewhere sea supply is absent.

### 8.3 Air supply

**Include:** airdrop and air transport of supply, capacities, and losses.

**Practice:** War in the Ice makes it central. Airdrops supply units in the field at triple cost, air transport moves supply between bases at no loss, and intercepting supply flights is the main air contest. Dai Senso, Panzergruppe Guderian and The Russian Campaign have no air supply.

## Part 9. Command and control

**Include:** headquarters, leaders, command ranges and radii, initiative, command points or chits, activation, and the loss of leaders.

**Practice:**
- *Leaders as enablers.* Panzergruppe Guderian's Soviet leaders supply units, permit attacks within their radius and add their rating to an attack. Stalin Moves West's HQs add attack strength and permit exploitation. The Russian Campaign's HQs set air range, and the loss of Hitler or Stalin freezes that nation's next impulse.
- *Turn order and command.* War in the Ice rolls initiative, and Blennemann's T3 system uses command points with an uncertain supply. Tarawa grants free actions to heroes, HQs and command posts, beyond a three-action limit.
- *Political command.* Dai Senso's Japanese Government marker models a divided Army and Navy leadership.

## Part 10. Reinforcements, replacements and production

**Include:** the arrival schedules, entry hexes, delays, replacement points and their sources, production or purchase, withdrawals, and recycling of eliminated units.

**Practice:**
- *Schedules on the turn track*: Velikiye Luki prints them there, and The Russian Campaign keeps them on order-of-battle cards.
- *Point-buy*:
  - Stalin Moves West: mobilization points come from the hexes held.
  - War in the Ice: resource points are both budget and score.
  - Dai Senso: option cards add units and replacement steps.
- *Replacements tied to geography*: The Russian Campaign's Axis replacements depend on the oil fields, and its Russian replacements on worker units.
- *Rule.* Every chart a schedule depends on belongs in the rulebook or in one named, authoritative aid. Panzergruppe Guderian's map and its rules disagree about the Soviet turn-12 arrivals.

## Part 11. Weather and time

**Include:**
- The weather table and how it is used.
- Weather areas.
- Effects on movement, combat, supply, air and naval activity.
- Night.
- Seasons.

**Practice:**
- *Weather rolled each turn*:
  - The Russian Campaign: a d6 with a cumulative modifier that damps long runs of extreme weather.
  - War in the Ice: one continent-wide roll, with grounding of most aircraft in poor weather.
  - Stalin Moves West: rolled from turn 4.
- *Weather fixed by the calendar*: Dai Senso, which has seven weather areas.
- *No weather but a night turn*: Tarawa. Its 11-hour turn 16 shrinks fields of fire to 1 hex.
- *No weather*: Panzergruppe Guderian.

## Part 12. Fog of war and detection

**Include:** what is hidden, from whom, and how it is revealed; the inspection of enemy stacks; dummies; secret cards.

**Practice:**
- *Untried units*: Panzergruppe Guderian and Stalin Moves West.
- *Face-down units and depth markers*: Tarawa.
- *Secret card hands*: Dai Senso.
- *A full detection system*: War in the Ice. Units cannot be attacked until detected by aircraft, contact, sensors or satellites, and a second roll reveals their type.
- *No hidden information*: The Russian Campaign.

## Part 13. Special units and special rules

**Include:**
- Rules for unit types with unique abilities (engineers, mountain, ski, paratroops, fortresses, partisans, commandos).
- Rules for one-time historical events.
- Each special rule placed in one section, with cross-references from the general rules.

**Practice:**
- Stalin Moves West gathers fortresses, the Berlin garrison, mountain units, partisans, airborne units and Fliegerkorps VIII in one "Unique Units" section.
- Dai Senso puts markers and events in alphabetical look-up sections.
- Velikiye Luki's components show ski, mountain, engineer, security, artillery and anti-aircraft units, and fortification markers for the city. Blennemann's notes argue for keeping exceptions to a minimum.

## Part 14. Victory conditions

**Include:**
- How victory points or objectives are scored.
- When they are checked.
- Sudden-death or automatic victory.
- Victory levels.
- Tie-breaking.

**Practice:**
- *Hexes held*: Panzergruppe Guderian, Stalin Moves West, Dai Senso and Velikiye Luki. Velikiye Luki values the city at 9 VP for the German and 1 for the Russian.
- *Hexes held plus losses*: Panzergruppe Guderian (5 VP per German division destroyed), Velikiye Luki (1 VP per Russian step lost and per German unit lost), and War in the Ice (net resource points).
- *Secured objectives*: Tarawa counts "secured" Japanese positions.
- *Sudden death*: The Russian Campaign checks yearly objectives after each Jan/Feb turn.
- *Placement.* The brief statement belongs in Part 1 and the full rules here, beside the scenarios that vary them.

## Part 15. Scenarios and setup

**Include, for each scenario:**
- Length and start and end turns.
- Setup, by hex or area, for each side.
- Reinforcement and withdrawal schedules.
- Special rules.
- Victory conditions.
- Balance notes.
- A suggested learning scenario.

**Practice:**
- *A ladder from short to long*: Dai Senso offers three training scenarios of 1-5 turns before its campaigns, which run to 36 turns. The Russian Campaign has a five-turn tournament scenario and four seasonal ones alongside the campaign.
- *A short first scenario*: Tarawa's First Waves uses only the basic rules.
- *A single scenario*: Panzergruppe Guderian.

## Part 16. Optional rules and variants

**Include:**
- Each option, with its effect on balance.
- A statement of which options are tested.
- Bidding for sides, where balance is uncertain.

**Practice:**
- *Optional rules, numbered within the main sequence*: The Russian Campaign (17 options).
- *Untested variants, kept apart*: The Russian Campaign separates them as "bonus materials".
- *Variants and options mixed*: War in the Ice combines scenario variants (early victory, open-ended, winter start) with optional rules (magnetic disruption, mines, political influence).
- *Bidding for sides*: Dai Senso, The Russian Campaign and the Panzergruppe Guderian "PGG II" variant.

## Part 17. Designer's notes, historical commentary and bibliography

**Include:**
- The purpose of each major system.
- Sources for the order of battle.
- What was left out, and why.
- What changed from earlier editions.

**Practice:**
- Blennemann explains why he omitted the 7th Flieger-Division as an optional reinforcement: the other side's response would be pure speculation.
- The Russian Campaign's developer notes tie each change to a problem.
- Tarawa, Panzergruppe Guderian and Stalin Moves West have almost no notes, so the reasons for their abstractions are unstated.

## Part 18. Examples of play

**Include:**
- A narrative walk-through of one turn.
- Worked examples beside each hard rule (multi-step procedures, supply tracing, retreats, special combat).

**Practice:**
- Panzergruppe Guderian opens with its walk-through.
- The Russian Campaign gives three examples for rail conversion alone.
- War in the Ice works through air combat, anti-aircraft fire and land combat step by step.
- Tarawa illustrates a whole beach landing.

## Part 19. Charts and tables

**Include:**
- Every chart the rules depend on: terrain effects, CRT, movement allowances, weather, supply, costs, reinforcement schedules, and victory points.
- Each reproduced in the book, even when it is also printed on the map.

**Practice:** This is the most common failure in the set. The terrain chart and CRT exist only on the map in Panzergruppe Guderian's original, Stalin Moves West, Dai Senso and The Russian Campaign. The Russian Campaign's movement-allowance chart is only on a player aid. War in the Ice's supply capacities appear only on a map track.

## Part 20. Glossary and index

**Include:** a glossary of every defined term, and an index.

**Practice:**
- Dai Senso has a glossary, but several terms a player needs early (Air Base, Open Port, Naval Base) are defined only there.
- The Russian Campaign has an index.

---

## Practices to copy, and practices to avoid

**To copy:**
- A narrative walk-through of one turn before the formal rules (Panzergruppe Guderian).
- Rule numbers that follow the sequence of play (Dai Senso).
- A one-page annotated sequence of play that doubles as a summary (War in the Ice).
- A complete short game in the first sections, with longer-game rules added later (Tarawa).
- A numbered procedure, one case per step, and a worked example for every multi-step mechanic (Tarawa's landing; War in the Ice's combat).
- Strict definitions of *must*, *may* and *one* (Dai Senso).
- Edition changes marked by color (Stalin Moves West; the Berger edition of Panzergruppe Guderian).
- A designer's note for each major system, saying what problem it solves (The Russian Campaign; Blennemann).
- Fiction or history that teaches strategy (War in the Ice's campaign history).

**To avoid:**
- Charts printed only on the map or aid cards, which leave the book incomplete. This occurs in five of the seven.
- Definitions scattered across sections. Panzergruppe Guderian defines its armored duds in two places but not in the untried-unit rule.
- An unstated order of modifiers (Panzergruppe Guderian, settled only in errata).
- Rules hidden in examples. The Russian Campaign's Mud second-impulse limit appears only in a sea-movement example.
- One system split across sections (Panzergruppe Guderian's Soviet supply and leadership).
- Unchecked cross-references. Every digest found wrong section numbers: Tarawa, Stalin Moves West and War in the Ice each have several.
- Two different statements of one test (Tarawa's catastrophic loss).
- "Starting turn 11" clauses scattered through the basic rules; a single table of the changes would serve better (Tarawa).
- Supply defined after the movement and combat rules that depend on it. If supply is checked at the start of the turn, its definition could come first, or at least a one-paragraph summary could.

Copyright Ben Paul Wise. All Rights Reserved.
