Copyright Ben Paul Wise. All Rights Reserved.

# image2sheet -- building an accurate hexsheet from a map image

A repeatable process for turning a scanned or photographed map (PNG, JPG, or a raster page of a PDF)
into a hexsheet XML document that matches the print, hex by hex and hexside by hexside. First run:
Panzergruppe Guderian (tasks/12-pgg-map.md); then D-Day at Tarawa, The Russian Campaign, Dai Senso and
out-of-sample maps. Decision log: PLAN.md, 2026-09-14 "Building sheets from images".

## Principles
1. **Fidelity, not plausibility.** Record only what is printed. Never invent a feature to make a network
   connect, and never delete a printed one to make a check pass. Checks report; people decide.
2. **Geometry first.** Every later stage trusts the grid. Verify it before reading any feature.
3. **Small, local, full-resolution looks.** A reader (a model or a person) sees one tile of about 4 x 4
   hexes at the scan's own resolution, with the grid and hex ids drawn on it. Never read a shrunken
   whole map to place a feature.
4. **Two independent readings where they are cheap.** Tiles overlap, so border hexes and hexsides are
   read twice; a rail or road exit must match its neighbour's. Disagreements become questions.
5. **Plain assembly.** A script turns the catalogue into XML one to one. No tidying, no re-routing.
6. **Verify against the image, not against the catalogue.** The final pass compares the render with the
   scan, tile by tile, so reading mistakes are caught, not confirmed.
7. **Images in, small copies out.** Nobody Reads an image over about 300 KB: large images have dropped
   workers' connections. Tools write JPEG tiles; readers Read those.

## Per-map configuration: `maps/<map>.json`
One file per map, the only map-specific input besides the images:
- `sources`: the primary image (the one the sheet's pixel frame follows) and any secondary images, each
  with its own calibration.
- `grid`: orientation, offset parity, id format, first printed column and row, and the printed extent to
  verify (first and last column and row numbers as printed).
- `control_points`: at least 12 printed hex numbers spread over the whole map (all four corners, the
  centre, every edge), each with its pixel position read from a zoomed crop.
- `vocabulary`: the map's legend mapped to hexsheet ids -- hex terrains, place glyphs, hexside lines
  (river, border, zone, mountain, lake, coast ...) and link kinds (rail, road ...) -- with a short
  description of how each is printed ("rail: black line with ticks").
- `candidates` (optional): colour ranges or other image-processing hints per feature (stage 4).
- `sheet`: the output path and the parts of an existing sheet to keep (palette, labels, panels) or
  rebuild (grid, terrain, places, hexside lines, links).

## Stages (each a script under this folder; each writes to `work/<map>/`, never over the sheet)
1. **calibrate** -- fit the grid (size, origin, and for a two-grid sheet each grid's offset) to the control
   points by least squares. Report the residual for every control point and the worst per region.
   GATE: every residual under 0.1 hex, and the printed extent fits inside the image with no printed
   hex left outside the grid. A failure stops the process.
2. **overlay** -- draw the fitted grid, the hex ids and a dot at every hexside midpoint over the primary
   image. Also write a shrunken overview (under 300 KB) only for a person's orientation.
3. **tile** -- cut the overlay into tiles of 4 x 4 hexes with one hex of overlap on every side, at full
   resolution, each a JPEG under about 300 KB, named by the hexes it covers, with a manifest.
4. **candidates** (optional) -- where colours separate features cleanly (sea, white rail with a casing,
   blue rivers that are not the grid colour), propose hexside and exit candidates per tile with colour
   masks and line thinning. Candidates are hints shown to the reader, never written to the sheet directly.
5. **catalogue** -- a reader fills one JSON record per tile (schema below) from the tile image, the
   vocabulary and any candidates. Rules for the reader: record only what is printed; mark anything
   unreadable (under a counter, a label, a fold) as `unclear` with a note; do not guess.
6. **merge** -- combine tile records into one catalogue. Every hexside read in two tiles and every rail or
   road exit must agree with its twin (A's exit to the north-east is B's exit to the south-west).
   Output: the catalogue plus `disagreements.json` (hex or hexside, the two readings, both tiles).
   GATE: every disagreement and every `unclear` is resolved by a second, focused look (a crop centred
   on the hex or hexside, larger if needed) or listed as a named exception with its reason.
7. **assemble** -- write the sheet XML from the catalogue: grid from calibrate; terrain; places;
   hexside lines as `<edge>` elements; links as chains built from matching exits. Kept parts of the old
   sheet are copied unchanged. Validate against hexsheet.xsd.
8. **render** -- hexsheet2svg.py to SVG and PNG, then tile the render exactly as stage 3 tiled the scan.
9. **verify** -- a reader looks at each scan tile beside its render tile and lists every difference in
   JSON (hex or hexside, what the print shows, what the render shows). GATE: repeat stages 5-9 for the
   differing hexes until the list is empty or holds only named exceptions.
10. **report** -- counts per feature, the exception list, residuals, and a list of places for a person to
    spot-check against the print.

`network_check.py` runs after assembly as a report only: a disconnected piece or a stub is a question
("is this printed?"), never a reason to change the sheet. A map's network_check profile records the
printed exceptions by name.

## Catalogue record (one per tile)
```json
{
  "tile": "r05-c09",
  "hexes": {
    "2117": {"terrain": "clear", "place": {"glyph": "city-major", "name": "Smolensk"},
             "exits": {"rail": ["ne", "sw"], "road": ["n", "se", "sw"]}, "unclear": []}
  },
  "hexsides": {
    "2117:n": {"line": "river"},
    "2117:ne": {"line": null}
  },
  "notes": ["2118:se partly under a counter; river continues on 2218:nw"]
}
```
Directions are the sheet orientation's compass names (flat: n ne se s sw nw; pointy: ne e se sw w nw).
A hexside is named once, from the hex with the lower id, as hexsheet XML writes edges.

## Choosing the reader model
Tiles are small, full-resolution and annotated, and the process checks itself twice (overlap agreement,
render-versus-scan differences), so a Sonnet reader is enough for most tiles. Send to a stronger model
only what the checks flag: disagreements that survive one focused look, dense junction tiles (a city
where several railways and rivers meet), and the verify pass for tiles that keep differing. Writing and
changing these scripts is ordinary Python work.

Copyright Ben Paul Wise. All Rights Reserved.
