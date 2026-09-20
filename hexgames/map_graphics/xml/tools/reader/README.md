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
python legend.py DIR                 (finds the panel, cuts swatches, writes legend/ for naming)
python cells.py DIR                  (scores every cell against the named swatches)
python structure.py DIR              (chains, regions, glyphs, doubts; writes chain files)
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
record for the example maps is `anchors.json`) and turns index ranges into printed ids: printed
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
`legend/panel.jpg` (the panel as found), `legend/swatch-NN.jpg` (one per swatch, cut and normalised),
and `legend/vocabulary.json` after naming:
```
[{"swatch": "swatch-03.jpg", "name": "River hexside", "kind": "side-along", "sheet": "river",
  "shape": "line", "colour": "#78bee1"},
 {"swatch": "swatch-07.jpg", "name": "Railroad", "kind": "side-across", "sheet": "rail", …},
 {"swatch": "swatch-01.jpg", "name": "Clear", "kind": "hex", "sheet": "clear", …},
 {"swatch": null, "name": "town", "kind": "glyph", "sheet": "town", "from": "library/gdw-1986"}]
```
`kind` is one of `hex`, `ring`, `glyph`, `side-along`, `side-across`, `side-mid`, `vertex`, `region`.
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

### structure (structure.py)
- `terrain.json` `{"1829": "clear", …}` with the runner-up share as the doubt score.
- `<kind>.json` chain files in chain2catalogue.py's schema, ends with reasons, `todo` empty, and
  `"seen": true` on every hexside (structure.py never writes a hexside with no evidence).
- `regions.json` `{"country": [{"name": "Poland", "hexes": […]}], "sea-area": […]}`.
- `places.json` in merge.py's places and markers shape, names filled by the label pass.
- `doubts.json` in task 16's schema, every entry produced by a score in the middle band.
Then `chain2catalogue.py` and `assemble.py` run unchanged.

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
