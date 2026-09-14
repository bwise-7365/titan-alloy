Copyright Ben Paul Wise. All Rights Reserved.

# hexsheet — an XML map-sheet language for hex wargames

`hexsheet.xsd` defines the language; `hexsheet2svg.py` is the reference renderer
(validates against the XSD, writes SVG, and with `--png` rasterises through Inkscape).
Four instance documents approximate real sheets; each is compared with its scan in
`C:\Library\War-Games\...`.

| File | Sheet | Orientation | Numbering | Validates |
|---|---|---|---|---|
| `d-day-at-tarawa.xml` | D-Day at Tarawa, 1786 × 1153 | pointy | `{row:02}{col:02}`, rows 25→1 southward, A14 = 2336 | yes |
| `the-russian-campaign.xml` | The Russian Campaign 5th, 1224 × 1483 | pointy | `{rowletter}{col}`, A..QQ southward, columns 33→1 eastward, Kerch Strait KK19/KK20 | yes |
| `dai-senso.xml` | Dai Senso!, 3990 × 3198, two sheets | pointy | `w`/`e` + `{row:02}{col:02}`, rows numbered from the south, columns from each sheet's west edge | yes |
| `panzergruppe-guderian.xml` | Panzergruppe Guderian (Cyrillic), 5615 × 3727 | **flat** | `{col:02}{row:02}` | yes |

Sheet width and height are the scan's pixel size, so a render at `--scale 1` overlays the
scan directly.

```bash
python hexsheet2svg.py the-russian-campaign.xml --png --scale 1
```

## Design

One element per address kind from `..\graphic-element-taxonomy.md`:

```
sheet  @id @title @source @width @height @background @font
  grid+      @orientation=flat|pointy @offset=odd|even @cols @rows @size(circumradius) @ox @oy
             @id-format @col-start @col-step @row-start @row-step @terrain @show-ids @id-side @clip
  palette    color* (@id @value)          -- every colour is a palette id, never a literal
  terrains   terrain* (@fill @stroke @pattern)
  lines?     line* (@stroke @width @dash @casing @casing-width @ticks @opacity)
  then, in any order:
  hexes      @terrain @ids                 -- bulk terrain assignment
  hex        @id @terrain @ring @ring-width @name > glyph* (@symbol @slot @color @text @dir) side* (@dir @symbol @color)
  edge       @at=HEX:DIR @line @symbol @color @label      -- one hexside: stroke and/or edge-centred glyph
  path       @kind @name @line @edges @offset              -- an ordered hexside chain
  link       @kind @name @line @hexes @owner               -- an ordered hex-centre chain; drawn smoothed:
                                                             through a hex, hexside midpoint to midpoint;
                                                             at ends and junctions, midpoint to centre
  region     @layer @name @hexes @tint @opacity @outline @label @label-at
  label      @text (@at @slot | @x @y | @path) @angle @size @color @weight @italic @spacing @halo @anchor
  panel      @id @title @x @y @w @h @rotate @fill @stroke > text* box* track* table*
```

- Hexes are addressed by their **printed identifier**; `grid/@id-format` says how it is generated
  from column and row (`{col}`, `{row}`, `{col:02}`, `{rowletter}`, `{colletter}` and literal text;
  letters run A..Z then AA, BB, ..). Column and row numbering may start anywhere and run either way.
- A hexside is `HEX:DIR`. Flat-topped hexes use `n ne se s sw nw`; pointy-topped use `ne e se sw w nw`.
- `side` is the hex-plus-one-of-its-edges address (Tarawa's fire dots): a glyph belonging to one hex,
  placed on that edge's midpoint, facing in. `edge` is the shared boundary (a river, a strait arrow).
- Symbols are a fixed vocabulary the renderer draws (`position-badge`, `fire-intense`, `city-major`,
  `port`, `strait`, ...); the XSD enumerates them.
- Layers are emitted in the taxonomy's order: terrain, regions, grid, edges, links, rings, hex glyphs,
  side glyphs, labels, panels. Panels may sit over live hexes and rotate.

## How the four instances were made

`tools\` holds the scripts. Everything numeric came from the scans; the text came from the digests
and from reading the sheets.

1. `calibrate.py` finds the hex grid in a scan (cross-correlation of the grid-line pixels with one
   hex outline, folded by the lattice period) and prints `size ox oy`.
2. `overlay.py` draws index-labelled hexes over a crop, to pin printed numbering to grid indices
   against landmarks (Kerch Strait, position A14, legible IDs).
3. `sample.py` classifies each hex's terrain by a per-pixel nearest-colour vote → `<hexes>` lists.
4. `trace.py` detects coloured lines along hexsides (`edges`), between centres (`links`), and blobs at
   hexside midpoints (`sides`), inset outlines (`rings`), centres and slots → `edge`, `link`, `hex` fragments.
5. `build_*.py` assembles each sheet: header, sampled fragments, hand-placed cities, labels and furniture.

## Known approximations

- Terrain is one value per hex; sub-hex geography, drawn coastlines and islets are gone by design.
- Rivers that meander through hex interiors (TRC, PGG) are only partly caught by hexside tracing.
- The tracer cannot tell Dai Senso's rail from road, nor Tarawa's steady from intense fire dots.
- Tarawa's position badges carry their colour but not their printed ID (only A14 is known).
- Cities and place names were positioned by hand from a reduced view; a few land one hex off.
- Furniture reproduces titles, tracks and tables, not the full printed text of every box.
- Dai Senso's `map-detail.png` numbers hexes one row and one column lower than the game-map scan;
  the game-map scan's own labels were followed.

Copyright Ben Paul Wise. All Rights Reserved.
