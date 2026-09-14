Copyright Ben Paul Wise. All Rights Reserved.

# Axis Empires: Dai Senso! — Map Graphics Inventory

**Source.** `C:\Library\War-Games\Dai Senso Pacific WWII\Dai Senso game map adjusted.png`
(3990 × 3198), a scan carrying the West Map in full and part of the East Map.
Element names are taken from the two legend blocks printed on the sheet — the
**Terrain Effects Chart** and the **Terrain Key** — both at the lower right.

This document catalogues the **graphical elements** on the sheet and the address
each one needs, as input to a C++ map-object library and an SVG/HTML renderer. It
is not a rules summary. Nothing here assumes any particular drawing algorithm; the
question is only *what kinds of things must be drawable and addressable*.

Fidelity is deliberately limited. Where the printed art carries cartographic detail
that no rule reads — drawn coastlines, offshore islets, river courses inside a hex —
this document says so and proposes the simplification rather than the reproduction.

Of the three maps examined this one is the richest, and the only one whose own
printed terrain chart is **split into a hex column and a hexside column**. The
designers reached the same conclusion the coordinate system does.

---

## 1. Overview of the Sheet

The full game uses **two 22 × 34 inch sheets**, a West Map reaching from India to
Japan and an East Map reaching from Japan to Hawaii. They are laid side by side and
are geographically continuous but independently numbered, so a hex identifier carries
a sheet prefix: `w5422`, `e4904`.

The hex grid is **flat-topped** in printed orientation and continuous across land and
sea. Sea hexes are fully part of the grid and are entered by ground units only under
specific conditions.

Off-map furniture rings the sheet and also intrudes on it: holding boxes and delay
tracks along the top and left, the turn track and weather-area diagram bottom-left,
the VP track and Ceded Lands box in the middle-bottom, the Terrain Effects Chart and
Terrain Key bottom-right, and — importantly — **a Naval Zone Box printed over sea hexes
for every naval zone**, at whatever angle fits. The rules state explicitly that the
hexes beneath those boxes are still in play.

The artwork separates into layers, drawn back to front:

1. Base geography — land and sea fills, drawn coastlines, islets, inland rivers.
2. Region tints — country and dependent colouring.
3. Hex grid strokes.
4. Hexside features — rivers, mountains, lakes, straits, naval zone borders,
   country and region borders.
5. Centre-to-centre links — the road and rail network.
6. Hex-perimeter rings — strategic hex outlines in three faction colours.
7. Hex-anchored glyphs — cities, towns, ports, limited stacking, oil, aid, US impact.
8. Edge-centred directional glyphs — strait arrows.
9. Text — hex identifiers, place names, area names.
10. Overlaid panels — naval zone boxes and rules reminders.

---

## 2. Standard Elements

These are the graphical kinds that also appear on the other two maps examined, and
that a shared library should provide. Each is given with the **address** it needs.

### 2.1 Hex area fill

A solid fill covering one whole hex. The printed Terrain Effects Chart lists the hex
terrain types directly:

| Terrain | Rendering on this map | Proposed simplification |
|---|---|---|
| Clear (Normal) | pale tan | solid fill |
| Clear (Desert) | yellow-tan | solid fill |
| Rough (Hills) | olive | solid fill |
| Rough (Forest) | mid green | solid fill |
| Rough (Swamp) | grey-green | solid fill |
| All-Sea | mid blue | solid fill |
| Ice | very pale blue with an ice glyph | solid fill plus a centred glyph |

Note that Hills, Forest and Swamp are three *drawings* of one rule value, Rough. Clear
Normal and Clear Desert are likewise two drawings of one rule value, except that Desert
also determines weather-area membership. So the hex needs a **terrain value** and,
separately, a **terrain appearance** — they are not one to one.

*Address:* one hex.

### 2.2 Hex grid stroke

The hexagon boundary, drawn as a thin line, warm grey over land and pale blue-grey over
sea. As on the other two maps, the grid stroke colour varies with the underlying terrain.

*Address:* one hex, drawn as a closed path.

### 2.3 Hex perimeter ring

A **thick coloured stroke following the whole hex boundary**, marking a Strategic Hex —
the objects the victory conditions count. Three colours, one per faction: green for
Western, red for Soviet, orange for Axis.

This is the same element kind as Tarawa's position ring and the Russian Campaign's city
ring, used for a third purpose. It is common enough across the three sheets to belong in
the core library.

*Address:* one hex, plus a colour and an inset.

### 2.4 Hex-centred glyph

An icon drawn at or near the hex centre. This map has more of these than the other two
combined:

- **City** — a yellow filled circle.
- **Capital** — a larger star-like or highlighted symbol.
- **Provisional Capital** — a distinct starred variant.
- **Town** — a small white square outline.
- **Port** — a cyan circular badge with an anchor.
- **Multi-Zone Port** — the same badge with a doubled or ringed treatment.
- **Key Port** — a distinct barred variant.
- **Limited Stacking** — a small white square carrying two or three pips, like a domino.
- **Oil hex** — a derrick silhouette.
- **Aid or Lend-Lease hex** — a small green marker symbol.
- **US Impact hex** — a small white star.
- **Ice** — an ice-floe glyph.

Several of these can coexist in one hex: a coastal capital with a port and a limited
stacking symbol is ordinary. So the per-hex glyph list must hold **several glyphs at
once**, each in its own slot, and the slots need to be laid out so they do not collide.

*Address:* one hex, plus a named slot.

### 2.5 Hex-anchored glyph at an offset

The port anchor is placed toward the water side of its hex rather than at the centre, so
it reads as a harbour. As on the Russian Campaign map, the offset direction follows the
drawn coastline and is a cartographic judgement rather than a rule.

*Address:* one hex, plus an offset or a named slot.

### 2.6 Hex-anchored glyph facing one of its edges

Not used on this map. Dai Senso has nothing equivalent to Tarawa's fire dots.

### 2.7 Edge stroke

A stroke drawn **along a single hexside**. This map carries more distinct hexside types
than either other sheet — the Terrain Key shows them as a group, and the Terrain Effects
Chart gives most of them their own movement cost and combat shift:

| Hexside | Rendering |
|---|---|
| Mountain | brown ridge or sawtooth motif along the edge |
| River | blue line along the edge |
| Lake | blue band with the lake body beside it |
| All-Sea | the boundary between a sea hex and a land hex |
| Strait, connected | see §2.10 |
| Strait, not connected | see §2.10 |
| Naval Zone border | heavy white dashed line |
| Country or Dependent border | orange-and-yellow dashed line |
| Region border | paler dashed line, same family |

*Address:* one edge.

### 2.8 Edge chain (polyline along hexsides)

All of the above are authored as paths running through several hexsides in sequence.
Naval zone borders in particular run for very long distances across open sea, and country
borders enclose whole nations and their dependents.

The three border kinds — country, dependent, region — are drawn in the same visual family
with decreasing weight, which means a renderer needs the family to be parameterised rather
than three unrelated styles.

*Address:* an ordered list of edges, plus a name and a class.

### 2.9 Centre-to-centre link

A stroke from one hex centre to another:

- **Rail** — a white cased line with cross-ticks.
- **Road** — a plain white line, lighter weight.

Both run hex centre to hex centre, branch at towns and cities, and form connected networks.
Small white square nodes mark junctions.

There is a subtlety worth recording. Roads and rails are drawn centre to centre, but the
printed Terrain Effects Chart lists them under **Hexside Terrain Type** and charges their
movement cost per hexside crossed — half a movement point for a one-step unit, one point for
a multi-step unit — replacing the destination hex's cost. So the same drawn line is
simultaneously a link between two centres and a modifier attached to the hexside it crosses.
A renderer only needs to draw it once; a data model needs both views of it.

*Address:* an ordered pair of hexes for one segment; an ordered hex list for a line. The
hexside it crosses is derivable from the pair.

### 2.10 Edge-centred directional glyph

**A glyph centred on a hexside, oriented across it.** This map uses the kind heavily:

- **Strait, connected** — a white double-headed arrow at the hexside midpoint, its axis
  perpendicular to the hexside.
- **Strait, not connected** — a jagged or lightning-form double-headed arrow, same placement.
- **Beachhead hexside** — not printed on the map; created during play by placing a Beachhead
  marker in a sea hex with its arrow pointing at one hexside. So this is the same element
  kind, but generated at runtime rather than authored.

The connected-versus-unconnected distinction is carried entirely by the glyph shape, and the
two have different movement costs and different supply behaviour.

*Address:* one edge; the orientation follows from the edge. For the runtime beachhead case:
one hex plus one direction.

### 2.11 Region tint

A pale colour wash over a set of hexes. This map has more region layers than either other
sheet, and they **overlap**:

- **Country** — each country has its own tint.
- **Dependent** — a sub-territory of a country, tinted as a variant.
- **Region** — a sub-area of a country that can be ceded separately, tinted again.
- **Weather Area** — six of them, defined as lists of countries; shown not by a tint on the
  map but by a small key diagram beside the turn track.
- **Naval Zone** — bounded by its dashed border hexsides, containing sea hexes *and* coastal
  land hexes, and some coastal hexes belong to two zones at once.

Because a coastal land hex can be in a country, a dependent, a region, a weather area and one
or two naval zones simultaneously, these are **five independent labellings**, not a partition.
A renderer that assumes one region colour per hex will not cope.

*Address:* a set of hexes, drawn as a tint, an outline around its boundary edges, or both.

### 2.12 Text

Six distinct text kinds:

| Kind | Example | Placement |
|---|---|---|
| Hex identifier | `5205` | small, rotated 90°, set just inside one hexside |
| Place name | `Sapporo`, `Hakodate` | black serif, offset beside the symbol, horizontal |
| Major place name | `BANGKOK` | caps, larger |
| Area name with owner | *KARAFUTO (Japan)* | grey italic serif, two lines, spanning several hexes |
| Sea and feature name | *Kurile Is.*, *South China Sea* | grey or white italic, often at an angle or curved |
| Panel text | *On Station*, *Convoys* | inside a rotated panel, following the panel's angle |

The **area name with a parenthetical owner** is specific to this map and reflects the
political model: a territory is labelled with both its own name and whose it currently is.

### 2.13 Off-map furniture

Panels, tracks, boxes and tables: the delay box and naval warfare delay box, the strategic
warfare box, the government holding box, the Europe/Africa, Western US, Panama Canal, French
Polynesia and Eastern Europe off-map boxes, the USCL track, the posture display, the war state
display, the turn track, the VP track, the ceded lands box, the weather areas key, the Terrain
Effects Chart and the Terrain Key.

Structurally: **panels**, **tracks** (ordered cells), **boxes** (drop targets), **tables**
(grids of text), and **displays** (a grid of named boxes, as in the posture display).

Two features set this map's furniture apart. First, several off-map boxes are **partly
spatial**: an off-map box counts as part of a naval zone if a unit can reach it, and as
adjacent to another off-map box if direct transfer is allowed, both stated in the box's own
printed text. Second, the **naval zone boxes are printed over sea hexes at arbitrary
rotations**, and the hexes beneath them remain in play.

*Address:* not hex-based. Named nodes with a rectangle and, for the naval zone boxes, a
rotation angle.

---

## 3. Elements Specific to This Map

Things this sheet does that the Tarawa and Russian Campaign maps do not.

### 3.1 A printed chart that is itself split hex versus hexside

The Terrain Effects Chart has two sections with the same two columns each — **Hex Terrain
Type** with an MP cost and a CRT column shift, and **Hexside Terrain Type** with an MP cost and
a CRT column shift. Nine hexside types are listed. This is the clearest external confirmation
in the whole sample that a map model needs edges as first-class objects.

### 3.2 Panels rotated to arbitrary angles over live map area

Every naval zone has a printed box holding three sub-boxes, and the boxes are set at whatever
angle fits the shape of the sea. The renderer needs panels with a rotation, drawn above the map
layer, while the game model ignores them entirely.

### 3.3 Overlapping region layers

Five independent labellings over the same hexes, several of them mutable during play as
countries are conquered, liberated and reactivated. No other sheet examined has more than two,
and both of those are static.

### 3.4 Two sheets

The playing area spans two independently numbered sheets that are geographically continuous.
A renderer must be able to emit one sheet, the other, or both side by side, and the coordinate
model needs sheet-crossing adjacency that does not follow from the numbering.

### 3.5 Decorative cartography

Coastlines are drawn as fine irregular outlines; small offshore islets are drawn inside sea
hexes purely for recognition; inland rivers such as the Mekong are drawn as blue lines running
*through* land hexes, quite separately from the river hexsides that the rules read.

**Proposed simplification:** replace all of it with per-hex terrain fills and the authored
hexside features. The drawn inland rivers in particular are pure decoration and should not be
confused with river hexsides — this map has both, and only the hexside version is read by any
rule.

---

## 4. Anchoring and Coordinate Notes

Every element on this sheet reduces to one of eleven address kinds.

| # | Address kind | Elements on this map |
|---|---|---|
| 1 | Hex, as an area | terrain fill, grid stroke |
| 2 | Hex, as a closed path | strategic hex ring |
| 3 | Hex centre | city, capital, town, oil, aid, US impact, limited stacking, ice |
| 4 | Hex + offset or slot | port anchor, and the several glyphs that share a hex |
| 5 | Single edge | mountain, river, lake, all-sea, naval zone border, country border, region border |
| 6 | **Ordered edge list** | all of the above, as authored paths |
| 7 | Ordered hex pair | one road or rail segment |
| 8 | **Ordered hex list** | a road or rail line |
| 9 | **Edge midpoint, oriented across the edge** | strait arrows, connected and unconnected |
| 10 | **Hex + direction** | beachhead hexside, created at runtime |
| 11 | Set of hexes | countries, dependents, regions, weather areas, naval zones |

Kinds 6, 8, 9 and 10 are the ones a hex-only coordinate system cannot express directly.

**Vertices** are used implicitly throughout: every border, river and naval zone boundary is a
vertex path. No rule reads a vertex.

**Directions** are needed as values for the strait arrows' orientation, which follows from the
edge, and for the beachhead marker's facing, which does not — the beachhead is placed in a sea
hex and its arrow is chosen by the player, so it is a genuine (hex, direction) datum created
during play.

**The hex identifier** is a sheet prefix plus four digits: `w5422`, `e4904`, `w3218`. Which half
of the four digits indexes north–south is not established here. Named ranges of hexes appear in
the rules as inclusive spans along a sheet edge, such as `w5311` to `w6011`.

---

## 5. Rendering Implications

### 5.1 What the C++ side needs

- A **terrain value** per hex — clear, rough, all-sea, ice — kept separate from a **terrain
  appearance** — normal, desert, hills, forest, swamp — since the mapping is many to one.
- A **hex decoration list** holding *several* glyphs per hex, each with a named slot, plus a
  slot layout that keeps them from colliding. This map needs the multi-glyph case; the other
  two do not.
- An **edge feature table**: zero or more strokes per edge, each with a style id and a
  reference to the named path it belongs to.
- A **named path object** for authored edge chains, with a class — river, mountain, lake,
  naval zone border, country border, region border — so the border family can be styled by
  parameter.
- A **link network object** for roads and rails, with a per-segment kind, and a derived view
  that reports the hexside each segment crosses.
- An **edge glyph object** for the strait arrows: an edge, an icon id distinguishing connected
  from unconnected.
- A **runtime edge glyph**: a hex plus a direction, for beachhead markers placed during play.
- A **region object** supporting **overlapping membership**: a hex may belong to a country, a
  dependent, a region, a weather area, and one or two naval zones at once. Region membership
  is mutable for the political layers.
- A **label object** with placement modes: centred, offset, edge-anchored-and-rotated,
  along-path, and two-line-with-owner.
- **Panels with a rotation**, drawn above the map layer, plus tracks, boxes, tables and
  displays. Off-map boxes additionally need to participate in naval zone membership and in
  box-to-box adjacency, so they are not purely presentational.

### 5.2 What the SVG side needs

- Flat fills for terrain, keyed on terrain appearance rather than terrain value.
- `<symbol>` definitions for the reusable icons — city circle, capital star, town square, port
  anchor, multi-zone port, key port, limited stacking pips, oil derrick, aid marker, US impact
  star, ice floe, strait arrow in two forms. Instanced with `<use>` and a translate-then-rotate
  transform.
- `<path>` for hexes, strategic hex rings, edge chains and link lines. Rail hatching is best
  drawn as two stacked strokes rather than as a marker pattern.
- `<text>` with a rotation transform for hex identifiers, and `<textPath>` for the curved sea
  and area names.
- A `<g>` per layer in the order given in §1, plus a `<g>` per region layer so that country,
  dependent, region, weather and naval-zone tints can be toggled independently. Given that five
  labellings overlap, showing all of them at once is unreadable; the renderer should expect to
  show one at a time.
- A rotated `<g>` per naval zone box, drawn above the map layer.
- A `class` attribute on every element carrying its logical kind, and a data attribute for
  mutable political state — country owner, strategic hex faction, rail ownership — so a live
  view can recolour without regenerating geometry.

### 5.3 The awkward cases

**Several glyphs in one hex.** A coastal capital with a port, a limited stacking symbol and a
strategic hex ring needs four things drawn in one hexagon without collision. This is a small
layout problem the library should solve once, with named slots and a collision-aware fallback,
rather than leaving it to each caller.

**Overlapping region tints.** Five labellings cannot all be tinted at once. Either show one at a
time, or show boundaries rather than fills for the secondary layers.

**Naval zones that contain both sea and land.** A naval zone is not a contiguous sea area — it
includes coastal land hexes, and some of those belong to two zones. A region object that assumes
disjoint membership will get this wrong.

**Rotated panels over live hexes.** The panel must be drawn above the hexes and must be
completely invisible to the game model. Anything that computes "what is at this hex" must not
see it.

**Two sheets.** Adjacency across the sheet boundary does not follow from either sheet's numbering
and has to be authored.

Copyright Ben Paul Wise. All Rights Reserved.
