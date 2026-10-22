Copyright Ben Paul Wise. All Rights Reserved.

# D-Day at Tarawa — Map Graphics Inventory

**Source.** `C:\Library\War-Games\D-Day at Tarawa wargame\DDaT-map2.png`
(1786 × 1153), the extended-game version of the Delphine Echassoux 2014 map
redesign. `DDaT-map1.png` is the same artwork with a different sequence-of-play
panel. Terrain and symbol names are taken from the two legend blocks printed on
the sheet.

This document catalogues the **graphical elements** on the sheet and the address
each one needs, as input to a C++ map-object library and an SVG/HTML renderer. It
is not a rules summary. Nothing here assumes any particular drawing algorithm; the
question is only *what kinds of things must be drawable and addressable*.

Fidelity is deliberately limited. Where the printed art carries decorative detail
that no rule reads — the wandering outlines of crater fields, the drawn shoreline,
individual palm sprigs — this document says so and proposes the simplification
rather than the reproduction.

---

## 1. Overview of the Sheet

One landscape sheet. The playable hex field occupies the lower two-thirds and the
right four-fifths; the remainder is off-map furniture — the sequence-of-play strip
along the top, the turn track top-right, holding boxes for Japanese eliminated
units, depth markers and reserves, and two legend blocks bottom-left.

The hex grid is **flat-topped** in printed orientation, continuous across land and
water, and runs off all four edges of the playable area with no border decoration.

The artwork separates cleanly into layers, drawn back to front:

1. Base geography — sand, water, vegetation, the airstrip, crater fields, buildings.
2. Hex grid strokes.
3. Hexside features — seawall, pier, coral reef, water fire zone boundaries.
4. Hex-perimeter rings — the coloured position outlines.
5. Centre-to-centre links — position connectors.
6. Hex-anchored glyphs — position badges, fire dots, artillery markers, LVT wrecks,
   arrival boxes.
7. Text — hex identifiers, position labels, feature names.

That layer order matters: fire dots sit on top of hex grid strokes and under nothing,
and the position ring is drawn just inside the hex boundary so that a neighbouring
hex's fire dot can sit just outside it without collision.

---

## 2. Standard Elements

These are the graphical kinds that also appear on the other two maps examined, and
that a shared library should provide. Each is given with the **address** it needs.

### 2.1 Hex area fill

A solid or textured fill covering one whole hex.

| Terrain | Rendering on this map | Proposed simplification |
|---|---|---|
| Beach | pale warm sand | solid fill |
| Clear | slightly paler sand | solid fill |
| Airstrip | white | solid fill |
| Water | light blue | solid fill |
| Coral reef | mottled pale green over blue | solid fill, or one texture |
| Palm trees | scattered green sprig glyphs over sand | one repeating texture |
| Rough / crater | tan lobed blobs over sand | solid fill, or one texture |

*Address:* one hex.

**Note on fidelity.** The printed airstrip is a white polygon with straight edges cut
at an angle across the hex grid, so a hex is often part airstrip and part sand. The
crater fields are likewise free-form. Both are terrain the rules read per hex, so
both should become whole-hex fills. This is the same simplification Ben proposed for
the small lake-like curves: one discrete terrain value per hex, no sub-hex geometry.

### 2.2 Hex grid stroke

The hexagon boundary itself, drawn as a thin neutral line. On this map it is a
mid-grey over land and a slightly lighter blue-grey over water, so the grid stroke
colour is a function of the underlying terrain rather than a constant.

*Address:* one hex, or equivalently the full edge set. Practically it is drawn once
per hex as a closed path.

### 2.3 Hex perimeter ring

A **thick coloured stroke following the whole hex boundary**, drawn slightly inset so
that it reads as belonging to the hex rather than to the grid. On this map every
Japanese position hex carries one, in the position's colour: purple, red, olive,
green, blue or orange.

This is worth treating as its own element kind rather than as six edge strokes. It is
authored as a property of the hex, it is a single closed path, and it needs an inset
distance so it does not collide with the neighbouring hex's ring.

*Address:* one hex, plus a colour and an inset.

### 2.4 Hex-centred glyph

An icon or badge drawn at the hex centre. On this map:

- **Position badge** — a small filled hexagon in the position colour carrying the
  position ID as text (`B9`, `E13`, `C1`). Drawn at the centre, at the same rotation
  as the map hexes.
- **Potential LVT wreck** — a teal diamond outline, centred, in a water hex.

*Address:* one hex, plus an optional rotation.

### 2.5 Hex-anchored glyph at an offset

An icon that belongs to a hex but is deliberately placed away from the centre so it
does not collide with the centred badge. On this map:

- **Artillery marker** — a black letter (`L`, `M` or `H`) beside a small red rising-sun
  flag, placed below and right of the position badge.
- **Coastal / inland setup code** — a small `C` or `I` glyph below the badge.
- **Armour setup symbol** — a small tank silhouette.

The offsets are consistent across the sheet, which suggests a small set of named
**anchor slots** within a hex (centre, below-centre, lower-right) rather than free
coordinates.

*Address:* one hex, plus a named slot.

### 2.6 Hex-anchored glyph facing one of its edges

**This is the element that most needs the ABC/QRS coordinate system**, and Tarawa uses
it more heavily than either other map.

A **fire dot** is a half-disc whose flat side lies on a hexside and whose bulge points
into the hex. It is drawn in the projecting position's colour, and it is placed on the
hexside of the receiving hex that faces the position projecting it — the rulebook says
so explicitly. A single hex can carry up to six of them, in different colours, one per
hexside.

Two printed variants:

- **Intense fire dot** — solid filled half-disc.
- **Steady fire dot** — half-disc with a white notch cut into it.

So the element is: *a glyph belonging to hex H, positioned at the midpoint of the edge
between H and a named neighbour, rotated so it faces into H.* Its address is a
(hex, direction) pair — which is exactly a hex plus one of the six QRS basis
directions, or equivalently an edge plus a side.

*Address:* one hex plus one of its six edges, with an implied facing.

### 2.7 Edge stroke

A stroke drawn **along a single hexside**, belonging to the boundary rather than to
either hex. On this map:

- **Seawall hexside** — a heavy dark band with a chain-link motif.
- **Pier hexside** — a dark diagonal bar.
- **Coral reef hexside** — a ragged green scalloped line.

*Address:* one edge.

### 2.8 Edge chain (polyline along hexsides)

The same as above but authored as a path running through several hexsides in sequence,
turning at vertices. On this map:

- **Seawall** runs as a long chain along the beach.
- **The Pier** is a heavy grey band running from the shoreline out into the lagoon along
  a chain of hexsides, with running text set along it.
- **Water fire zone boundary lines** are thick coloured polylines, one per position
  colour, dividing the lagoon into fire arcs. Several colours can run in parallel along
  the same chain of hexsides, offset slightly so both remain visible.

The parallel-offset case is worth noting: two boundary lines sharing a hexside are
drawn side by side, not on top of each other. A renderer needs a per-chain lateral
offset, or it will lose one of them.

*Address:* an ordered list of edges, which is equivalently a vertex path.

### 2.9 Centre-to-centre link

A stroke from one hex centre to another. On this map there is exactly one kind:

- **Fire position connector** — a thick dashed line in the position colour, joining the
  hexes of a position group. Adjacent hexes only, as far as the printed map shows.

*Address:* an ordered pair of hexes.

### 2.10 Directional glyph in a hex

An arrow indicating a facing. On this map:

- **USMC arrival box** — a teal rotated square in each beach approach hex, with an arrow
  attached pointing at one of the six hex directions. This is the printed facing that
  landing units adopt.

Some beach approach hexes carry two arrows, a primary and a secondary, which the drift
rules distinguish.

*Address:* one hex plus a direction. Where two arrows exist, two directions with a
primary/secondary role.

### 2.11 Text

Four distinct text kinds, each with its own placement rule:

| Kind | Example | Placement |
|---|---|---|
| Hex identifier | `1538` | small, rotated 90°, set just inside one hexside, same side for every hex |
| Position label | `E13` | inside the position badge, horizontal |
| Feature label | *The Pier* | set along a path, following the pier band |
| Box label | `R3C`, `Turn 5` | horizontal, at a hex-anchored slot |

The hex identifier deserves special mention. It is set **rotated** and offset from a
consistent hexside — a per-hex label with a fixed edge anchor and a rotation. Any
renderer needs that as a first-class placement, because it is how all three maps
examined number their hexes.

### 2.12 Off-map furniture

Rectangular panels holding tracks, tables and holding boxes: the sequence-of-play
strip, the turn track (a grid of numbered cells), the Japanese eliminated units box,
two depth marker boxes, the reserve box, the command post range track (an ordered
strip of numbered cells), the available LVT box, the US infantry loss box, and two
legend blocks.

Structurally these are: **panels** (a titled rectangle), **tracks** (an ordered
sequence of cells, each a drop target), **boxes** (an untitled drop target), and
**tables** (a grid of text cells).

*Address:* not hex-based. Named nodes with a rectangle.

---

## 3. Elements Specific to This Map

Things this sheet does that the Russian Campaign and Dai Senso maps do not.

### 3.1 Colour as an index, not decoration

The six position colours are the game's activation key: a drawn fire card names three
colours, and every position of those colours acts. Colour here is **data**, not styling.
The same colour is carried by the hex perimeter ring, the centre badge, every fire dot
the position projects, the position group connector, and the water fire zone boundary
line. A renderer must therefore drive all of those from one palette entry, and a
recolour for accessibility or for a dark theme has to keep the six hues mutually
distinguishable.

### 3.2 Fire dots as a dense per-edge layer

No other map examined carries a comparable density of per-edge, per-hex, directional
glyphs. Most land hexes on Betio carry between one and four fire dots of different
colours around their perimeter. This is the single strongest argument on any of the
three sheets for being able to name (hex, edge) pairs directly.

### 3.3 Water fire dots use a different shape and a different anchor

Inside water hexes, fire is marked not by half-discs on hexsides but by **small filled
squares placed in a row near the hex centre**, one per projecting position colour. A
water hex can carry one, two or three. So the same underlying datum — "this hex is in
the field of fire of position colour X" — is drawn as an edge-anchored half-disc on
land and as a centre-anchored square in water.

For a library this means the *glyph* is a presentation choice keyed on terrain, while
the *datum* is uniform. Worth keeping those separable.

### 3.4 Sub-hex icons at arbitrary rotation

Buildings and fortified buildings are drawn as grey or black rectangles with internal
hatching, rotated to match the real street plan, and they frequently straddle hex
boundaries. The rules read them as a per-hex terrain type.

**Proposed simplification:** drop the rotated rectangles entirely and use two whole-hex
terrain fills, `building` and `fortified_building`. Nothing is lost mechanically.

### 3.5 Decorative geography

The shoreline is a drawn curve with a soft edge; the crater fields are free-form lobes;
the palm trees are individually placed sprigs; the airstrip is an angled white polygon.
None of these is hex-aligned and none is read by any rule beyond the hex terrain type it
implies.

**Proposed simplification:** replace all of it with per-hex terrain fills. The coastline
becomes the boundary between water-filled and land-filled hexes and needs no separate
element at all.

---

## 4. Anchoring and Coordinate Notes

Every element on this sheet reduces to one of nine address kinds.

| # | Address kind | Elements on this map |
|---|---|---|
| 1 | Hex, as an area | terrain fill, grid stroke |
| 2 | Hex, as a closed path | position perimeter ring |
| 3 | Hex centre | position badge, LVT wreck diamond |
| 4 | Hex + named slot | artillery marker, setup code, armour symbol |
| 5 | **Hex + one of its six edges** | fire dots (intense, steady) |
| 6 | Single edge | seawall segment, pier segment, coral reef segment |
| 7 | **Ordered edge list** | seawall run, the pier, water fire zone boundary lines |
| 8 | Ordered hex pair | fire position connector |
| 9 | **Hex + direction** | beach approach facing arrows |

Kinds 5, 7 and 9 are the ones a hex-only coordinate system cannot express directly.

**Vertices** are used implicitly: every edge chain is a path through vertices, and the
turning behaviour at a vertex — whether the stroke mitres or rounds — is a rendering
question the library will have to answer. No rule on this map reads a vertex.

**Directions** are needed as values, not merely as adjacency: the fire dot's facing, the
arrival box's arrow, and the drift rule that rotates a facing "to the next hexside to
the right" all require the six directions to be an ordered cyclic type with a rotation
operation.

**The hex identifier** is four digits, two per axis. The printed numerals run vertically
along the hex edges and are too small to read at the available scan resolution, so which
half indexes north–south is not asserted here; it should be settled against the physical
map. Position A14 is in hex `2336`.

---

## 5. Rendering Implications

### 5.1 What the C++ side needs

- A **terrain enumeration** with one value per hex, covering: beach, clear, airstrip,
  water, coral reef, palm trees, rough/crater, building, fortified building. Nine values
  replace all the decorative geography.
- A **hex decoration list**: zero or more glyphs per hex, each with a slot (centre, or one
  of a few named offsets) and a payload (icon id, optional text, optional colour).
- A **per-hex edge glyph list**: zero or more glyphs per (hex, edge) pair, each with an
  icon id and a colour. This is the fire dot layer.
- An **edge feature table**: zero or more strokes per edge, each with a style id. Needed
  separately from the chains, because a renderer may want to draw them per edge while an
  author writes them as a path.
- A **path object** for authored edge chains, holding an ordered edge list plus a style
  and a lateral offset index for the parallel case.
- A **link object** for centre-to-centre segments, holding a hex pair plus a style.
- A **label object** with a placement mode: centred, slotted, edge-anchored-and-rotated,
  or along-path.
- **Panels, tracks and boxes** as non-hex named nodes.

### 5.2 What the SVG side needs

- `<pattern>` definitions for the two textures worth keeping (palm trees, crater), if
  textures are wanted at all; otherwise flat fills throughout.
- `<symbol>` definitions for each reusable icon: position badge, intense fire dot, steady
  fire dot, water fire square, LVT wreck diamond, arrival box with arrow, artillery flag,
  pier head. Instanced with `<use>` plus a `transform` of translate-then-rotate.
- `<path>` for hexes, perimeter rings, edge chains and links. Dashed strokes via
  `stroke-dasharray` — the position connector and the seawall are both dashed, with very
  different patterns.
- `<text>` with a `transform` rotation for hex identifiers, and `<textPath>` for the
  labels that run along the pier.
- A `<g>` per layer, in the order given in §1, so that z-order is explicit and a viewer
  can toggle layers.
- A `class` attribute on every element carrying its logical kind and, where relevant, its
  position colour, so that a stylesheet can recolour the whole sheet. Given that colour is
  data on this map, driving the six position hues from CSS custom properties would let one
  SVG serve a light theme, a dark theme and a colour-blind-safe palette.

### 5.3 The awkward cases

**Parallel edge chains.** Two water fire zone boundary lines running along the same
hexside must be offset laterally. The renderer needs to know how many chains share each
edge and assign each an offset index. This is a small layout pass over the edge chains
before drawing.

**Ring inset versus neighbour's fire dot.** The position perimeter ring is inset and the
neighbouring hex's fire dot sits just outside the shared hexside. Both are near the same
line. The inset distances need to be parameters, not constants baked into the drawing
code.

**Half-disc orientation.** A fire dot must be rotated to face into its own hex. With QRS
directions available this is a table lookup from direction to angle; without them it means
computing an angle from two hex centres every time.

Copyright Ben Paul Wise. All Rights Reserved.
