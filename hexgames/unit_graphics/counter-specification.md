Copyright Ben Paul Wise. All Rights Reserved.

# Counter Specification — Synthesis Across Three Games

A synthesis of `d-day-at-tarawa-counters.md`, `the-russian-campaign-counters.md` and
`dai-senso-counters.md`, written as the specification called for in
`military-unit-icons-handoff.md`.

The handoff asks for "a specification — not a reproduction", describing per counter:
frame shape and affiliation, the slot carrying the branch icon, how unit size is
marked, where designation and other text sit and what each field encodes, and what
the colour coding means. It asks that the software **convey the same information**
without copying fonts, corner treatments or other stylistic execution. This document
answers those five questions across the three games, then proposes a slot grid, a
C++ record, an SVG output model, and a sourcing plan for the composable pieces.

The three games span nearly forty years of counter design — Avalon Hill 1976, Jedko,
Triumvirate 2008, Decision Games 2011 and 2014, Compass 2020, Consim / GMT 2022 — and
the same information design recurs across all of them with only surface variation.
That is the strongest evidence that a generator can approximate them all.

---

## 1. The handoff's five questions

### 1.1 What frame shape maps to which affiliation?

**None. No counter in any of the three games uses frame shape for affiliation.**

APP-6 and MIL-STD-2525 distinguish friend, hostile, neutral and unknown by the frame:
rectangle, diamond, square, quatrefoil. Wargame counters never do this. Every unit on
every side uses the **same plain rectangle** for its symbol box, and the side is shown
by **colour** instead.

The reason is structural. A wargame counter is not viewed from one side's perspective;
both players look at the same pieces, and a German unit is "friendly" to one player and
"hostile" to the other at the same time. Colour is the only affiliation cue that means
the same thing to both players.

**Specification:** the symbol frame is always a rectangle for ground units; affiliation
is carried by the palette. An *optional* doctrinal mode could draw APP-6 affiliation
frames from one player's point of view, but none of the use cases requires it.

### 1.2 Which visual slot carries the branch or unit-type icon?

**The centre of the counter, a little above the middle, inside a small horizontal
rectangle** — the "symbol box". This is consistent across every edition of every game.

Two kinds of exception:

- **Silhouettes replace the box.** Tanks and LVTs in Tarawa; aircraft, ships and some
  tanks in Dai Senso; Stukas, partisans and — by player choice — tank armies in TRC.
  The TRC 5th edition prints some units in both styles and says there is "no functional
  distinction between the two types."
- **Emblems replace the box** for leaders and some markers: a star, a cross, a flag, a
  roundel.

**Specification:** the centre slot holds one of three things — a NATO-style symbol box
with a branch icon and modifiers, a silhouette, or an emblem. These are alternative
renderings of the same field.

### 1.3 How is unit size / echelon marked?

**A row of X's centred directly above the symbol box**, in TRC and Dai Senso.

| Mark | TRC | Dai Senso |
|---|---|---|
| `XXX` | corps | — |
| `XXXX` | army | army |
| `XXXXX` | army group | war zone |
| `XXXXXX` | high command | — |

This matches the APP-6 echelon amplifier convention in both meaning and position.

**Tarawa has no echelon marks.** All its units are companies or smaller, and the space
above the box holds the designation instead.

**Specification:** an optional echelon field drawn with the APP-6 amplifier. When absent,
the top-centre slot is free for other text.

### 1.4 Where does the designation and other text sit, and what does each field encode?

A consistent set of positions around the symbol box:

| Position | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| Top centre | designation (`A/1/2`) | echelon | echelon |
| Upper left | tank colour disc (Jp) | set-up code | reinforcement code |
| Upper right | — | — | step marks; or range circle |
| Left column | step pips (US) | — | — |
| Left of box | — | nationality letter (minor allies) | nationality ID, rotated |
| Right of box | arrival turn over beach | designation, rotated or not | historical ID, rotated |
| Bottom, large | strength or strength–range | combat – movement | attack – defence – movement |
| Lower left | target shape (US); weapon requirements (Jp) | — | — |
| Lower right | weapon codes | — | — |
| Bottom band | replacement band (per rules) | caption band (backs) | delay stripe |

**The value line is the dominant element in every game** — the largest text on the
counter, across the bottom. It holds one, two or three tokens separated by hyphens.
Tokens are usually numbers but can be letters (`U` for unlimited range) or words (`Hero`,
`RD`).

**Rotated text** is common: TRC's 1976 and 5th editions and all of Dai Senso rotate the
designation 90° along the side of the box. The TRC 2020 remake sets it horizontally. So
rotation is a per-field choice, not a fixed property.

### 1.5 What does the colour coding mean?

Colour is layered. Five independent channels appear:

| Channel | Tarawa | TRC | Dai Senso |
|---|---|---|---|
| **Tile ground** | side (US / Japan) | nationality or formation (9 values) | country (many) |
| **Text colour** | side | light-on-dark for SS | distinguishes minor countries sharing a ground |
| **Symbol-box fill or stroke** | elite (Jp red vs white) | Guards (red), Italian, Luftwaffe | quality: colonial (white), elite (other) |
| **Bands** | four-step replacement | caption band on backs | delay stripe |
| **Small tiles and squares** | position-colour disc | year of arrival (backs) | DRM beneficiary (black / green / red / white) |

**Specification:** a palette keyed by nationality or formation gives ground, text, box
stroke and box fill. Quality flags (elite, colonial, Guards) override the box fill. Bands
and small tiles take their colour from data, not from the palette.

---

## 2. A common slot grid

All three games fit one grid of named slots on a unit square:

```
+---------------------------------------+
| UL        |      TC        |       UR |
|           |                |          |
| ML        |    CENTRE      |       MR |
|           |                |          |
| LL        |    BOTTOM      |       LR |
|=========== BAND (full width) ==========|
+---------------------------------------+
         plus a LEFT COLUMN strip for pips
```

| Slot | Typical content | Rotation allowed |
|---|---|---|
| `UL` upper left | set-up or reinforcement code; small disc | no |
| `TC` top centre | echelon amplifier, or designation, or marker title | no |
| `UR` upper right | step marks; range circle; DRM square | no |
| `ML` middle left | nationality code | yes, 90° |
| `CENTRE` | symbol box, silhouette or emblem | no |
| `MR` middle right | designation or historical ID; arrival info | yes, 90° |
| `LL` lower left | target shape; requirement codes | no |
| `BOTTOM` | value line, the largest text | no |
| `LR` lower right | weapon codes | no |
| `BAND` | delay stripe; caption band; type label | no |
| `LCOL` left column | step pips, vertical | no |
| free | diagonal text on backs; up/down arrows | any angle |

Positions should be expressed as fractions of the counter size, so the same layout
scales from half-inch to one-inch counters.

---

## 3. Element catalogue by frequency

`Y` present; `—` absent.

| Element | Tarawa | TRC | Dai Senso | Recommendation |
|---|:--:|:--:|:--:|---|
| Square tile, flat ground colour by side | Y | Y | Y | core |
| Rectangular symbol box, same for all sides | Y | Y | Y | core |
| NATO-style branch icon in the box | Y | Y | Y | core, from APP-6 sources |
| Value line of hyphenated tokens | Y | Y | Y | core |
| Designation beside or above the box | Y | Y | Y | core |
| Silhouette replacing the box | Y | Y | Y | core, as an image slot |
| Symbol-box colour as a quality flag | Y | Y | Y | core |
| Two faces with different content | Y | Y | Y | core |
| Generic markers: ground, title, icon, value | Y | Y | Y | core template |
| Echelon amplifier above the box | — | Y | Y | core, optional field |
| Rotated side text | — | Y | Y | core |
| Nationality code beside the box | — | Y | Y | core |
| Set-up / reinforcement code | Y | Y | Y | core |
| Step marks, with the mark's shape (dot, square) carrying its own meaning | Y | — | Y | core |
| Full-width band | Y | Y | Y | core |
| Target shape (random selector) | Y | — | — | game-specific glyph |
| Weapon or requirement codes | Y | — | — | game-specific text slot |
| Concealed face showing type only | Y | — | — | derived face |
| Colour-coded date tile on the back | — | Y | — | game-specific |
| Diagonal text on the back | — | Y | — | free-rotation text |
| Range circle | — | — | Y | game-specific glyph |
| Up / down arrows | — | — | Y | game-specific glyph |
| DRM square coloured by beneficiary | — | — | Y | game-specific glyph |
| Hexagon outline behind a title | — | — | Y | marker device |
| Diagonal two-colour split of the ground | — | — | Y | marker device |
| Back face as an independent counter | — | — | Y | core record design |

---

## 4. Composable pieces — where each comes from

### 4.1 From APP-6 / MIL-STD-2525 sources

The handoff names two candidate sources: the DISA / Esri `joint-military-symbology-xml`
SVG files (Apache 2.0, one file per symbol component) and `milsymbol` (MIT, path data
extractable by a one-time Node script). Either can supply:

| Needed for | APP-6 component |
|---|---|
| Infantry | land unit main icon: infantry |
| Armour | land unit main icon: armour |
| Mechanized / motorized infantry | land unit main icon: mechanized infantry |
| Cavalry | land unit main icon: cavalry / reconnaissance |
| Cav-mech | land unit main icon: armoured reconnaissance |
| Engineer | land unit main icon: engineer |
| Artillery | land unit main icon: field artillery |
| Anti-tank | land unit main icon: anti-armour |
| Mountain | sector modifier: mountain |
| Airborne, paratroop | sector modifier: airborne |
| Marine | sector modifier: amphibious |
| Echelon marks, corps to high command | echelon amplifiers |
| Fighter, bomber (fallback for silhouettes) | air symbol set: fixed-wing icons |
| Carrier, surface fleet (fallback) | sea surface symbol set |
| Submarine (fallback) | sea subsurface symbol set |
| Tank (fallback for silhouettes) | land equipment: tank |

Only the **main icons, modifiers and echelon amplifiers** are needed. The APP-6
**frames** are not used — every counter uses a plain rectangle (§1.1).

### 4.2 Custom pieces — small, simple, and not in APP-6

| Piece | Used by |
|---|---|
| `HQ` lettering inside the box | Tarawa, TRC |
| HQ checkerboard | Dai Senso |
| `W`, `P` lettering inside the box | TRC |
| Garrison mark | Dai Senso |
| Fortress (empty box), port-a-fort (trench lines) | Dai Senso |
| Heavy-infantry partial fill | Tarawa |
| Machine-gun vertical mark | Tarawa |
| Step pip (dot) and step square | Tarawa, Dai Senso |
| Target shapes: circle, diamond, triangle | Tarawa |
| Position-colour disc | Tarawa |
| Range circle | Dai Senso |
| Up and down arrows | Dai Senso |
| DRM square | Dai Senso |
| Hexagon outline | Dai Senso |
| Date tile with rounded corners | TRC |

All are a few SVG primitives each. None needs a piece library.

### 4.3 Artwork the generator should not try to create

Silhouettes of specific vehicles, aircraft and ships; national emblems and roundels;
leader portraits; the photographs on Tarawa's markers; TRC 5th edition's wood-grain
texture. The generator should **accept supplied SVG** for silhouettes and emblems, and
otherwise fall back to the APP-6 equivalent or to a flat ground with a caption.

---

## 5. Proposed C++ model

A sketch; names are placeholders.

```
enum class CounterFamily { GroundUnit, AirSupport, FleetSupport, Leader, Marker };
enum class CentreKind    { SymbolBox, Silhouette, Emblem, LargeText, None };
enum class Quality       { Normal, Elite, Colonial, Guards };
enum class Echelon       { None, Company, Battalion, Regiment, Division,
                           Corps, Army, ArmyGroup, HighCommand };
enum class StepMark      { Dot, Square };

struct PaletteEntry { Colour ground, text, boxStroke, boxFill; };

struct SymbolBox {
    std::string mainIcon;               // e.g. "infantry", "armour", "hq-checker"
    std::vector<std::string> modifiers; // e.g. "mountain", "airborne"
    std::optional<Colour> fillOverride; // quality flag
    std::optional<std::string> partialFill; // heavy-infantry bar
};

struct TextField {
    std::string text;
    Slot slot;
    int rotationDeg = 0;                // 0 or 90 for side text; any for backs
    SizeClass size;                     // Small, Medium, Large, ValueLine
};

struct CounterFace {
    PaletteKey palette;
    CentreKind centreKind;
    std::variant<SymbolBox, ImageRef> centre;
    Echelon echelon = Echelon::None;
    std::vector<std::string> valueTokens;   // {"7","2"}, {"3","3","1"}, {"Hero","RD"}
    std::vector<TextField> texts;
    int steps = 0, maxSteps = 0; StepMark stepMark = StepMark::Dot;
    std::vector<Glyph> glyphs;              // target shape, range circle, arrows, DRM, disc
    std::optional<Band> band;               // delay stripe, caption band, type label
    std::optional<Split> diagonalSplit;
};

struct CounterRecord {
    CounterFamily family;
    CounterFace front;
    std::variant<CounterFace, DerivedBack, CounterRecordRef> back;
};
```

Three points follow from the use cases.

- **`back` is a variant.** Tarawa derives the back from the front by rule (fewer steps,
  or everything but the symbol suppressed). TRC gives the back its own content
  (set-up information). Dai Senso often makes the back a **different counter**
  altogether. All three must be expressible.
- **`valueTokens` is a list of strings, not a pair of integers.** Tarawa has one token
  or two, sometimes letters; TRC has two; Dai Senso has three.
- **`centre` is a variant.** Symbol box, silhouette and emblem are alternative
  renderings of one field.

---

## 6. SVG output model

### 6.1 One counter face

```
<svg viewBox="0 0 100 100" class="counter ground-unit jp">
  <rect class="ground" width="100" height="100"/>
  <polygon class="split" points="..."/>              <!-- optional -->
  <rect class="band delay" y="78" width="100" height="16"/>   <!-- optional -->
  <g class="symbol" transform="translate(30,32)">
    <rect class="box" width="40" height="26"/>
    <use href="#icon-infantry"/>
    <use href="#mod-mountain"/>
  </g>
  <use class="echelon" href="#ech-army" transform="translate(50,26)"/>
  <text class="value" x="50" y="92">3-3-1</text>
  <text class="natid" transform="translate(24,45) rotate(-90)">Kwa</text>
  <text class="histid" transform="translate(76,45) rotate(90)">16</text>
  <text class="reinf" x="6" y="12">44</text>
  <g class="steps"> <circle .../> <circle .../> </g>
</svg>
```

- **Unit-square `viewBox`**, so every position and font size is a fraction of the
  counter and one layout scales to any counter size.
- **Pieces by `<use>`** from a `<defs>` library assembled from the APP-6 sources and the
  custom set.
- **Classes on everything**, so a stylesheet supplies the palette. That keeps the
  information separate from the look, which is the point of the handoff's
  "approximate, don't duplicate" rule, and makes it easy to produce a printer-friendly
  or colour-blind-safe variant.

### 6.2 Counter sheets

- Counters laid out on a grid at a chosen size and gutter.
- **The back sheet is the front grid mirrored left to right**, so that duplex printing
  puts each back behind its own front. The TRC 2020 remake sheet shows exactly this.
- Front and back paired by record, since the back may be a different counter.
- Optional crop marks and a bleed around each counter.

### 6.3 Fonts and corners

- Use a freely licensed condensed sans-serif for text and a bold condensed face for the
  value line. Do not use the fonts on the source sheets.
- Use square corners, or a single uniform radius chosen by the user. Do not reproduce any
  source sheet's corner treatment.

---

## 7. What not to copy

Following the handoff:

| Do not copy | Convey instead |
|---|---|
| Exact fonts | a free condensed sans; the value line largest |
| Corner treatments, bevels, drop shadows | square or uniformly rounded tiles |
| Wood-grain and other textures | flat grounds |
| Photographs on markers | flat ground, title, optional pictogram |
| Specific silhouette drawings | supplied SVG, or the APP-6 equipment icon |
| Exact hues | a palette that keeps the same *distinctions* |
| Exact positions to the pixel | the slot grid in §2 |

The test is whether a player who knows the original counter can read every field on the
generated one — not whether the two look alike.

---

## 8. Open questions

- **Which APP-6 source?** JMSML gives ready SVG files per component but is archived;
  milsymbol is maintained but needs an extraction script. The piece set needed is small —
  about a dozen main icons, three modifiers, six echelon marks, and a handful of air and
  sea fallbacks — so either is workable.
- **How much of the marker family belongs in the library?** Dai Senso's political markers
  are numerous and varied. A generic marker template (ground, title, optional icon,
  optional signed value, optional reminder line, optional outline, optional split) covers
  every example seen, but it is broad.
- **Tarawa's Japanese counters** are known only from rules samples. The type inventory is
  complete; the individual counters are not.
- **Dai Senso backs** are known only from the rules' description. No back sheet was
  available.
- **Doctrinal mode.** Should the generator offer true APP-6 affiliation frames as an option,
  for users who want military-manual output rather than wargame counters? The handoff
  mentions both styles. It costs little, since the frames are in the same piece sources.

Copyright Ben Paul Wise. All Rights Reserved.
