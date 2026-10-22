Copyright Ben Paul Wise. All Rights Reserved.

# reader -- grammar-first reading of a hex wargame map (design contract D1 and D4)

Status 2026-09-18: contract for Ben's review (doc/2026-09-18-grammar-first-reading-plan.md, phase D).
Only `lattice.py` exists; it was built against this contract and run over the eighteen example maps
(doc/2026-09-18-lattice-first-run.md has the table). The other tools are named here so that their
files are fixed before they are written. lattice.json also carries `basis` (the fitted lattice vectors
in pixels), `grown` (every cell's position as grown from the ink, with a confidence) and `refit` (the
perfect lattice least-squares fitted to the grown PRINTED cells, with the slip rounds); cells are cut
at the grown positions, never with the grid element alone. `batch.py` runs lattice.py over a folder
of scans and writes `work/batch-report.txt` and `work/batch-table.json`.

Principle: the lattice enumerates every cell; each tool cuts a small strip or patch at a known cell and
scores it; nothing looks at a region larger than a cell except `lattice.py` (the whole scan, once) and
`legend.py` (the legend panel, once). Model looks are bounded: one to name the legend swatches, one
contact sheet per thirty doubts. Every tool prints a `--report` a person can check without images.

## Tools and files

```
python lattice.py IMAGE --out DIR [--overlay] [--svg] [--anchor ID C R ID C R ...] [--spacing PX]
python batch.py FOLDER [--hint "NAME=PX" ...] [--only NAME ...]   (lattice.py over every scan)
python legend.py DIR [--panel X0 Y0 X1 Y1]   (finds the panel, cuts swatches, writes legend/ for naming)
python legend.py DIR --placed legends.json   (swatches a person placed by eye; same outputs)
python legend.py DIR --finish                (naming.json -> vocabulary.json)
python cells.py DIR [--vocabulary OTHER/legend] [--overlay]   (scores every printed cell against the swatches)
python structure.py DIR [--oracle SHEET.xml] [--overlay]   (terrain, chains with end reasons, places, doubts)
python compare.py DIR ...            (difference image; round trip; swap; common style)
```

`DIR` is the map's work folder, `work/<map>/`, never committed. Its files, in the order they are made:

### lattice.json (lattice.py)
```
{"source": "…/Stalin Moves West map.jpg", "size_px": [1650, 2550],
 "orientation": "pointy", "size": 53.29, "ox": 12.3, "oy": 40.1, "rotation_deg": 0.11,
 "spacing": 92.3, "basis": {"u": [92.3, 0.0], "w": [46.1, 79.9]},
 "index": {"cols": [0, 44], "rows": [0, 27], "offset": "odd"},
 "printed": {"count": 326, "threshold": 0.31, "cells": ["c3r5", …]},
 "clip_index": ["c0r0-c0r27", …],
 "refit": {"lattice": {"ox": 12.1, "oy": 40.3, "u": [92.28, 0.02], "w": [46.1, 79.9]},
           "rounds": [{"round": 0, "rms_px": 5.3, "slips": 23, "spacing": 92.29}, …],
           "cells": "printed"},
 "grown": {"worst_px": 72.0, "cells": {"c3r5": [289.4, 440.1, 0.62], …}},
 "numbering": {"id-format": "{row:02}{col:02}", "order": "row-col", "half": 2,
               "col-start": 27, "row-start": 34, "col-step": 1, "row-step": -1,
               "offset": "odd", "parity": 0, "orientation": "pointy",
               "anchors": [{"id": "2232", "col": 5, "row": 12}, {"id": "1042", "col": 15, "row": 24}, …]},
 "method": {"candidates": [{"spacing_px": 92.3, "angle": 0.0, "phase_score": 3.1, "orientation": "pointy"}, …],
            "ring": 6, "harmonic": 4, "phase_score": 3.1, "fitted_cells": 435, "piecewise": false},
 "passed": true, "notes": []}
```
`size` is the circumradius in source pixels and `ox, oy` the centre of index (0, 0), exactly the grid
element's meaning, so `common.make_grid` builds the same lattice. `index` gives the lattice's cell
ranges over the image before numbering is known; `numbering` is filled from the anchors (printed ids
paired with lattice indices, read by a person from crops of the scan with the index drawn; the
Rule of hex grids (Ben, 2026-09-21): the outline may be irregular, as Target Leningrad's coast is, but
there is never a hole inside the grid; a cell enclosed by printed cells is printed. lattice.py applies
it after the printed-cell test: unprinted cells that cannot be reached from the lattice's border
through unprinted cells are holes, marked printed and listed under "holes" in lattice.json (Target
Leningrad's Lake Peipus hexes). Cells at the outline that the test still misses are the eye's:
"printed" in anchors.json. The eye's record for the example maps is `anchors.json`, which may also list under "printed" the ids of hexes the printed-cell test missed, passed as `--printed`) and turns index ranges into printed ids: printed
`col = col-start + col-step * c'` and `row = row-start + row-step * r'`, where `(c', r')` is the
lattice index re-paired for the sheet's grid `offset` ("odd" = the renderer's pairing, `c' = c`;
"even" = the other zigzag, `c' = c + ((r + parity) % 2)` for pointy, `r' = r + ((c + parity) % 2)`
for flat). Anchors must include both row parities (pointy) or column parities (flat), or the offset
cannot be told; a third anchor checks the fit. Ids are all digits, split in half; letter columns
(The Russian Campaign: Y23, AA24) are not yet supported.
`printed` is the set of cells whose outline the scan draws, from the periodic-energy test; `clip_index`
its complement inside the index box. `passed` means: two anchors agree with the numbering, and the
phase score is above the gate. The dense residual per hex is NOT computed; it was the old process's
pixel habit.

### legend/ (legend.py)
`legend/panel-N.jpg` (each panel, swatches numbered), `legend/swatch-NN.png` (one per swatch, cut and
normalised), `legend/contact.jpg` (every swatch with its printed label, for the one naming look),
`legend/naming.json` (one row per swatch: position, size, interior colour, per-edge and spoke ink, a
kind guess, and `name`/`kind`/`sheet` to fill in) and `legend/vocabulary.json` after `--finish`:
```
[{"swatch": "swatch-03.jpg", "name": "River hexside", "kind": "side-along", "sheet": "river",
  "shape": "line", "colour": "#78bee1"},
 {"swatch": "swatch-07.jpg", "name": "Railroad", "kind": "side-across", "sheet": "rail", …},
 {"swatch": "swatch-01.jpg", "name": "Clear", "kind": "hex", "sheet": "clear", …},
 {"swatch": null, "name": "town", "kind": "glyph", "sheet": "town", "from": "library/gdw-1986"}]
```
`kind` is one of `hex`, `ring`, `glyph`, `side-along`, `side-across`, `side-mid`, `vertex`, `region`.

Two routes fill naming.json. The finder (`legend.py DIR`) paints the printed hexes out, looks for hex
outlines at legend sizes with the line's own contrast, and groups them into panels; it is right on
table-style keys (Tannenberg, 9 of 9 named) and finds nothing on legend-less maps (Tallinn), but on
charts where the swatches sit in irregular cells (BFM, SMW) it needs `--panel` and still misses or
over-reads. The eye's route (`--placed legends.json`, Ben's choice of 2026-09-20, budget over tuning)
reads each swatch's centre and radius off a gridded crop of the scan into `legends.json`, keyed by work
folder, as `[name, kind, sheet, x, y, size]` with the panel rectangle; legend.py cuts and measures
those exactly as the finder would, marks each row `"placed": "eye"`, and `--finish` cannot tell the
routes apart. BFM redone (7) and SMW (16) were done this way. An editor makes the second route a
rectangle drag and a click per swatch.
`sheet` is the id the structure document will use. A swatch that came from the style library rather
than the print carries `from`. The style document (palette, terrains, lines, marks) is derived from this
file and the swatch colours; until task 14's mechanism A lands it is written as the sheet's declaration
sections, the parts assemble.py today calls `keep`.

### cells.json (cells.py)
One row per cell, scores in [0, 1] per sheet id, only kinds the vocabulary declares:
```
{"hex":    {"1829": {"hex": {"clear": 0.91, "forest": 0.06}, "glyph": {"city": 0.97},
                     "ring": {}, "vertex": {"n": {}, "s": {}}}},
 "side":   {"1829-1830": {"along": {"river": 0.02, "border": 0.00},
                          "across": {"rail": 0.88}, "mid": {"bridge": 0.01}},
            "1829:w":    {"along": {…}, "across": {…}, "mid": {…}}}}
```
Cell names are printed ids and the hexside names common.py already uses. Nothing in this file is a
decision; it is measurement.

How the scores are measured (2026-09-20, first version, proved on the BFM pair only). Every reference
is taken from the legend's own swatch with the probes that measure the cells: a fill is the swatch's
interior colour; a glyph is the colour of what is drawn over the swatch's fill; a line kind is the
colour drawn over the fill in the one side band, spoke or side midpoint of the swatch that carries
drawing. A cell's fill scores by colour similarity (zero at 80 RGB units). A line scores by the fraction
of its band's length at which some pixel across the band is the line colour and is neither fill nor the
printed hex outline, so a river that wanders off its hexside still scores by the length it covers, and
woods green (within tolerance of BFM's tan rail) does not count as rail because it is the fill. A glyph
scores by how much is drawn in the interior that no line kind explains, times the similarity of its
colour. Whatever the swatch does not show scores 0; a kind with no swatch (Clear on both BFM printings)
has no column and is the structure step's default. Only printed cells are scored; the grown lattice's
other cells are margin and furniture. A map with no legend gets its swatches as exemplar hexes on the
map itself, placed by eye in `legends.json` (BFM map 1), so nothing is borrowed from another print.

Second day (2026-09-21, SMW and Target Leningrad, Ben's choice over the bland BFM original). Added:
the fill of a hex is read in an annulus inside the outline and outside any pictogram (a swatch whose
star covers the centre would otherwise call the star its fill); a pixel counts for a line only when the
line colour is nearer than every other colour the cell can show (both fills with the palette of their
printed texture, the outline, the other line kinds), which is what separated SMW's sea from its river
and rough's speckles from the red front line; a fill swatch with more than 5% drawn over its fill is a
textured kind and cells score it half by fill colour, half by how much of that texture they show
(Target Leningrad prints forest and rough as mottle over clear); glyphs score per kind from the drawn
pixels nearest that kind's colour, in the map's glyph slot (`legends.json` "glyph-slot", Target
Leningrad draws pictograms in the upper half); exemplar hexes may be named by printed id. SMW: fills
right by eye (rough 38, marsh 17, sea 16, forest 5, clear the rest), rivers and both borders on the
printed hexsides, cities 25 (the map has about 20), rails under-read (dashed, 30 spokes), stars 0 and
oil 104 (the grey derrick colour is also the rail dash and the hex id). Target Leningrad: sea 12,
forest 11, rough 41, clear 58, rivers 63 hexsides, fortification boxes on 26 hexsides, rails 3.

Known limits after the BFM pair: glyph kinds are told apart by colour only, so BFM redone's cities
(grey blob on tan roads) never reach 0.5 against their tiny legend rendering; BFM map 1 prints rivers,
rails and the hex outline in near-identical dark greys and its fortification grey is 20 units from
paper, so colour alone puts a river on nearly every hexside there. Weight and waviness probes are the
next step, in structure.py's ranking or here; they were not attempted within the 2026-09-20 cap.

### structure (structure.py)
- `terrain.json` `{"1829": "clear", …}` with the runner-up share as the doubt score.
- `<kind>.json` chain files in chain2catalogue.py's schema, ends with reasons, `todo` empty, and
  `"seen": true` on every hexside (structure.py never writes a hexside with no evidence).
- `regions.json` `{"country": [{"name": "Poland", "hexes": […]}], "sea-area": […]}`.
- `places.json` in merge.py's places and markers shape, names filled by the label pass.
- `doubts.json` in task 16's schema, every entry produced by a score in the middle band.
Then `sheet.py DIR` writes `DIR/chain/sheet.xml` straight from the lattice numbering, the vocabulary
and the chain files, validated against hexsheet.xsd, and `hexsheet2svg.py` renders it. (For a map that
has an image2sheet configuration, `chain2catalogue.py MAP --chain-dir DIR/chain` and `assemble.py` are
the other route; SMW was built both ways on 2026-09-21 and the two agree.) The grid element is the
lattice: size and origin from the fitted centres, cols, rows, id format and starts from the numbering,
the stagger measured from the centres, the unprinted cells of the rectangle in clip. Style is what the
reader measured, each fill and stroke its swatch colour; the document is the structure.

Three maps through the whole track, 2026-09-21, all valid and rendered (`work/<map>/chain/sheet.png`):
SMW as above; Target Leningrad (exemplar swatches by printed id): terrain sea 12, forest 11, rough 54
(rough over-read where the print's forest mottle is), clear 60; rivers 85 hexsides in 36 chains, most
ending at the map edge because the coastline reads as river; cities 22 against 14 printed; the grid
and every id right. Tannenberg (legend of 15 swatches, the six line rows placed by eye): terrain
clear 864, swamp 48, forest 15 (the print's forest mottle mostly missed), broken 12; the line kinds are
over-read against the grey hex grid and the yellow paper (river 710 hexsides, border 443, blocked 136)
and the swatch line colours are too pale to draw; rails 42 steps in 23 chains, 14 ending at places;
197 places against about 40 printed. Ben's bar (2026-09-21, SMW): close enough to finish in an editor,
and the right types of structure present. SMW and Target Leningrad meet it; Tannenberg meets it for
the grid, terrain and places, not yet for lines.

First version (2026-09-21, SMW). Terrain is the best fill above 0.3, else the default (`--default
clear`). Chains grow by hysteresis over the hexside scores: seeds at or above `--high` (0.6; rail 0.4),
continued through hexsides at or above `--low` (0.3; rail 0.2) that share a vertex (rail: a hex), split
at junctions. Two structural rules, both recorded as doubts rather than applied silently: a lone
hexside or step with both ends unexplained is not a line, and for rail a hex carrying four or more
steps is a false star (marsh mottle, text) while a piece of fewer than three steps reaching neither a
place nor the map edge goes nowhere. End reasons: edge, sea (river beside sea terrain), junction, place
(rail at a city or port), unexplained. `places.json` carries glyphs only, no names (nothing is
invented; assemble.py now accepts a place without a name), and other glyph hits go to `glyphs.json`.
`doubts.json` is in task 16's schema. Against the committed SMW sheet as oracle: terrain agrees on 302
of 319 hexes (the rest are coastal/clear/sea calls along the Baltic); river hexsides 125 of 135 found
with 40 extra; border 79 of 89 with the front line's 147 hexsides as the extras (the sheet has no
front line); rail 47 of 122 steps with 34 extra, the known weak kind. The sheet built from this reading
validates and renders; that render, `work/<map>/chain/sheet.png`, is the approximation the map track
is for, and the doubts list plus the unexplained ends are the editor's input.

### compare (compare.py)
`diff.png` render against scan; `roundtrip.txt` structure read from the render against the structure
read from the scan; `swap/<style>.svg` renders under other styles; `common.svg` under the neutral style.

## The style library (D4)

`library/<house>-<year>/` holds swatch crops cut by legend.py from maps whose legend was read, one per
sheet id, plus `library/index.json`:
```
[{"house": "decision-games", "year": 2018, "map": "smw", "kind": "side-across", "sheet": "rail",
  "swatch": "decision-games-2018/rail.jpg", "note": "thin black, cross ticks"}]
```
A map with no legend borrows the nearest house's rows; the borrowed rows are written into its
vocabulary with `from`, and the reading's report lists every borrowing as an assumption. The library is
also the input to the rendering-swap test: each house is a style.

## Gates
- lattice: `passed` true, and one whole-map overlay looked at by a person.
- legend: every swatch named, every name bound to a sheet id, kinds cover what the scan shows.
- cells: none; measurement only.
- structure: chain_check.py zero non-adjacent pairs, ends explained or listed; doubts count reported.
- compare: round trip identical; swap and common style render for every style in the library.

Copyright Ben Paul Wise. All Rights Reserved.
