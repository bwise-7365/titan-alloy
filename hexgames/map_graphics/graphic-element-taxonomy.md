Copyright Ben Paul Wise. All Rights Reserved.

# Map Graphic Element Taxonomy

A synthesis of `d-day-at-tarawa-graphics.md`, `the-russian-campaign-graphics.md`
and `dai-senso-graphics.md`, written to scope the drawable-object side of a C++
hex wargame library and its SVG/HTML output, resting on the `hexmap` ABC/QRS
coordinate system (`C:\repos\ghub-per\panj\hexmap`).

The three maps are use cases, not reproduction targets. The question this document
answers is: **what set of element kinds must the library be able to hold and draw
so that all three sheets could be rebuilt from data?** Not every printed nuance —
the maps carry a good deal of cartographic decoration that no rule reads, and the
recommendation throughout is to drop it in favour of per-hex terrain values.

Algorithms are out of scope. Where a graph is needed, this document names the graph
and assumes a suitable third-party C++ library will be chosen later.

---

## 1. The eleven address kinds

Every element on all three sheets reduces to one of eleven ways of saying *where*.
This is the core result.

| # | Address kind | `hexmap` expression | Tarawa | TRC | Dai Senso |
|---|---|---|:--:|:--:|:--:|
| 1 | Hex, as an area | one `CoordQRS` | ● | ● | ● |
| 2 | Hex, as a closed path | one `CoordQRS` + inset | ● | ● | ● |
| 3 | Hex centre point | one `CoordQRS` | ● | ● | ● |
| 4 | Hex + named slot | `CoordQRS` + slot id | ● | ● | ● |
| 5 | **Hex + one of its six edges** | `CoordQRS` + QRS direction | ● | — | — |
| 6 | Single edge | one `CoordABC` edge address | ● | ● | ● |
| 7 | **Ordered edge list (path)** | vertex path in `CoordABC` | ● | ● | ● |
| 8 | Ordered hex pair (link) | two `CoordQRS` | ● | ● | ● |
| 9 | **Ordered hex list (link chain)** | `CoordQRS` sequence | — | ● | ● |
| 10 | **Edge midpoint, oriented across it** | `CoordABC` edge + normal | — | ● | ● |
| 11 | Set of hexes (region) | `CoordQRS` set | — | ● | ● |

Kinds **5, 7, 9 and 10** are the ones an ordinary axial or cube hex coordinate system
cannot express directly, and they are exactly the ones the ABC/QRS system was built
for. Kind 5 appears only on Tarawa but appears there in the thousands. Kinds 7 and 10
appear on all three.

A twelfth address kind exists and is not hex-based at all: **named off-map node**,
for panels, tracks, boxes, tables and displays. All three sheets need it.

---

## 2. Element kinds, by frequency across the three sheets

`Y` present; `—` absent. Ordered roughly by how strongly the three maps argue for it.

| Element kind | Address | Tarawa | TRC | Dai Senso |
|---|:--:|:--:|:--:|:--:|
| Hex terrain fill | 1 | Y | Y | Y |
| Hex grid stroke | 1 | Y | Y | Y |
| Hex-centred glyph | 3 | Y | Y | Y |
| Hex-anchored glyph at a slot | 4 | Y | Y | Y |
| Hex perimeter ring | 2 | Y | Y | Y |
| Single-edge stroke | 6 | Y | Y | Y |
| Edge chain, authored as a path | 7 | Y | Y | Y |
| Centre-to-centre link | 8 | Y | Y | Y |
| Hex identifier label, rotated, edge-anchored | 3 | Y | Y | Y |
| Place / feature label at an offset | 3 or 4 | Y | Y | Y |
| Off-map panel, track, box, table | node | Y | Y | Y |
| Link chain (a named line through many centres) | 9 | — | Y | Y |
| Edge-centred directional glyph | 10 | — | Y | Y |
| Region, as a hex set with tint and/or outline | 11 | Y | Y | Y |
| Area label spanning many hexes, curved or angled | path | Y | Y | Y |
| **Hex + edge directional glyph** | 5 | Y | — | — |
| Hex + direction facing arrow | 5 | Y | — | Y |
| Parallel edge chains needing lateral offset | 7 | Y | — | — |
| Several glyphs coexisting in one hex | 4 | — | — | Y |
| Overlapping region layers | 11 | — | — | Y |
| Panel rotated over live map area | node | — | Y | Y |
| Per-segment mutable link state | 8, 9 | — | Y | Y |
| Two independently numbered sheets | — | — | — | Y |

Everything in the top block is unanimous and belongs in the core. Everything below
the horizontal break appears on one or two sheets and is where the design decisions
are.

---

## 3. Proposed C++ object model

Sketch only — names are placeholders and the point is the shape, not the syntax.

### 3.1 Addresses

The library already has `CoordQRS` for hex centres and centre-to-centre steps, and
`CoordABC` for hexes and vertices. Three thin address types complete the set:

```
struct HexAddr;                 // wraps CoordQRS
struct EdgeAddr;                // an undirected hexside; two adjacent HexAddr, or
                                //   an ABC vertex pair, whichever is canonical
struct SideAddr;                // HexAddr + Direction — "this hex's edge, facing in"
enum class Direction;           // six values, cyclic, with rotate(±n)
```

`SideAddr` and `EdgeAddr` are different types on purpose. An `EdgeAddr` is a boundary
shared by two hexes and owned by neither — a river, a border, a seawall. A `SideAddr`
belongs to one hex and faces outward or inward — a Tarawa fire dot, a beachhead's
arrow. Conflating them is the mistake this whole exercise is meant to prevent.

### 3.2 Map data tables

```
class HexLayer;                 // HexAddr -> terrain value, terrain appearance,
                                //   grid-stroke style, ring (optional), glyph list
class EdgeLayer;                // EdgeAddr -> zero or more edge features
class SideLayer;                // SideAddr -> zero or more directional glyphs
class PathSet;                  // named edge chains: rivers, borders, coastlines,
                                //   zone boundaries, seawall runs, the pier
class LinkNetwork;              // named centre-to-centre networks: rail, road,
                                //   position connectors; per-segment mutable state
class RegionSet;                // named hex sets, overlapping allowed, mutable
class LabelSet;                 // labels with placement modes
class FurnitureSet;             // panels, tracks, boxes, tables, displays
```

Seven tables and a furniture set. `SideLayer` exists for Tarawa alone but costs almost
nothing and is the clearest justification for the coordinate system.

### 3.3 Glyph placement within a hex

Dai Senso needs four things in one hexagon — a strategic ring, a capital, a port and a
limited-stacking symbol — without collision. Tarawa needs a centred badge plus a setup
code plus an artillery marker. The printed maps solve this with a small set of consistent
positions.

```
enum class Slot { Centre, Above, Below, LowerLeft, LowerRight, UpperLeft, UpperRight,
                  EdgeToward /* + Direction */ };
```

`EdgeToward` is the port-anchor case: place the glyph offset from the centre toward a
named edge. That is a `SideAddr` used as a placement hint rather than as data.

Recommendation: fixed named slots with a documented layout, and a collision check that
warns rather than silently overlapping. Free-form per-glyph offsets should be available
as an escape hatch but not the normal path.

### 3.4 Styles

```
struct StrokeStyle { colour, width, dash pattern, cap, join, opacity };
struct FillStyle   { colour or pattern id, opacity };
struct TextStyle   { family, size, weight, slant, case, colour, halo, letter-spacing };
struct GlyphStyle  { symbol id, scale, rotation, colour overrides };
```

Two observations from the sheets:

- **Grid stroke colour varies with the underlying terrain** on all three maps — grey over
  land, blue-grey over sea. So the grid stroke is not a constant and should be looked up
  per hex.
- **Colour is sometimes data, not styling.** Tarawa's six position colours are the game's
  activation key; Dai Senso's three strategic-hex colours are factions. Those hues must
  come from a named palette that the game logic also reads, and a recolour must preserve
  distinguishability rather than being a free aesthetic choice.

### 3.5 Layers and z-order

All three sheets stack the same way. A fixed layer order in the library, with each layer
individually toggleable, covers every case:

1. terrain fill
2. region tint
3. grid stroke
4. edge features
5. link networks
6. hex perimeter rings
7. hex glyphs
8. side glyphs and edge-centred glyphs
9. labels
10. overlaid panels

---

## 4. SVG / HTML output model

### 4.1 Document shape

```
<svg viewBox="...">
  <defs>
    <pattern id="terr-rough"> … </pattern>       <!-- only if textures are wanted -->
    <symbol id="gly-city-major"> … </symbol>
    <symbol id="gly-fire-intense"> … </symbol>
    <symbol id="gly-strait-connected"> … </symbol>
    …
  </defs>
  <g class="layer terrain">   <path class="hex terr-clear" d="…"/> … </g>
  <g class="layer regions">   … </g>
  <g class="layer grid">      … </g>
  <g class="layer edges">     <path class="edge river" d="…"/> … </g>
  <g class="layer links">     <path class="link rail" data-owner="axis" d="…"/> … </g>
  <g class="layer rings">     … </g>
  <g class="layer hexglyphs"> <use href="#gly-city-major" transform="translate(…)"/> … </g>
  <g class="layer sideglyphs"><use href="#gly-fire-intense" transform="translate(…) rotate(…)"/> … </g>
  <g class="layer labels">    … </g>
  <g class="layer panels">    <g transform="rotate(…)"> … </g> </g>
</svg>
```

### 4.2 The mapping, element kind to SVG

| Element kind | SVG |
|---|---|
| Hex terrain fill | `<path>` or `<polygon>`, `fill` from a class |
| Hex grid stroke | `<path>`, `fill:none`, stroke from a class keyed on terrain |
| Hex perimeter ring | `<path>` on an inset hexagon, thick stroke |
| Hex-centred glyph | `<use href="#sym" transform="translate(cx,cy)">` |
| Hex glyph at a slot | same, with the slot offset folded into the translate |
| **Side glyph (fire dot)** | `<use>` with `translate(edge midpoint) rotate(direction angle)` |
| Single-edge stroke | `<line>` or a two-point `<path>` |
| Edge chain | one `<path>` through the vertex sequence, `stroke-dasharray` for dashed kinds |
| Parallel edge chains | the same path with a lateral offset applied at build time |
| Centre-to-centre link | `<path>` through hex centres |
| Rail hatching | two stacked `<path>` elements — a solid casing and a dashed tick line — rather than a marker pattern |
| **Edge-centred directional glyph** | `<use>` with `translate(edge midpoint) rotate(edge normal)` |
| Region tint | `<path>` over the union of the hex set, or a `<g>` of hexes with a class |
| Region outline | `<path>` along the set's boundary edges |
| Hex identifier | `<text transform="translate(…) rotate(90)">` |
| Place label | `<text>` at an offset |
| Curved area label | `<textPath href="#somepath">` with `letter-spacing` |
| Panel, track, box, table | `<g>` of `<rect>` and `<text>`, optionally inside a `rotate()` |

### 4.3 Why classes and data attributes matter here

Three of the sheets' behaviours argue for putting logical identity in the markup rather
than baking appearance into geometry:

- **Live state.** Rail ownership on the Russian Campaign map changes twice per game turn;
  strategic hex control on the Dai Senso map changes constantly. Emitting
  `data-owner="axis"` and restyling by CSS avoids regenerating geometry.
- **Palette swaps.** Ben has already used dual-palette SVG elsewhere. Driving the six
  Tarawa position hues from CSS custom properties lets one file serve a light theme, a
  dark theme and a colour-blind-safe palette — important because on that map colour is the
  activation key, so the hues must stay mutually distinguishable in every palette.
- **Layer toggling.** Dai Senso has five overlapping region labellings that cannot all be
  shown at once. A `<g class="layer regions countries">` per labelling makes showing one at
  a time trivial.

### 4.4 Geometry helpers the renderer needs from `hexmap`

Small, but each is used constantly:

- hex centre in output units, for a given `HexAddr` and a given hex size
- the six corner points of a hex, for the fill path
- an **inset** hexagon path, for perimeter rings
- edge midpoint, for a given `EdgeAddr`
- the **angle** of an edge normal, for edge-centred glyphs
- the **angle** of a `Direction`, for side glyphs and facing arrows
- the vertex sequence of an edge chain, with mitre or round joins at the turns
- the boundary edge list of a hex set, for region outlines
- a lateral offset of an edge chain, for the parallel-chain case

With `CoordABC` naming vertices and `CoordQRS` naming directions, all nine are table
lookups or short vector sums rather than trigonometry from scratch.

---

## 5. What to simplify away

The user's instruction was that not every nuance need be reproduced, and the three
sheets carry a lot that no rule reads. The recommendation in each case is to replace
drawn geography with a per-hex terrain value.

| Printed detail | Sheet | Recommendation |
|---|---|---|
| Lake-like curves inside a hex | Tarawa | one terrain value, e.g. `swampy` |
| Free-form crater lobes | Tarawa | terrain value `rough` |
| Individually placed palm sprigs | Tarawa | terrain value, optionally one texture |
| Angled white airstrip polygon | Tarawa | terrain value `airstrip`, whole hex |
| Rotated building rectangles | Tarawa | terrain values `building`, `fortified` |
| Drawn shoreline with soft glow | all three | drop; it emerges from the terrain fills |
| Mountain mottling | TRC, Dai Senso | flat fill, or one texture |
| Irregular woods patches spilling across hexes | TRC | flat fill per hex |
| Offshore islet shapes inside sea hexes | TRC, Dai Senso | drop |
| Inland river courses drawn through hexes | Dai Senso | drop — keep only river *hexsides* |
| Sea range dots | TRC | keep; cheap, and it is a genuine drawing aid |

Two things that look decorative but must **not** be dropped:

- **The Russian Campaign's black coastal line.** There is a drawn shoreline and a separate
  rule-bearing coastal line that ground units may not cross. They nearly coincide but are
  different objects, and only the second one matters. It must be authored as its own edge
  chain, not derived from terrain fills.
- **Dai Senso's river hexsides**, as distinct from its drawn inland rivers. Same trap, same
  answer.

---

## 6. Where the three sheets disagree

Four places where a single fixed implementation will not serve all three.

**Water.** Tarawa's water hexes are entered constantly and carry fire dots, wreck markers
and arrival boxes. The Russian Campaign's sea hexes are gridded and numbered but never
entered. Dai Senso's sea hexes are entered under conditions and carry beachhead markers and
rotated panels. "Water" is not one concept; the library should treat sea as an ordinary
terrain value and let the game decide what may enter.

**Region layers.** Tarawa's regions are static authored sets. The Russian Campaign's are
static with a tint and a boundary. Dai Senso has five overlapping labellings, several of them
mutable. A region model that assumes disjoint membership will fail on the third.

**Colour semantics.** On the Russian Campaign map colour is styling. On Tarawa and Dai Senso
it is data — the position activation key and the faction respectively. The library needs a
named palette that both the renderer and the game logic can read.

**Line networks.** Tarawa's position connectors are a handful of isolated segments between
adjacent hexes. The Russian Campaign's rail is one large network with mutable per-segment
ownership. Dai Senso has two networks, road and rail, which also project onto hexsides as
movement costs. A `LinkNetwork` needs to cover all three: isolated segments, long chains, and
a derived hexside view.

---

## 7. Suggested build order

1. **Geometry helpers** in `hexmap`: centre, corners, inset path, edge midpoint, edge normal
   angle, direction angle, vertex path for an edge chain, boundary edges of a hex set, lateral
   offset of a chain.
2. **Address types**: `HexAddr`, `EdgeAddr`, `SideAddr`, `Direction` with rotation.
3. **Data tables**: `HexLayer`, `EdgeLayer`, `SideLayer` — enough to draw a plain terrain map
   with rivers and fire dots, which is already most of Tarawa.
4. **Paths and links**: `PathSet` and `LinkNetwork`, with per-segment state.
5. **Regions and labels**: `RegionSet` with overlapping membership, `LabelSet` with the four
   placement modes.
6. **Furniture**: panels, tracks, boxes, tables, with rotation and with tracks reporting a
   cell's numeric value.
7. **SVG writer**: layered `<g>` output, `<symbol>` library, classes and data attributes
   throughout.

A useful early milestone: render the Tarawa hex field from terrain values plus a fire-dot
`SideLayer` and nothing else. If that comes out legible, the hardest addressing case is
already solved.

---

## 8. Open questions

- **Textures or flat fills?** The maps use texture heavily. Flat fills are far simpler and
  probably clearer on screen. Worth deciding before writing the SVG writer, since it changes
  whether `<pattern>` is needed at all.
- **How much slot layout should the library own?** Dai Senso's four-glyphs-in-a-hex case argues
  for the library solving it once. Tarawa's consistent offsets argue the same. But a fixed slot
  set may not survive contact with a fourth game.
- **Should `SideAddr` and `EdgeAddr` be one type with a flag?** Two types is clearer and catches
  the conflation error at compile time. One type is less to carry. Leaning to two.
- **Hex identifier axis conventions.** Neither the Tarawa nor the Dai Senso digit order was
  established from the scans — the printed numerals are too small. Both should be settled
  against the physical maps before writing any printed-ID to coordinate mapping. The Russian
  Campaign's letter axis *is* settled: letters name rows north to south, `A` through `QQ`.
- **Do vertices need to be addressable in their own right?** No rule on any of the three sheets
  reads a vertex. They are needed for drawing edge chains and for the mitre behaviour at turns.
  That is a rendering argument, not a rules argument, and it is worth saying so plainly rather
  than overstating the case.

Copyright Ben Paul Wise. All Rights Reserved.
