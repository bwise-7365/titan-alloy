Copyright Ben Paul Wise. All Rights Reserved.

# D-Day at Tarawa — Counter Inventory

**Purpose.** This document describes the unit counters and markers of *D-Day at
Tarawa* (John Butterfield, Decision Games, 2014) as input to a counter-generating
library. It follows the brief in `military-unit-icons-handoff.md`: describe **what
information each counter carries and where**, in abstract terms, so that software
can convey the same information. It does **not** try to reproduce the printed
artwork, fonts, corner treatments or photographs.

**Sources.** There is no scanned counter sheet in the game folder. The inventory
below is built from three documents:

| Source | What it shows |
|---|---|
| `Copy_of_D-Day_at_Tarawa_USMC_OOB_Chart_V1.1.pdf` | Every US unit counter, full-strength face, arranged by turn and beach |
| `4-step_Companies.pdf` | The four-step infantry company counters, full-strength face |
| `DDAT-Rules_V13.pdf`, pp. 5–6, §2.2–2.26 | Labelled anatomy of US and Japanese units, both faces; all Japanese unit types; depth markers; every marker type |

Because the Japanese counters appear only as rules samples, the Japanese inventory is
complete by *type* but not by individual counter. That is enough for a specification.

---

## 1. Overview

### 1.1 Counter families

| Family | Examples | Faces |
|---|---|---|
| US combat units | infantry, heavy infantry, engineer, artillery, tank | full strength / reduced |
| US headquarters | Hall 8/2, Shoup 2/2, Holmes 6/2 | hero side / non-hero side |
| US LVT markers | 1/A/2AT | two steps / one step |
| Japanese units | infantry, engineer, HQ, anti-tank, machine-gun infantry, tank | revealed / unrevealed |
| Japanese depth markers | coastal, inland, armour | revealed / unrevealed |
| Game-state markers | turn, phase, action letters, hero, disrupted, garrison, smoke… | usually two related states |

### 1.2 The big differences from the other two games

- **No size or echelon marks.** Every US unit is a company or smaller, and none
  carries the XX/XXX/XXXX row that TRC and Dai Senso print above the unit symbol.
  Size is implied by the designation.
- **Both faces are used for state.** A US unit's back is its reduced-strength
  version. A Japanese unit's back is a *concealed* version showing only its general
  type. A headquarters' back is the same headquarters after its commander is hit.
- **Printed data that is not strength.** US units carry a random-selector shape (the
  target symbol); Japanese units carry a list of US weapons needed to defeat them.
- **Colour means side and elite status**, not nationality. There are only two sides
  and one nation each.

---

## 2. Standard Elements

Elements that also appear, in some form, on the TRC and Dai Senso counters.

### 2.1 Counter tile and background colour

A square tile filled with one flat colour per side:

| Side | Background | Text colour |
|---|---|---|
| US Marines | olive yellow-green | white for values, black for small text |
| Japanese | pale cream | red for values and requirement codes, dark for formation |

The background colour is the only indication of side. Frame shape is not used (see
§2.2).

### 2.2 Unit symbol box and branch icon

A small horizontal rectangle, centred on the counter a little above the middle,
containing a branch icon in the NATO style. The rectangle is the same shape for both
sides — there is no diamond or other shape for the enemy, as there would be in
APP-6 or MIL-STD-2525.

| Unit type | Printed icon | Nearest APP-6 main icon |
|---|---|---|
| Infantry (US, Japanese) | box with a diagonal cross | Infantry |
| Heavy infantry (US) | infantry cross with a solid bar filling the left part of the box | no exact match; Infantry plus a custom fill |
| Engineer (US, Japanese) | box with the bridge-like "ш" symbol | Engineer |
| Artillery (US) | box with a solid dot in the centre | Field Artillery |
| Headquarters (US, Japanese) | box with the letters `HQ` | no exact match; APP-6 marks HQ with a staff line |
| Anti-tank (Japanese) | box with an inverted V | Anti-armour |
| Machine-gun infantry (Japanese) | infantry cross with an added vertical mark | no exact match; Infantry plus a machine-gun modifier |
| Tank (US, Japanese) | **side-view tank silhouette**, no box | Armour, or an equipment icon for a tank |
| LVT (US marker) | side-view amphibious tractor silhouette, no box | Amphibious modifier on an armoured vehicle icon |

Note that tanks and LVTs abandon the symbol box for a picture of the vehicle. A
generator therefore needs an **image slot** that can replace the symbol box, not just
a set of box icons.

### 2.3 Value line

The largest text on the counter, along the bottom edge.

| Counter | Value line | Meaning |
|---|---|---|
| US infantry | `7-2` | attack strength – range |
| US heavy infantry | `8-3` | attack strength – range |
| US tank | `4-7` | attack strength – range |
| US artillery | `4-U` | attack strength – unlimited range |
| US engineer | `2` | attack strength only |
| US HQ | `Hero` + `RD` | no strength; status word and the weapon it supplies (radio) |
| Japanese unit | `2` | defence strength |
| Japanese depth marker | `1` | added defence strength |

The separator is a hyphen. A unit without range prints a single number. Range can be
a letter (`U`). So the value line is a short list of tokens, not a fixed pair.

### 2.4 Designation

A short military designation, **across the top** of US and Japanese units:
`A/1/2` (company A, 1st battalion, 2nd Marines), `3/A/1/18`, `1/A/2T`,
`A/1/7SSNL`. On US HQs it is a commander's name and unit: `Hall 8/2`.

On TRC and Dai Senso counters the top-centre position holds the echelon mark and the
designation moves to the right of the symbol. Tarawa uses the top-centre position for
the designation instead.

### 2.5 Step pips

A **vertical column of one to four solid dots down the left edge** of US units, beside
the symbol box. Four dots is a full infantry company; engineers and artillery have
one or two; HQs have none. LVT markers show two.

Four-step units use two physical counters. The rules say the second, replacement
counter is "distinguished by a dark green band"; that band is not visible on the
sources available here.

### 2.6 Arrival information

To the **right of the symbol box**, two short lines: the arrival turn (`1`, `6`,
`17`, `28`) above the beach code (`R1`, `R2`, `R3`, `G`). Event-entry units print
`E` instead of a turn. The rules also list `Rdc` (a replacement counter) and `D+2`
(optional rule only).

### 2.7 Front and back faces

| Counter | Front | Back |
|---|---|---|
| US infantry | full strength: 4 pips, `7-2` | reduced: fewer pips, lower strength, **own weapons listed** |
| US tank | 2 pips, `4-7` | 1 pip, `2-7` |
| US HQ | commander's name, `Hero RD` | same, without hero status |
| LVT | 2 pips | 1 pip |
| Japanese unit | formation, symbol, requirements, strength | **concealed: symbol only**, no data |
| Depth marker | illustration, requirement code, strength | **concealed: type label only** (`Coastal Depth`, `Inland Depth`, `Armor Depth`) |
| Admiral Shibasaki | `In Command` | `Killed` |

### 2.8 Markers

Markers are tiles with a coloured ground and a word or two, sometimes with an icon or
a photograph:

| Marker | Presentation |
|---|---|
| Turn, Phase | blue ground, one word |
| Japanese action letters | orange ground, one large letter (`A`, `I`, `M`, `P`, `R`) |
| US Action Taken | green ground, three words |
| US Command Post Range | green ground, the HQ symbol box, a designation, `COMMAND RANGE` |
| US Support | green ground, star, `SUPPORT` |
| US naval fire | blue ground, `Naval Gunfire` and a value `9-U` |
| No LVT Comm | green ground, three words |
| Hero, Garrison, Disrupted, Smoke, Artillery Destroyed, Admiral Shibasaki | photographic background with a caption |

The photographs are artwork and should not be copied. Each can be replaced by a flat
ground, a simple icon, and the caption.

---

## 3. Elements Specific to This Game

### 3.1 Target symbol

A solid black **circle, diamond or triangle** in the lower-left corner of every US
unit, just left of the value line. It is a random selector: fire cards and events name
one of the three shapes, and units bearing it are affected. It has nothing to do with
unit type or strength.

For the library this is a **game-specific glyph slot** holding one of a small set of
geometric shapes. The shapes are simple enough to draw directly; no piece library is
needed.

### 3.2 Weapon codes as printed text

Two-letter codes appear as small stacked text:

- On a **reduced US unit**, at the lower right: the weapons that unit still has —
  `BR DE RD`. On full-strength engineers, on the front: `DE FT` or `SP`.
- On a **Japanese unit or depth marker**, at the lower left in red: the US weapons
  needed to defeat it — `FL RD`, `MG FT`, `AR`, `CC`.

The vocabulary is fixed: `BZ MG BR MO RD DE FT AR NA SP FL CC`. The slot holds one to
three codes stacked vertically.

### 3.3 Symbol colour as the elite flag

On Japanese units the colour of the symbol box and its icon carries elite status: **red
for elite** formations, **white for non-elite**. The background stays cream either way.
So the symbol box has its own stroke colour, independent of the counter's background
and of the value text.

### 3.4 Tank position colour

Each of the three Japanese tank counters carries a **small coloured disc** in the upper
left — red, blue or green. It links the tank to a colour of defensive position on the
map. A small solid disc in a corner slot, one colour from the map's six-colour
position palette.

### 3.5 The concealed face

The Japanese back face shows only the unit-type symbol on the cream ground — no
formation, no strength, no requirements. A generator must therefore be able to produce
a **reduced face from the same unit record** by suppressing every slot except the
symbol.

### 3.6 Depth markers

Depth markers are not units; they sit under a Japanese unit and add strength. Their
front is a photograph with a requirement code and a strength in the lower left; their
back is a solid ground with a two-word type label. The photograph is decoration and can
be replaced by a flat ground, a type label, the code and the number.

### 3.7 Heavy-infantry marking

Heavy infantry (companies D, H and M, which carry machine guns and mortars) are shown by
a **solid bar filling the left portion of the infantry box**. This is not a standard
APP-6 modifier. It is a partial fill of the symbol box, and a generator needs to support
it as a box-fill variant.

### 3.8 HQ status text

US HQs have no strength. Their value-line slot carries the word `Hero` and the weapon code
`RD`, in the same size as a strength would be. So the value line can hold words, not only
numbers.

---

## 4. Layout Slots

The US unit face, schematically. Positions are approximate and relative to the tile.

```
+------------------------------+
|        [designation]         |   top centre:     A/1/2
| o                            |
| o   [ symbol box ]  [turn]   |   left column:    step pips
| o                   [beach]  |   right of box:   arrival turn, beach code
| o                            |
| [target] [value line] [wpns] |   bottom:         target shape, 7-2, weapon codes
+------------------------------+
```

The Japanese unit face:

```
+------------------------------+
|  (disc)  [formation]         |   top:            A/1/7SSNL; tank colour disc
|                              |
|        [ symbol box ]        |   centre:         red = elite, white = non-elite
|                              |
| [reqs]          [strength]   |   bottom:         FL RD in red; 2 in red
+------------------------------+
```

| Slot | US unit | Japanese unit | Marker |
|---|---|---|---|
| Top centre | designation | formation | title word |
| Upper left | — | tank position disc | — |
| Left column | step pips | — | — |
| Centre | symbol box or silhouette | symbol box or silhouette | icon or large letter |
| Right of centre | arrival turn over beach | — | — |
| Lower left | target shape | weapon requirements | — |
| Bottom | value line | strength (right-aligned) | value, e.g. `9-U` |
| Lower right | weapon codes | — | — |

---

## 5. Rendering Implications

### 5.1 What a counter record needs for this game

- side (US or Japanese), which selects the background and text colours
- unit type, which selects the icon or the silhouette
- elite flag (Japanese), which selects the symbol colour
- designation string
- step count, and the maximum step count, for the pips
- value tokens, a short list such as `["7", "2"]`, `["4", "U"]` or `["Hero", "RD"]`
- target shape: circle, diamond, triangle or none
- weapon codes, and whether they are *possessed* (US) or *required* (Japanese), which
  decides colour and position
- arrival turn and beach code
- optional position-colour disc
- a description of the back face, usually derived from the same record by rule: fewer
  steps and lower values, or all data suppressed

### 5.2 Pieces needed

**From an APP-6 / MIL-STD-2525 source** (JMSML SVG files or a milsymbol extraction):
infantry, engineer, field artillery, anti-armour main icons; amphibious modifier;
armour, or a tank equipment icon, as a fallback for the silhouette.

**Custom, small and simple:** the `HQ` lettering inside the box; the heavy-infantry
partial fill; the machine-gun vertical mark; the three target shapes; the step pip; the
position-colour disc.

**Silhouettes** for the tank and the LVT are artwork. A generator should accept a
supplied SVG for them, and fall back to the APP-6 armour or equipment icon when none is
supplied.

### 5.3 SVG notes

- One `<svg>` per face with a unit-square `viewBox`, so that all positions and text
  sizes are fractions of the counter size.
- The symbol box as a `<rect>` with its own stroke colour; the icon placed inside it
  with `<use>` from a piece library.
- Step pips as `<circle>` elements in a column, count driven by data.
- Stacked weapon codes as separate `<text>` lines rather than one line with breaks.
- Target shapes as `<circle>`, a rotated `<rect>`, and a `<polygon>`.
- Markers drawn from a generic template — ground colour, title, optional icon,
  optional value — rather than as one-off artwork.

Copyright Ben Paul Wise. All Rights Reserved.
