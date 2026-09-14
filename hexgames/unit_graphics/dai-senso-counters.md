Copyright Ben Paul Wise. All Rights Reserved.

# Axis Empires: Dai Senso! — Counter Inventory

**Purpose.** This document describes the unit counters and markers of *Axis Empires:
Dai Senso!* (Alan Emrich, Thomas Prowell and Salvatore Vasta, Decision Games, 2011)
as input to a counter-generating library. It follows the brief in
`military-unit-icons-handoff.md`: describe **what information each counter carries
and where**, in abstract terms, so that software can convey the same information. It
does **not** try to reproduce the printed artwork, fonts or corner treatments.

**Sources.**

| Source in the game folder | What it shows |
|---|---|
| `pic1753886.png` | Counter sheet **Front 1**, 2011: Japan, Kwantung Army, Axis minor countries, miscellaneous and European markers |
| `pic1753887.png` | Counter sheet **Front 2**, 2011: Russia, Chinese and other minor countries, Great Britain and Commonwealth, United States |
| `pic385874.jpg` | Front 2 of the **2008 Triumvirate Games** printing, sharper scan, different colour-band layout |
| `pic1913992.jpg` | Photograph of printed counters on the map, close enough to read the bands and pips |
| `Dai Senso  Living_Rules_February_2014.pdf`, pp. 6–8 | Labelled ground-unit and support-unit anatomy; all unit-type symbols; marker types; reinforcement codes |

The game has 560 counters on two sheets. Only the fronts are available as scans; the
backs are described from the rules.

---

## 1. Overview

### 1.1 Counter families

The rules divide counters into **units** and **markers**. Units are further divided:

| Family | Examples | Values printed |
|---|---|---|
| Ground units | infantry, garrison, cavalry, airborne, marine, armour, mechanized, cav-mech, HQ, fortress, port-a-fort | attack – defence – movement |
| Air support units | Air Force, Bomber, Interceptor, CV Strike | range in a circle |
| Fleet support units | CV Fleet, Surface Fleet, Sub Fleet | none |
| Operational markers | Airdrop, Beachhead, Convoy, Detachment, Logistics, Partisan Base | varies |
| Political and status markers | Neutrality, Armistice, Allied Collapse, Enforced Peace, Entry, Influence, Oil Embargo, weather, war state… | often a signed number |

### 1.2 The big differences from the other two games

- **Three numbers** on ground units: attack, defence and movement are separate.
- **Colour means country, not faction.** A minor country can change sides during
  play, so its counters are coloured by country and carry no faction indication at
  all.
- **A counter's back is often a different counter.** Air Force units have Escort
  Troop Convoys on their backs; CV Fleets have CV Strikes; airborne units have their
  Airdrop markers; some infantry have Detachment markers.
- **Markers outnumber units** on some parts of the sheet, and many of them carry
  political rather than military information.

---

## 2. Standard Elements

Elements that also appear, in some form, on the Tarawa and TRC counters.

### 2.1 Counter tile and background colour

A square tile with one flat ground colour per **country**. The main grounds visible on
the sheets:

| Country or group | Ground |
|---|---|
| Japan | orange-red |
| Kwantung Army (Japanese) | darker red |
| Russia | brown |
| Great Britain and Commonwealth | tan |
| United States | sage green |
| Each Chinese and other minor country | its own colour: yellow, orange, purple, blue, olive, pink, grey, white and others |

Minor countries are distinguished by the **combination** of ground colour and text
colour — red text on yellow, cyan on red, green on white, white on grey — because
there are more minor countries than clearly different grounds. The 2008 and 2011
printings arrange the minor-country bands differently but follow the same principle.

Frame shape is never used for side.

### 2.2 Unit symbol box and branch icon

A small horizontal rectangle in the middle of the tile. The rules list every ground
unit symbol:

| Unit type | Printed icon | Nearest APP-6 main icon or modifier |
|---|---|---|
| Infantry | diagonal cross | Infantry |
| Garrison | cross with a small circle | no exact match; Infantry plus a custom mark |
| Cavalry | single diagonal | Cavalry / reconnaissance |
| Airborne | cross with a canopy mark | Infantry with the Airborne modifier |
| Marine | cross with an anchor | Infantry with the Amphibious modifier |
| Armour / tank | rounded oval | Armour |
| Mechanized | cross plus oval | Mechanized infantry |
| Cav-mech | diagonal plus oval | Armoured reconnaissance |
| HQ | a black-and-white **checkerboard** filling the box | no exact match; APP-6 marks HQ with a staff line |
| Fortress | an empty box | no APP-6 equivalent |
| Port-a-fort | a box with trench lines | no APP-6 equivalent |

The rules add an important note: "the unit-type symbol is what matters when determining
a unit type, and not the individual unit components." A three-step HQ built from armour
steps is an HQ, not armour.

### 2.3 Echelon marks

Lower-case X's **centred above the symbol box**:

| Mark | Meaning |
|---|---|
| `xxxx` | Army |
| `xxxxx` | War zone — a Chinese army group |

Smaller formations carry no mark. The rules say "all armies and army groups are
multi-step units", so an echelon mark generally implies more than one step pip.

### 2.4 Value line

Three numbers **across the bottom**: **attack – defence – movement**, for example
`3-3-1`, `7-6-2`, `0-1-1`, `0-3-0`. The rules are explicit that "only ground units have
three numbers on the bottom" — the three-number line *is* the definition of a ground
unit. A zero movement is printed as `0`.

### 2.5 Historical ID

The unit's historical designation, **to the right of the symbol box, rotated 90°**:
`3`, `4`, `16`, `KDA`, `1FE`, `TF 11/17`. On minor-country units the same slot often
carries `Exp` (expeditionary) or `Res` (reserve) instead.

### 2.6 Nationality ID

A short abbreviation **to the left of the symbol box, rotated 90°**, naming the minor
country or faction: `Kwa` (Kwantung), `Aus`, `Ind`, `NZ`, `HK`, `Kan`, `Sze`, `Yun`,
`Mong`, `Fra`, `Phil`. The rules say major-country units carry none, "except for
Japanese Kwantung units".

### 2.7 Reinforcement code

A small code in the **upper-left corner**:

| Code | Meaning |
|---|---|
| a number, e.g. `44` | the option card that brings this counter into play |
| a letter, e.g. `A`, `B`, `E` | the political or conditional event that brings it into play |
| `ASR` | the *Allies Support Resistance* political event |
| `N` | the *Minor Country Created* event — a "new" minor country |
| `®` | a random-campaign variant counter, not used in the standard game |
| `*` | not used in the combined *Axis Empires* game |
| a small earmark beneath the code | British, French, Russian and US counters, to keep them apart from the European sister game |

This is a denser vocabulary than either other game, and it is **lookup data**: the code
ties a counter to a card or event.

### 2.8 Step value

**One to three small marks in the upper-right corner.** Most units use **dots**;
garrison and fortress units use **squares**, to show they are limited in how they can
be combined or broken down.

The point for the library is that **the shape of a step mark can carry meaning of its
own**, separate from the count. A generator must support more than one step-mark shape
and let the game assign a meaning to each.

### 2.9 Back face

The rules describe the backs by family:

| Front | Back |
|---|---|
| Multi-step ground unit | a reduced version of the same unit |
| Certain infantry | a Detachment marker (pennant flag) |
| Airborne unit | its Airdrop marker (parachute) |
| Air Force | an Escort Troop Convoy |
| Bomber | a Devastation marker |
| CV Fleet | its CV Strike unit, and the reverse |
| Surface Fleet | an Escort Troop Convoy |
| Convoy | *Supply* on one side, *Troop* on the other |
| Beachhead | arrow, port, aircraft and limited-stacking icons on the front; arrow, port and battleship icons on the back |

So in this game the two faces of one piece of cardboard often represent **two different
game objects**. A generator's counter record must allow the back to be an independent
counter, not only a transformation of the front.

### 2.10 Markers

Markers use the same tile and ground colour as their country, with a title, often an
icon, and sometimes a signed number. See §3.6.

---

## 3. Elements Specific to This Game

### 3.1 Delay stripe

A **black band across the bottom of the counter, behind the value line** or the unit-type
label. It means the counter goes to the Delay Box, not the Force Pool, when removed
from the map. The photograph shows it clearly: the Kwantung `3-2-1` HQ and the `0-3-0`
fortress have it; the `3-3-1` infantry beside them do not.

A band slot, present or absent, whose colour contrasts with the ground. The value text
sits on top of it.

### 3.2 Symbol-box colour as a quality flag

The rules: "Unit-type symbol color indicates a 'special' unit type. Not all unit-type
symbols are colored; those units are normal units." **White** means a **colonial** unit;
**any other colour** means an **elite** unit. So the box fill carries a three-valued
quality field — normal, colonial, elite — independent of the ground colour.

Tarawa uses symbol colour for elite status too, and TRC uses box fill for Guards. All
three games give the symbol box its own colour channel.

### 3.3 Support units have a different layout

Air and fleet support units do not use the symbol box at all. The rules' samples:

**Air support unit:**

```
+------------------------------+
| [A]                    (3)   |   reinforcement letter;  range in a circle
|                              |
|      [ aircraft icon ]  [1]  |   silhouette;  historical ID
|                              |
|==========[Air Force]=========|   type label on the delay-stripe band
+------------------------------+
```

**Fleet support unit:**

```
+------------------------------+
| [4]      [TF 11/17]          |   reinforcement number;  historical ID
|                              |
|      [ ship silhouette ]     |   side-view ship
|                              |
|==========[CV Fleet]==========|   type label on the band
+------------------------------+
```

Additional small marks on support units:

- a **down arrow** on Bombers and Sub Fleets: they cannot contest support-unit placement;
- an **up arrow** on Interceptors: they can *only* contest placement;
- a **die-roll-modifier square** holding a signed number, coloured by who it benefits —
  black for the Axis, green for the West, red for the Soviets, white for all.

So support units need: a silhouette slot, a type-label band, a range circle, an arrow
glyph, and a coloured modifier square.

### 3.4 Headquarters as a checkerboard

Dai Senso draws HQs as a black-and-white checkerboard filling the symbol box, where TRC
and Tarawa write `HQ`. A generator needs HQ as one semantic value with several possible
renderings.

### 3.5 Multi-national and dependent units

Some Japanese counters represent formations raised in dependent territories —
*Japanese Dependent Hainan*, *Japanese Dependent Inner Mongolia*, *Papua*, *Burma* — and
carry the territory name as text. Some Axis HQs are multi-national and take a second
nationality from a step placed in a holding box. The counter carries a **name field**
that is a place, not a unit.

### 3.6 Political and status markers

A large family with no counterpart in TRC or Tarawa. Recurrent visual devices:

| Device | Examples |
|---|---|
| Title text only | *Neutrality*, *Armistice*, *Enforced Peace*, *Lend-Lease* |
| Title plus a **signed number** | `+1`, `-1`, *War Production −1*, *Pacific USCL* |
| A **hexagon outline** around the title | *Allied Collapse* (green), *Oil Embargo* (orange) — echoing the strategic hexes on the map |
| A **national emblem or roundel** | *British Entry* roundel, *US Entry* star, Japanese flag on *Government Army* |
| A **flag** icon | *Soviet Influence*, *Western Influence* |
| A simple **pictogram** | A-bomb mushroom cloud, parachute, anchor, pennant, half-black circle for *Partisan Base* |
| A **diagonal two-colour split** of the ground | several dependent and political markers, and *Minor Country Prod* |

The diagonal split's meaning is not stated in the pages read. It is recorded here as a
presentation device the library should support, without assigning it a meaning.

### 3.7 Blitz, weather and war-state markers

Operational-state markers use short bold titles on distinctive grounds: *BLITZ*, *MUD*,
*STORMS*, *V-J Day*, *Pacific War State*, *European Limited War*, *European Total War*.
Some carry an instruction in small type: *Open City in this hex*, *No Exit ZOC*. So a
marker may carry a **rules reminder** as a secondary text line.

### 3.8 Two printings

The 2008 Triumvirate printing and the 2011 Decision Games printing of Front 2 show the
same counters with different minor-country colour assignments and band layouts. That
confirms the colours are a palette choice, not part of the information.

---

## 4. Layout Slots

The ground-unit face, from the rules' labelled sample:

```
+------------------------------+
| [44]       xxxx        o o   |   reinforcement no.;  echelon;  step dots
|                              |
| [nat]  [ symbol box ] [hist] |   nationality ID, rotated;  historical ID, rotated
|                              |
|==========[ 7-6-2 ]===========|   attack-defence-movement, on the optional delay band
+------------------------------+
```

| Slot | Ground unit | Air unit | Fleet unit | Marker |
|---|---|---|---|---|
| Upper left | reinforcement code | reinforcement letter | reinforcement number | reinforcement code, sometimes |
| Top centre | echelon | — | historical ID | title |
| Upper right | step dots or squares | range in a circle | — | DRM square, sometimes |
| Left of centre | nationality ID, rotated | — | — | — |
| Centre | symbol box | aircraft silhouette | ship silhouette | icon, emblem or large number |
| Right of centre | historical ID, rotated | historical ID | — | — |
| Bottom | attack – defence – movement | type label | type label | title or value |
| Bottom band | delay stripe, optional | delay stripe | delay stripe | delay stripe, optional |
| Anywhere small | — | up or down arrow; DRM square | down arrow; DRM square | — |

---

## 5. Rendering Implications

### 5.1 What a counter record needs for this game

- country, which selects ground and text colour
- counter family: ground unit, air support, fleet support, marker
- unit type, and a quality flag: normal, colonial, elite
- echelon: army, war zone, or none
- attack, defence and movement
- step count and step-mark shape: dot or square
- historical ID, nationality ID, and either `Exp` or `Res`
- reinforcement code with its type: card number, event letter, `N`, `®`, `*`, `ASR`
- delay stripe: yes or no
- for support units: silhouette, type label, range, arrow, DRM value and beneficiary
- for markers: title, optional icon, optional signed number, optional reminder text,
  optional hexagon outline, optional diagonal split
- a back face that may be an **independent counter record**

### 5.2 Pieces needed

**From an APP-6 / MIL-STD-2525 source:** infantry, cavalry, armour, mechanized infantry,
armoured reconnaissance main icons; airborne and amphibious modifiers; army and army-group
echelon amplifiers; fixed-wing air icons for fighter and bomber; sea-surface icons for
carrier and surface combatant; a sub-surface icon for the submarine.

**Custom:** the HQ checkerboard; the garrison mark; fortress and port-a-fort; step dots
and squares; the range circle; the up and down arrows; the DRM square; the delay band;
the hexagon outline; the diagonal split.

**Silhouettes and emblems:** aircraft, ships, flags, roundels and pictograms are artwork.
Accept supplied SVG, and fall back to the APP-6 air and sea icons for the silhouettes.

### 5.3 SVG notes

- A unit-square `viewBox` per face.
- Rotated `<text>` on both sides of the symbol box for nationality and historical IDs.
- The delay band as a full-width `<rect>` drawn before the value text.
- The diagonal split as a `<polygon>` covering one triangle of the tile.
- The hexagon outline as a `<polygon>` with a thick stroke behind the title.
- Because the back can be a different counter, the sheet writer must pair front and back
  records explicitly and mirror the back grid for duplex printing.

Copyright Ben Paul Wise. All Rights Reserved.
