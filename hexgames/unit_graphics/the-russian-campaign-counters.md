Copyright Ben Paul Wise. All Rights Reserved.

# The Russian Campaign — Counter Inventory

**Purpose.** This document describes the unit counters and markers of *The Russian
Campaign* as input to a counter-generating library. It follows the brief in
`military-unit-icons-handoff.md`: describe **what information each counter carries
and where**, in abstract terms, so that software can convey the same information. It
does **not** try to reproduce the printed artwork, fonts, corner treatments or
textures.

**Sources.** The game folder holds counters from four different editions. Taken
together they are more useful than any single one, because they show the **same
information laid out four different ways** — which is exactly the room a generator
has when it approximates rather than copies.

| Edition | Source in the folder | What it shows |
|---|---|---|
| 3rd edition, Avalon Hill, 1976 | `TRC3-77/German_OOB_1.png`, `Russian_OOB_1.png` | every unit, drawn as line art on the order-of-battle charts |
| 3rd edition, Avalon Hill, 1976 | `TRC3-77/HPSCANS/scan_001.jpeg` | labelled counter diagram, rules §2 |
| 1st edition, Jedko | `TRC 1ed map with pieces opening v1.jpg` | photograph of the printed counters on the board |
| Remake, Compass Games, 2020 | `TRC-74-remake/trc_units_sep14_page_1_front.jpg`, `page_2_back` | **complete counter sheet, both sides** |
| 5th Deluxe, Consim / GMT, 2022 | `TRC v5 deluxe/The Russian Campaign 5th Rules - GMT 2022.pdf`, p. 3 | labelled counter diagram, both faces; nationality colour table; unit-type and marker lists |
| 5th Deluxe, Consim / GMT, 2022 | `TRC v5 deluxe/GuardsTk_Units.jpg` | a strip of Guards tank armies, in NATO and silhouette styles |

The 2020 remake sheet is the only complete sheet and is used as the primary example.
The 5th-edition rules page is used for field names.

---

## 1. Overview

### 1.1 Counter families

| Family | Examples | Numbers printed |
|---|---|---|
| Combat units | infantry, armour, motorized, mountain, cavalry, paratroop corps and armies | combat – movement |
| Headquarters | German army groups N, C, S; Russian Stavka | combat – movement |
| Leaders | Hitler, Stalin | combat – movement |
| Air units | Stukas, Sturmoviks | none, or an odds shift |
| Workers | Russian industrial units | one number: replacement value |
| Partisans | Russian partisan bands | none |
| Optional units | battle groups, artillery | combat – movement |
| Markers | turn, weather, railhead, sudden death, invasion… | varies |

### 1.2 The big differences from the other two games

- **Every unit has one step.** There are no step pips and no reduced side.
- **The back face carries set-up and arrival information**, not a weaker version of
  the unit. On the 2020 and 5th-edition counters the back tells you where the unit
  starts, or when and where it arrives.
- **Two numbers, not three.** One combat factor serves for both attack and defence.
- **Colour means nationality and formation**, and there are many of them: German,
  four Axis minor allies, SS, Luftwaffe, Russian, and Russian Guards.

---

## 2. Standard Elements

Elements that also appear, in some form, on the Tarawa and Dai Senso counters.

### 2.1 Counter tile and background colour

A square tile with one flat background colour per nationality or formation. The
5th-edition rules print the palette as a table:

| Nationality / formation | 5th ed. background | 2020 remake background |
|---|---|---|
| German | field grey-green | grey-green |
| Italian | olive green | pale green |
| Hungarian | green | mid grey |
| Rumanian | dark green | dark green |
| Finnish | light blue | very pale grey, blue text |
| Luftwaffe | light blue | slate blue |
| SS | near-black | near-black, white text |
| Russian | brown | tan |
| Russian Guards | red (applied to the symbol box — see §3.2) | red symbol box |

The 1976 rules give a shorter version: "Russian units are brown, German are field
gray, German SS are white on black, and German Allies are olive."

The editions disagree on the exact hues but agree on the principle: **one background
colour per nationality**, dark-on-light for most, **light-on-dark for SS**. No edition
uses frame shape for side.

### 2.2 Unit symbol box and branch icon

A small horizontal rectangle, centred a little above the middle of the tile, holding a
NATO-style branch icon. Every edition uses the same rectangle for both sides.

| Unit type | Printed icon | Nearest APP-6 main icon or modifier |
|---|---|---|
| Infantry | diagonal cross | Infantry |
| Armour / panzer | rounded oval | Armour |
| Motorized / mechanized / panzer grenadier | cross plus oval | Mechanized infantry |
| Cavalry | single diagonal | Cavalry / reconnaissance |
| Mountain | cross with a small solid triangle at the bottom | Infantry with the Mountain modifier |
| Paratroop | cross with a small canopy or wing mark | Infantry with the Airborne modifier |
| Headquarters | the letters `HQ` in the box | no exact match; APP-6 marks HQ with a staff line |
| Worker | the letter `W` in the box | no APP-6 equivalent |
| Partisan | the letter `P` in the box (1976); a rifle silhouette (2020) | APP-6 has no close equivalent |
| Leader | a national emblem — star, cross — or a portrait | no APP-6 equivalent |
| Air unit | an aircraft silhouette, no box | an APP-6 fixed-wing air icon |
| Battle group (optional) | a tank silhouette (5th ed.) | Armour |

### 2.3 Echelon marks

A row of X's **centred above the symbol box**:

| Mark | Size (5th ed. rules) |
|---|---|
| `XXX` | Corps |
| `XXXX` | Army |
| `XXXXX` | Army Group |
| `XXXXXX` | High Command |

This matches the APP-6 echelon amplifier convention exactly, and it sits in the same
place — directly above the frame. The 1976 counters draw the X's as tiny bars; the 2020
and 5th-edition counters set them as text.

### 2.4 Value line

The largest text, **across the bottom**: `combat – movement`, for example `8-7`, `4-4`,
`10-8`. Workers print a single number, their replacement value. Stukas print nothing.
Leaders and HQs print ordinary pairs (`1-7`, `1-8`). The 1976 Hitler and Stalin
counters print `1-0` — no movement.

### 2.5 Unit designation

The unit's number, **to the right of the symbol box**: `41`, `56`, `1G`, `24`. In the
1976 and 5th editions it is **rotated 90°** and set vertically along the right side of
the box; in the 2020 remake it is horizontal. Suffixes carry formation: `SS`, `Res`,
`HG` (Hermann Göring), `Pz`, `Cv`, `Gd`.

### 2.6 Nationality letter

For the Axis minor allies, a single letter **to the left of the symbol box**: `R`
Rumanian, `H` Hungarian, `I` Italian, `F` Finnish. German and Russian units carry none;
their nationality is the background colour alone. So nationality is shown twice for the
minor allies — colour and letter — and once for the major powers.

### 2.7 Set-up code

A small code in the **upper-left corner**. The 1976 diagram calls it the *set-up
co-ordinate*; the 5th edition calls it the *set-up location* and uses a letter matching
a map region or reinforcement group. The 5th-edition Guards counters use a number
there instead (`6`, `9`, `10`…).

### 2.8 Back face

On the 2020 and 5th-edition counters, the back face carries arrival or set-up
information:

| Back face content | Example |
|---|---|
| Arrival month pair and year | `Nov-Dec 1941` |
| Arrival date and place | `Jul-Aug 1941 Kursk` |
| Set-up region | `Baltic MD`, `Kiev MD`, `Finnish Border` |
| German army group | `AGN`, `AGC`, `AGS` |
| Minor ally homeland | `Rumania`, `Finland` |
| City set-up | `Moscow`, `Riga`, `Tula` |
| Special handling | `Special` |
| Worker availability | `any City`, `available` |
| 5th edition | unit name, set-up location, reinforcement turn: `30 Inf / Moscow / GT 3` |

### 2.9 Markers

Plain tiles with a word, an icon, or both: `Turn`, `Year`, `Impulse`, weather, railhead
(a locomotive silhouette), sudden death, invasion, fortress city, RR convert, RR move,
para range, scenario ends, RP bid, artillery barrage. The 5th-edition set uses flat
coloured grounds with a short title and a small drawing.

---

## 3. Elements Specific to This Game

### 3.1 Two icon styles for the same unit

The 5th-edition rules say that some units are printed in **both** a NATO-style version
and a silhouette-style version, and players may use either — "there is no functional
distinction between the two types." The Guards tank strip shows eight tank armies twice:
once with a white-outlined box around a red armour oval, once with a red tank silhouette.
Every other field is identical.

This is the clearest endorsement in any of the three games that the icon is a
**replaceable slot**. A generator should treat NATO icon and silhouette as two renderings
of one field.

### 3.2 Symbol-box fill as a formation flag

Several formations are marked not by background colour but by **filling the symbol box**:

- **Russian Guards**: symbol box filled red, on the ordinary Russian background.
- **Italian units** (2020): symbol box filled yellow-green.
- **Luftwaffe ground units** (2020): symbol box filled pale blue.
- **Hungarian units** (2020): symbol box filled white on the grey ground.

So the symbol box has a fill colour independent of the tile background. Two nations can
share a background while their elite formations are distinguished by the box.

### 3.3 The SS inverted scheme

In every edition shown, SS units reverse the scheme: **white text and symbols on a
near-black ground**. The 1976 order-of-battle chart prints them as black tiles with white
lettering even in monochrome line art. A generator needs a per-palette choice of light
or dark text, not a single text colour.

### 3.4 Leaders and headquarters

- **Hitler** and **Stalin** have their own emblem in place of the symbol box — a black
  cross or a red star in the 2020 remake — and their name set above the emblem.
- **Army group HQs** show `HQ` in the box and a letter to the right: `N`, `C`, `S`. The
  1976 chart spells the name out, rotated: `North`, `Centre`, `South`.
- **Stavka** shows `STAVKA` above an `HQ` box.

### 3.5 Workers

Worker units use the Russian ground with a **red box containing a white `W`**, and a
single number beneath: their replacement value, `1` to `3`. Some carry the name of the
city they start in above the box: `Moscow`, `Kharkov`, `Stalino`, `Leningrad`, `Kiev`,
`Archangel`. One prints its value in parentheses, `(2)`.

### 3.6 Partisans and air units — no numbers

Partisans and Stukas carry **no values at all**: a silhouette and a label. So the value
line is optional, and a counter can consist of an icon and a caption only.

### 3.7 Colour-coded back tiles

On the 2020 remake back faces, arrival dates are set on a small **rounded tile whose fill
colour encodes the year**:

| Year | Tile fill |
|---|---|
| 1941 | white |
| 1942 | peach |
| 1943 | light blue |
| 1944 | pink |

A tile may carry a place name beneath the date, and some carry a black band with white
text below: `any City`, `available`. Set-up locations with no date are printed as plain
text on the ground, and city names are often **set diagonally**, at about 45°. `Special`
is set diagonally in red on a white tile.

So the back face needs: a date tile with a data-driven fill, optional place text, an
optional caption band, and free text at an arbitrary angle.

### 3.8 The back sheet is mirrored

On the 2020 remake, the front sheet has German units on the left and Russian on the
right; the back sheet has **Russian on the left and German on the right**. The back sheet
is the mirror image of the front, so that when the two are printed on opposite sides of
one sheet and cut, each back lands behind its own front. A counter-sheet generator must
produce the back as a left-to-right mirror of the front grid.

### 3.9 Textures and artwork that should not be copied

- The 5th-edition Guards counters use a **wood-grain texture** for the brown ground.
- Stuka and partisan silhouettes are drawings.
- The 1976 counters use a specific stencil style for numbers.

All of these are execution details in the sense of the handoff brief. Flat grounds and a
free condensed sans-serif font carry the same information.

---

## 4. Layout Slots

The combat-unit face, common to all four editions with small variations:

```
+------------------------------+
| [set-up]    XXX              |   top-left: set-up code;  top-centre: echelon
|                              |
| [nat] [ symbol box ] [desig] |   left: nationality letter; right: designation
|                              |
|          [ 8 - 7 ]           |   bottom: combat - movement
+------------------------------+
```

The back face (2020 remake):

```
+------------------------------+
|                              |
|        +----------+          |   a date tile, fill colour = year
|        | Nov-Dec  |          |
|        |   1941   |          |
|        |  Moscow  |          |   optional place
|        +----------+          |
|        [any City ]           |   optional caption band
+------------------------------+
```

| Slot | Combat unit | Worker | Leader | Air unit | Back face |
|---|---|---|---|---|---|
| Upper left | set-up code | — | — | — | — |
| Top centre | echelon | city name | name | echelon or none | — |
| Left of centre | nationality letter | — | — | — | — |
| Centre | symbol box or silhouette | `W` box | emblem | silhouette | date tile or text |
| Right of centre | designation, rotated or not | — | — | name label | — |
| Bottom | combat – movement | replacement value | combat – movement | none | caption band |

---

## 5. Rendering Implications

### 5.1 What a counter record needs for this game

- nationality or formation, which selects background, text colour and box fill
- unit type, which selects the icon — with an optional silhouette alternative
- echelon: corps, army, army group, high command
- designation and optional suffix
- nationality letter for the minor allies
- combat factor and movement factor, either possibly absent
- set-up code
- back face: a date and optional place, or a set-up region, or free text; plus an
  optional caption band

### 5.2 Pieces needed

**From an APP-6 / MIL-STD-2525 source:** infantry, armour, mechanized infantry,
cavalry main icons; mountain and airborne modifiers; the echelon amplifiers from corps
to high command; a fixed-wing air icon as a fallback for the Stuka silhouette; armour as
a fallback for the tank silhouette.

**Custom:** `HQ`, `W` and `P` lettering in the box; the national emblems for the two
leaders; the date tile; the caption band.

**Silhouettes:** accepted as supplied SVG, with the APP-6 fallback above.

### 5.3 SVG notes

- A unit-square `viewBox` per face.
- The palette as a small table keyed by nationality and formation, giving ground, text,
  box stroke and box fill. The SS row simply swaps light and dark.
- Designation `<text>` with an optional `rotate(90)` transform, since editions differ.
- Back-face text with an arbitrary rotation for the diagonal city names.
- A sheet writer that lays counters out on a grid, and for the back face emits the same
  grid **mirrored left to right**.

Copyright Ben Paul Wise. All Rights Reserved.
