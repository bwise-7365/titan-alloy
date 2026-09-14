Copyright Ben Paul Wise. All Rights Reserved.

# The Russian Campaign — Map Graphics Inventory

**Source.** `C:\Library\War-Games\The Russian Campaign\TRC map v1 adjusted.png`
(1224 × 1483), the Deluxe Fifth Edition map, Consim Press / GMT 2022. Terrain and
symbol names are taken from the Terrain Effects Chart printed at the bottom left of
the sheet.

This document catalogues the **graphical elements** on the sheet and the address
each one needs, as input to a C++ map-object library and an SVG/HTML renderer. It
is not a rules summary. Nothing here assumes any particular drawing algorithm; the
question is only *what kinds of things must be drawable and addressable*.

Fidelity is deliberately limited. Where the printed art carries cartographic detail
that no rule reads — the drawn coastline, the mottling inside mountain hexes, the
soft glow along the shore — this document says so and proposes the simplification
rather than the reproduction.

---

## 1. Overview of the Sheet

One portrait sheet. The playable hex field fills the middle; off-map furniture rings
it — the title block, replacement and surrendered-unit pools and the weather chart
across the top, the turn record track as a single long strip below them, and the
Terrain Effects Chart and Combat Results Table in the bottom-left corner. Two further
panels are printed *inside* the hex field, over sea hexes: the Baltic movement box and
the airpower availability chart in the north, the Black Sea movement box in the south.

The hex grid is **flat-topped** in printed orientation and continuous across land and
water. Sea hexes are as much part of the grid as land hexes: they are drawn, numbered,
and used for measuring air range, even though ground units may not enter them.

The artwork separates into layers, drawn back to front:

1. Base geography — land and sea fills, the drawn coastline, terrain mottling.
2. Region tints — military district shading.
3. Hex grid strokes.
4. Hexside features — rivers, country borders, district borders, blocked hexsides.
5. Centre-to-centre links — the rail network.
6. Hex-perimeter rings — city hex outlines.
7. Hex-anchored glyphs — city squares and circles, oil derricks, port anchors,
   sea range dots.
8. Edge-centred directional glyphs — the Kerch Strait arrow.
9. Text — hex identifiers, place names, area names, feature names.

---

## 2. Standard Elements

These are the graphical kinds that also appear on the other two maps examined, and
that a shared library should provide. Each is given with the **address** it needs.

### 2.1 Hex area fill

A solid or mottled fill covering one whole hex.

| Terrain | Rendering on this map | Proposed simplification |
|---|---|---|
| Clear | pale yellow-tan | solid fill |
| Woods | mid green, irregular patches | solid fill |
| Mountain | mottled olive-brown blob | solid fill, or one texture |
| Swamp | pale green-grey | solid fill |
| Sea | mid to dark blue | solid fill |
| Lake | same blue as sea | solid fill |

The printed woods and mountains are irregular shapes that spill across hex boundaries,
so a hex is often part woods and part clear. The rules read one terrain per hex.

*Address:* one hex.

### 2.2 Hex grid stroke

The hexagon boundary, drawn as a thin line. Over land it is a warm grey; over sea it is
a lighter blue-grey. As on the other two maps, the grid stroke colour varies with the
underlying terrain rather than being constant.

*Address:* one hex, drawn as a closed path.

### 2.3 Hex perimeter ring

A **thick pale cream stroke following the whole hex boundary**, marking a city hex. Every
named city — major and minor alike — carries one, which makes the ring the visual cue for
"this hex matters" and the centred icon the cue for *how* it matters.

*Address:* one hex, plus a colour and an inset.

### 2.4 Hex-centred glyph

An icon drawn at the hex centre:

- **Major city** — a grey filled square with a lighter border.
- **Minor city** — a white filled circle.
- **Oil field** — a small derrick silhouette.
- **Sea range dot** — a small white dot at the centre of a sea hex, printed to help count
  air range over water. Not every sea hex has one, and the rules say the absence of a dot
  does not stop you measuring range through that hex, so the dot is a **drawing aid with
  no data behind it**.

*Address:* one hex.

### 2.5 Hex-anchored glyph at an offset

An icon that belongs to a hex but is deliberately placed away from the centre:

- **Port anchor** — a small circular badge with an anchor, placed at the coastal edge of
  the city hex, on the water side, so it reads as a harbour rather than as a second
  town. Its offset direction varies from city to city and follows the drawn coastline.

This is a case where the printed offset is a cartographic judgement, not a rule. A
library can either store an explicit offset per port or place the anchor in a fixed slot
and accept the loss.

*Address:* one hex, plus an offset or a named slot.

### 2.6 Hex-anchored glyph facing one of its edges

Not used on this map. The Russian Campaign has nothing equivalent to Tarawa's fire dots.

### 2.7 Edge stroke

A stroke drawn **along a single hexside**, belonging to the boundary rather than to
either hex:

- **River** — a blue line of moderate weight.
- **Country border** — a heavy solid black line.
- **Russian military district boundary** — a dash-dot line.
- **Blocked hexside** — a chain of white dots. The rules describe it as a white dashed
  line; on this printing it reads as dots.
- **Coastline (as a rule object)** — the black coastal line that ground units may not
  cross. This is distinct from the drawn cartographic shoreline, and it follows hexsides.

*Address:* one edge.

### 2.8 Edge chain (polyline along hexsides)

The same as above but authored as a path running through several hexsides in sequence,
turning at vertices. Every one of the edge strokes above is authored this way: rivers run
for dozens of hexsides, country borders enclose whole nations, district boundaries
enclose regions.

Rivers additionally need an **identity**. The combat rule doubles a defender only when the
attackers are all on river hexes and the defender is not on a river hex *of the same
river*, so the Dnieper and the Don are different objects even where they nearly touch. A
river is therefore a named path, not just a set of decorated edges.

*Address:* an ordered list of edges, plus a name for rivers.

### 2.9 Centre-to-centre link

A stroke from one hex centre to another. On this map there is one kind, and it is the most
important element on the sheet after terrain:

- **Railroad** — a dashed dark line with short cross-ticks, in the conventional railway
  hatching. It runs hex centre to hex centre, branches at cities, and forms a connected
  network across the whole map with off-board terminals at the west, east and south edges.

Rails are authored as chains, not as isolated segments, and several chains meet at a city
or at a rail junction hex.

*Address:* an ordered pair of hexes for one segment; an ordered hex list for a line.

### 2.10 Edge-centred directional glyph

**A glyph centred on a hexside, oriented across it.** On this map there is exactly one:

- **Kerch Strait** — a white double-headed arrow drawn at the midpoint of the hexside
  between two named hexes, its axis perpendicular to that hexside, with an italic label
  beside it.

One instance, but a genuine element kind — and Dai Senso uses the same idea a dozen times
over for its straits. The address is an edge plus the implied cross-edge orientation.

*Address:* one edge; the orientation follows from the edge.

### 2.11 Region tint

A pale colour wash over a set of hexes, marking an area. On this map the Russian military
districts — Leningrad, Baltic, Western, Kiev, Odessa — are shown by a combination of a
dash-dot boundary and a faint tint, with a small italic label inside.

*Address:* a set of hexes, drawn either as a tint over the set or as an outline around its
boundary edges, or both.

### 2.12 Text

Five distinct text kinds, each with its own placement rule:

| Kind | Example | Placement |
|---|---|---|
| Hex identifier | `KK19` | small, rotated 90°, set just inside one hexside, consistently placed |
| Major city name | `LENINGRAD` | white or black caps, offset beside the city hex, horizontal |
| Minor city name | `Tallinn` | mixed case, smaller, offset beside the hex |
| Country / sea name | `FINLAND`, `Baltic Sea` | large, letter-spaced, often outlined, spanning many hexes, sometimes curved |
| Feature label | *Narva R.*, *Kerch Strait* | small italic, set beside or along the feature |

The hex identifier is set **rotated** and offset from a consistent hexside — a per-hex
label with a fixed edge anchor and a rotation. All three maps examined number their hexes
this way.

Country names are the awkward case: `FINLAND` is set in outlined caps, widely letter-spaced,
at an angle, across a dozen hexes. That needs text-along-a-path or a rotated text run with
explicit letter spacing.

### 2.13 Off-map furniture

Rectangular panels holding tracks, tables and pools: the title block, the Axis and Russian
replacement pools, the Axis and Russian surrendered-unit boxes, the weather chart, the
Russian paratroop reserve, the off-map units box, the Stukas and Sturmoviks boxes, the
turn record track, the airpower availability chart, the two sea movement boxes, the Terrain
Effects Chart, and the Combat Results Table.

Structurally these are: **panels** (a titled rectangle), **tracks** (an ordered sequence of
numbered cells, each a drop target and, on this map, also a numeric register), **boxes**
(untitled drop targets), and **tables** (a grid of text cells).

Two of these panels are printed **over sea hexes inside the playable field**, which means
the renderer needs panels to be able to sit above the map layer at arbitrary positions,
not only in a margin.

*Address:* not hex-based. Named nodes with a rectangle.

---

## 3. Elements Specific to This Map

Things this sheet does that the Tarawa and Dai Senso maps do not.

### 3.1 The rail network as the dominant line layer

No other sheet examined carries a comparable centre-to-centre network. The rail lines run
everywhere, branch at cities and junctions, and carry the game's movement and supply. They
are also the only element whose **ownership changes during play**: railhead markers are
placed on the network and advance and retreat, so a renderer that supports live play needs
the rail network to be recolourable per segment.

That is worth calling out as a requirement: the link layer needs per-segment state, not
just per-line style.

### 3.2 Sea hexes that are drawn but not enterable

Sea and lake hexes are fully part of the grid — numbered, gridded, dotted for range
counting — but ground units may not enter them. Tarawa's water hexes, by contrast, are
entered constantly. So "water" is not a single concept across the three maps, and the
library should not conflate "not part of the grid" with "not enterable".

### 3.3 The turn track used as a numeric register

The turn record track is a single long strip of numbered cells. Besides holding the turn
marker, it holds the cumulative weather modifier, the Russian worker replacement points,
the invasion counts and the scenario-end marker — each stored by *placing a marker on a
numbered cell*, where the cell number is the value.

Graphically this is one element kind — an ordered strip of cells — but it needs to support
several independent markers at once, and to report a cell's number as a value.

### 3.4 Decorative cartography

The coastline is drawn as an irregular curve with a soft cyan glow behind it; mountains are
mottled blobs; woods are irregular green patches; small offshore islets are drawn inside sea
hexes purely for recognition.

**Proposed simplification:** replace all of it with per-hex terrain fills. The coastline
becomes the boundary between sea-filled and land-filled hexes. The one thing that must
survive is the **black coastal line as a rule object** — the hexside chain that ground units
may not cross — which is a different thing from the drawn shoreline and does not follow it
exactly.

That distinction is easy to miss and worth stating plainly: this map has a *drawn* coast
and a *rule* coast, and only the second one matters.

---

## 4. Anchoring and Coordinate Notes

Every element on this sheet reduces to one of ten address kinds.

| # | Address kind | Elements on this map |
|---|---|---|
| 1 | Hex, as an area | terrain fill, grid stroke |
| 2 | Hex, as a closed path | city hex ring |
| 3 | Hex centre | city square, city circle, oil derrick, sea range dot |
| 4 | Hex + offset or slot | port anchor |
| 5 | Single edge | river segment, border segment, blocked hexside, coastal line |
| 6 | **Ordered edge list** | rivers (named), country borders, district borders, coastline |
| 7 | Ordered hex pair | one rail segment |
| 8 | **Ordered hex list** | a rail line |
| 9 | **Edge midpoint, oriented across the edge** | Kerch Strait arrow |
| 10 | Set of hexes | military district tint and outline |

Kinds 6, 8 and 9 are the ones a hex-only coordinate system cannot express directly.

**Vertices** are used implicitly and heavily: rivers, borders and district boundaries are all
vertex paths, and where a river passes through a hex corner rather than cleanly along a
hexside the drawn line needs the vertex to sit on. No rule reads a vertex.

**Directions** are needed for the Kerch Strait arrow's orientation, which follows from the
edge rather than being independently authored.

**The hex identifier** is a letter and a number: `F18`, `S11`, `AA29`, `KK19`, `QQ5`. Letters
past `Z` are doubled, so the letter component is a string ordered by length then
alphabetically. The rules use the letter to name a hex row in a north–south sense — Finnish
units may not move south of the `H` hex row, Balkan units not north of the `L` row, and
Russian reinforcements enter from the south edge at `QQ5`–`QQ16` — so `A` is the northern end
and `QQ` the southern. Which end of the numeric range is west is not established here; the
printed numerals are too small to read at the available scan resolution.

---

## 5. Rendering Implications

### 5.1 What the C++ side needs

- A **terrain enumeration** with one value per hex: clear, woods, mountain, swamp, sea,
  lake. Six values replace all the decorative cartography.
- A **hex decoration list**: zero or more glyphs per hex, each with a slot and a payload.
  City class, oil field and port all live here.
- An **edge feature table**: zero or more strokes per edge, each with a style id and, for
  rivers, a reference to the named path it belongs to.
- A **named path object** for authored edge chains, holding an ordered edge list, a name
  where one exists, and a style. Rivers need the name; borders and coastlines do not.
- A **link network object** for the rail system: nodes at hex centres, edges between them,
  a style, and **per-segment mutable state** for railhead ownership.
- An **edge glyph object** for the strait arrow: an edge, an icon id, and a label.
- A **region object**: a hex set, a tint, an optional boundary style, and a label.
- A **label object** with a placement mode: centred, offset, edge-anchored-and-rotated, or
  along-path with letter spacing.
- **Panels, tracks and boxes** as non-hex named nodes, with panels able to sit over the map
  field as well as beside it, and tracks able to hold several markers and report a cell's
  numeric value.

### 5.2 What the SVG side needs

- Flat fills for terrain; optional `<pattern>` for mountain mottling if any texture is wanted.
- `<symbol>` definitions for the reusable icons: major city square, minor city circle, oil
  derrick, port anchor, sea range dot, strait double-arrow. Instanced with `<use>` and a
  translate-then-rotate transform.
- `<path>` for hexes, city rings, edge chains and rail lines. The railway hatching is a
  dashed stroke drawn over a solid one, or a `stroke-dasharray` with a second stroked path
  for the ticks — two stacked paths is simpler and more robust than trying to build a
  marker pattern.
- `<text>` with a rotation transform for hex identifiers and `<textPath>` for the curved
  country and sea names, with `letter-spacing` set explicitly.
- A `<g>` per layer in the order given in §1, so z-order is explicit and layers can be
  toggled.
- A `class` attribute on every element carrying its logical kind, so a stylesheet can
  restyle the sheet. Because the rail network changes ownership during play, rail segments
  should also carry a class or data attribute for their current owner, so a live view can
  recolour them without regenerating the geometry.

### 5.3 The awkward cases

**Two coastlines.** The drawn shoreline and the rule-bearing black coastal line are different
objects that nearly coincide. If the map is regenerated from hex terrain, the drawn shoreline
disappears and only the rule coastline remains — which is the right outcome, but it means the
rule coastline must be authored as its own edge chain and not derived from the terrain fills.

**Rail ticks at junctions.** Where three or four rail lines meet in one hex, the cross-ticks
of the railway hatching collide near the centre. The printed map handles this by thinning the
ticks near junctions. A renderer either does the same or accepts the clutter.

**Doubled column letters.** `AA` sorts after `Z`, not between `A` and `B`. Any code that
turns a printed identifier into a coordinate, or that iterates hexes in printed order, has to
encode length-then-lexicographic ordering.

**Panels over the map.** Two furniture panels sit on top of sea hexes inside the playable
field. The hexes beneath them are still in play. The renderer must draw the panel above the
map layer and the game logic must ignore the panel entirely — so panels cannot be part of the
map's spatial model.

Copyright Ben Paul Wise. All Rights Reserved.
