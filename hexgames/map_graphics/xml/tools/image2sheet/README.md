Copyright Ben Paul Wise. All Rights Reserved.

# image2sheet -- building an accurate hexsheet from a map image

A repeatable process for turning a scanned or photographed map (PNG, JPG, or a raster page of a PDF)
into a hexsheet XML document that matches the print, hex by hex and hexside by hexside. First run:
Panzergruppe Guderian (tasks/12-pgg-map.md, the pilot); then D-Day at Tarawa, The Russian Campaign, Dai
Senso and out-of-sample maps. Decision log: PLAN.md, 2026-09-14 "Building sheets from images".

This README is meant to be enough on its own: a worker (Sonnet is enough) runs a new map from it. Every
number quoted "on PGG" is from the pilot and is a starting point, not a constant.

## Principles
1. **Fidelity, not plausibility.** Record only what is printed. Never invent a feature to make a network
   connect, and never delete a printed one to make a check pass. Checks report; people decide.
2. **Geometry first.** Every later stage trusts the grid. Verify it before reading any feature.
3. **Small, local, full-resolution looks.** A reader sees one tile of 4 x 4 hexes (plus a margin) at the
   scan's own resolution or larger, with the grid and hex ids drawn on it. Never read a shrunken whole map
   to place a feature.
4. **Two independent readings where they are cheap.** Tiles overlap by one hex, so border hexes and
   hexsides are read twice. Disagreements become questions, settled by a focused look.
5. **Plain assembly.** A script turns the catalogue into XML one to one. No tidying, no re-routing.
6. **Verify against the image, not against the catalogue.** The final pass compares the render with the
   scan, tile by tile, so reading mistakes are caught, not confirmed.
7. **Images in, small copies out.** Nobody Reads an image over about 300 KB: large images have dropped
   workers' connections. The tools write JPEGs under 300 KB; readers Read only those.
8. **Measure what a mask measures well, read what only an eye can.** Colour masks give coverage and
   presence numbers that are more exact than an eye estimate; a reader decides which hexes a line passes
   through, where a place is, and whether a stroke is a river or a lake outline.
9. **A base process, customised per map.** The stages, the gates and the tools are the same everywhere;
   what a print MEANS is not, and no configuration guesses it. Stage 0 ends in a written list of this map's
   puzzling elements and what each becomes in the sheet, settled with the person who knows the game. Two
   printed styles may become one sheet element, and a printed style may carry information the sheet does
   not keep. Treat a stage that "does not fit this map" as a decision to record, never as a rule to bend.
10. **Structure, not pixels (Ben, 2026-09-16).** The output is XML on a grid that is perfect by
   construction, so nothing has to line up in pixels and no image is ever warped, rectified or retouched.
   The image only has to ADDRESS: if a reader can see a city in hex 2030 of a crooked photograph, the
   sheet gets a city at 2030 and it lands correctly. The sheet records which hexes and hexsides carry
   which feature; it does not reproduce the print's appearance. PGG's rendered rivers are much thinner
   than the printed ones and that is CORRECT, not a defect to fix. Spend effort on getting the structure
   right -- the hex, the hexside, the chain, the terrain -- and spend none on matching pixels, line
   weights or colours. A geometric fit is good enough when every printed hex is unambiguously identified.

## Requirements and layout
- Python 3 with lxml, numpy, scipy and Pillow (PyMuPDF only for PDFs and quick looks). On Ben's machine:
  `C:/Users/bwise/AppData/Local/Python/bin/python.exe`. Run every command from this folder.
- `maps/<map>.json` -- the only map-specific input besides the images (below). Copy `maps/template.json`,
  which documents every key, and run `python check_config.py MAP` before stage 1: it reports missing keys,
  masks and terrains that name each other wrongly, and an unreadable source image, instead of letting a
  later stage throw.
- `examples/pgg/` -- the pilot's own filled-in reader and verify prompts, two worked catalogue records and
  its marker pass, with a README saying what to copy and what to re-derive. The `work/` folder is not kept
  in git, so these are the only examples a new map starts from.
- `work/<map>/` -- everything the stages write: calibration, tiles, candidates, catalogue records,
  resolutions, verify rounds, looks. Nothing is written over the sheet except by `assemble.py`.
- Scripts: `common.py` (shared: config, the renderer's own Grid, hexside names, JPEG writer),
  `check_config.py`, `calibrate.py`, `overlay.py`, `tile.py`, `crop.py`, `candidates.py`, `merge.py`,
  `assemble.py`, `verify.py`, `contact.py` (side-by-side contact sheets for a focused round or a terrain
  audit) and `compare.py`. Each prints its usage when run without arguments.

### Hexside names (used by every stage)
A reader names an inner hexside, and a railway or road step across it, by its two hexes: `0419-0420`
(either order). A hexside on the printed map edge has one hex and is written `HEX:DIR` (flat grids:
`n ne se s sw nw`; pointy grids: `ne e se sw w nw`). `merge.py` writes the canonical form (the two ids in
grid order); `assemble.py` writes `<edge at="HEX:DIR">` from the first hex.

## Per-map configuration: `maps/<map>.json`
See `maps/pgg.json` for a complete example.
- `sources`: `primary` (the image whose pixel frame the sheet follows) and any secondary images, each with
  `path`, `size_hint` (approximate circumradius in pixels), `line_dark` (grey level under which a pixel is
  grid-line ink; 60 on PGG) and `control_points` (filled in stage 1).
- `grid`: `orientation`, `offset`, `cols`, `rows`, `id-format`, `col-start`, `row-start` (and `col-step`,
  `row-step`, `clip` when needed), `id-side` (the hexside the PRINTED id sits against; PGG "s"), `terrain`
  (default terrain) and `extent` (`first`, `last`: printed ids that must be on the grid).
- `tiles`: `core` (4), `margin` (1), `scale` (1.5).
- `vocabulary`: `terrain`, `places`, `hexside_lines`, `links`, `markers`: each id with a short description
  of how the print draws it (colours included). The ids must be the sheet's terrain, glyph and line ids.
- `candidates`: `colour_masks` (per feature: `rgb` list and `tol`, or a `green` hue rule; optional
  `close`, `open`, `grow`), `hex_masks`, `default_terrain`, `terrain` rules (`mask`, `at_least`, `sure`,
  `maybe`), `presence` (per link kind), `city_at_least`, `hexside_lines`, `band`, `along_sure`,
  `along_maybe`, `links`, `half_width`, `along_share`, `min_run`, `end`, `corner`. Stage 4 explains them.
- `sheet`: `path` and `png` (relative to `map_graphics/xml`), `urban` (REQUIRED: `buildings` or `symbol`,
  how the renderer draws cities; written on `<sheet>`), `keep` (parts of the existing sheet copied
  unchanged: palette, terrains, lines, labels placed at x/y, panels) and `place_style` (label size and
  weight per place glyph).

## Stage 0 -- look before configuring (about 20 minutes)
1. Make a whole-map overview under 300 KB (resize to about 1300 px wide, JPEG quality 55) and 700 x 700 px
   full-resolution crops of the four corners and one dense place (`crop.py` works before calibration with
   `--box X0 Y0 X1 Y1 --scale 1`; for images with no config yet, PyMuPDF: open the image as a document and
   use `page.get_pixmap(clip=..., matrix=...)`, remembering that an image page's rect is in points, so scale
   pixel coordinates by `page.rect.width / image_width`).
2. From those alone settle: orientation; which columns (flat) or rows (pointy) are shifted; where the printed
   id sits inside its hex (`id-side`); the printed extent (first and last column and row); how each feature is
   drawn. PGG: flat, even printed columns shifted down (offset "odd"), id at the hex's bottom, columns 01-59
   and rows 01-31 (the old sheet had stopped at 56).
3. Colour census: quantise a crop to 10-12 level bins and list the most common colours; then census small
   boxes placed on one feature each. Record the colours in the vocabulary.
4. Check the rules for how partly covered hexes count. A rulebook often says nothing (PGG's does not), and
   "the terrain covering at least half of the hex" is then a guess, not a rule. On PGG that guess was wrong
   and 123 hexes had to be re-classified after the sheet was finished (the LESSON in stage 9).
5. **Ask the person for labelled hexes now, before any threshold is set.** Several hexes per terrain,
   including the faintest they would still call that terrain, and a few they call clear. Stage 4 sets every
   `at_least` from them. If they are not available, say so and stop at stage 4's tuning rather than picking
   a threshold nobody has tested; the whole sheet, its goldens and its verify pass rest on that number.
6. **Write the decisions list** (principle 9): every printed element whose meaning is not obvious, and what
   it becomes in the sheet, each line settled with the person who knows the game, not guessed. Put it in the
   task file and the vocabulary. Stalin Moves West, for example: two kinds of red dashed line, long-long for
   the start-of-game Soviet front line (which is also the USSR border) and long-short for national borders,
   both drawn as the same `border` element, because the sheet does not need the front line; and city hexes
   that are partly sea (Stettin, Danzig, Konigsberg) whose terrain is the city, with a port glyph, never
   water. A printed distinction the game does not need is dropped ON PURPOSE and written down, once.
7. Look at every other image of the same map (other editions, the original): they settle misprints and
   hidden places later (PGG: the English map reads "W, 2" where the Russian prints "W,1").

PITFALLS
- On PGG railways and roads are straight lines crossing hexes off-centre, and rivers wobble along
  hexsides. A tool that samples hex centres or exact hexside lines cannot read them; the old PGG sheet
  was built that way and was wrong in hundreds of places. Readers decide which hexes a line passes through.
- Woods, swamps and lakes are blobs that ignore hexes. Their terrain comes from measured coverage.

## Stage 1 -- calibrate (15 minutes; about 40k tokens with the eye checks)
1. Approximate centres: `python crop.py MAP primary look.jpg --box 0 0 700 700 --ruler 50 --scale 1` and read
   hex-centre positions off the red rulers. Choose at least 12 hexes: the four corners, the middle of each
   edge, the centre and a ring in between (the fit fails the gate without a point in each of nine regions).
2. `python calibrate.py MAP locate primary ID X Y [ID X Y ...]` refines each centre against the printed grid
   lines and writes `work/MAP/calibrate/primary-ID.jpg` with a red cross and a magenta outline.
3. Eye-check every crop: the crossed hex holds the expected printed number and the outline lies on the printed
   lines. Woods, lakes, names and railways in a hex do not spoil the match (score 0.2-0.6, 5-6 printed edges).
4. Paste the printed control-point lines into `sources.primary.control_points`; run
   `python calibrate.py MAP fit`.
GATE: every control-point residual under 0.1 hex (1 hex = centre-to-centre spacing), a point in each of the
nine regions, every grid hex inside the image, and no printed hex outline outside the grid. The fit also
prints an affine diagnostic (scale x and y, rotation) and a dense check refining every hex centre. PGG: worst
residual 0.008 hex, scale x/y 61.763/61.756, rotation 0.
- A single dense-check outlier with the rest under 0.01 is a feature lying on a grid line (PGG 3815, a
  railway along a hexside): look at it with `crop.py --hex ID --grid` and move on. Many outliers in one
  region mean a warped scan: add control points there or split the source.
- Secondary sources get their own control points and fit if their crops are needed with a grid.

## Stage 2-3 -- overlay and tiles (5 minutes)
- `python overlay.py MAP` writes `work/MAP/overview-primary.jpg` (orientation only).
- `python tile.py MAP scan` writes `work/MAP/tiles/scan/TILE.jpg` and `work/MAP/tiles/manifest.json`.
  A tile holds 4 x 4 core hexes plus one margin hex all round, at scale 1.5, named FIRST_LAST after its core
  (`0117_0420` = columns 01-04, rows 17-20). PGG: 120 tiles of 77-232 KB, 3 s.
- The overlay draws thin magenta hex outlines, a small dot at every hexside midpoint, and OUR id for each hex
  (magenta for core hexes, orange for margin hexes) on the side opposite the printed id. So every hex shows
  two ids naming the same hex; a printed id just above a hexside belongs to the hex above it.

## Stage 4 -- candidates (10 s a run; tuning takes 30-60 minutes on a new map)
`python candidates.py MAP [TILE ...]` measures the whole primary image every run and writes
`work/MAP/candidates.json`, a draft record per tile in `work/MAP/catalogue/draft/`, and a candidate tile per
tile in `work/MAP/tiles/cand/` (magenta hexside strokes for proposed rivers, dashed when only "maybe"; red
railway and yellow road steps centre to centre; rings for crossings near a hexside end; W/S/L terrain letters,
lower case or "?" when borderline). Tune on three or four hard tiles, looking at candidate tiles beside scan
tiles, before running all.
- Per hex: the share of the hex (inset from its outline) covered by each mask in `hex_masks`.
- Per hexside line (river): "along" = share of 20 bins along the hexside holding the mask within
  `band` x size beside it. >= `along_sure` is proposed, >= `along_maybe` is an unclear hint.
- Per link kind (rail, road): runs where the mask crosses the hexside segment. A run in the middle is a step;
  within `corner` of an end it is a "corner crossing" hint; shorter than `min_run` within `end` of an end is
  ignored; longer than `along_share` is "along". A step also needs `presence` (the link's coverage) in both
  hexes, else it becomes a "only a corner of X holds the line" hint.
What worked on PGG, and the pitfall behind each:
- Railway: the PURE white of its dashes (tol 3) grown 5 px. Paper is not pure white. Dark-pixel masks fail:
  grid lines, text and city blocks are as dark as the casing.
- Road: its grey (tol 16), opened 3 px (removes anti-aliased grid and casing edges), grown 2 px (bridges the
  gap where a black grid line crosses it).
- Woods: a list of mottle colours read a fully wooded hex as 0.76. A hue rule (green - red >= 14,
  green - blue >= 22, red >= 40, so swamp's dark hatch stays out) with a 5 px closing fixed it.
- Swamp: the hatch covers only 6-12 per cent of a swamp hex; propose any hex >= 0.03.
- One-hex symbols: the map draws many small woods blobs wholly inside one hex, covering only 0.25-0.45 of it.
  The half rule calls them clear; the first verifier called them woods on sight, and so did the old sheet. A
  terrain rule may add `blob_share` and `blob_cover` (PGG woods: 0.75 and 0.2): a hex holding that share of a
  connected blob of the mask, which covers at least that much of the hex, takes the terrain.
  `candidates.json` records `<mask>_blob` and `<mask>_blob_cover` per hex. Blob rules run only after every
  coverage rule has failed (a swamp hatch that the woods mask partly counts stays swamp).
- Crossings alone zigzag where a line runs near hex corners (Smolensk 2217-2318-2218): the presence test
  fixed it.
- Known weaknesses of these candidates, to fix before a new map's readers start (each cost reader time on
  PGG): lake outlines (river colour) proposed as rivers (skip hexsides touching a hex with lake coverage >=
  0.2); river dips round one hex at "along" 0.65-0.8 were real every time (lower `along_sure` to about 0.65);
  rivers cutting a hex corner to corner are missed and vertex passes counted (follow the river mask's thinned
  centreline); chains under-run by a hexside or two at their ends; woods under a railway or road undercounted
  (add link coverage to woods where the hex is mostly one blob); a corner crossing at t 0.8-1.0 that continues
  a proposed chain was a real step three times out of three.

## Stage 5 -- catalogue (the long stage: about 30k tokens and 9 minutes a tile per Sonnet reader)

### 5a. Worked examples first (45 minutes)
Read two hard tiles yourself (a dense junction; a multi-hex city with vertex passes) and write their records
to `work/MAP/catalogue/read/` with a `notes` array giving the reasoning for every hard call. Turn each hard call
into a numbered decision rule of the reader prompt. On PGG: Orsha (0117_0420) and Smolensk (2117_2420).

### 5b. The reader prompt
Write `work/MAP/catalogue/reader-prompt.txt` from the template below, filled in for the map (folder, python,
id format, id position, vocabulary, the worked examples). Readers are given only its path and their tile
names.

```text
You are a map READER in the image2sheet process. You turn annotated image tiles of a printed hex wargame map
into JSON catalogue records. Fidelity over plausibility: record only what is printed. Never invent a feature
to connect a network and never drop a printed one.
MAP: <title>. <Flat|Pointy>-topped hexes, ids "<format>". Folder: <this folder>. Python: <path>.
HARD RULES: never Read any image except the tile JPEGs named below and crops you make with crop.py (under
300 KB); never open the source images. Edit no file except your own output records; no git; use the Write
tool. Write each tile's record AS SOON AS that tile is finished, before looking at the next.
FOR EACH TILE T: look at work/MAP/tiles/scan/T.jpg (the scan with our overlay), work/MAP/tiles/cand/T.jpg (the
machine's candidates drawn on it) and work/MAP/catalogue/draft/T.json (the candidates as JSON); write
work/MAP/catalogue/read/T.json (same schema, "reader": "sonnet"). The draft is a HINT: check every item against
the scan tile, delete what is not printed, add what is printed and missing.
READING THE OVERLAY: each hex shows its PRINTED id (<where>) and OUR id (magenta core, orange margin) on the
other side; they name the same hex. <Neighbour directions; which columns or rows are shifted.> The tile's AREA
= all hexes with a magenta or orange id: record every area hex, and every hexside or step whose BOTH hexes
are in the area. Map-edge hexsides are "HEX:DIR". Inner hexsides are "A-B".
VOCABULARY: <each terrain, place glyph, hexside line and link kind, and how the print draws it>.
Places are recorded on their hex ("0420": {"terrain": "clear", "place": {"glyph", "name" as printed, "vp"}}),
never as markers, on EVERY hex holding the city's blocks (a city may cover two hexes), never on the hex
under its name label. Markers: <vp texts without a city, in-map entrance marks>; do not read the margin
brackets (a separate pass), river names or titles. <Printed lines and furniture to ignore.>
DECISION RULES:
1. Terrain: at least HALF of the hex. Between about 0.35 and 0.65 the MEASURED coverage decides (keep the
   draft). Override only when the mask counted the wrong thing or is off by more than 0.2 (note it). Where a
   railway or road crosses a blob, woods + link coverage near 0.6 in a hex mostly under one blob is woods.
2. A line through a hex vertex shared by A, B and C, from A to C: step A-C, not A-B-C, unless it runs inside
   B for more than about a fifth of the hex width.
3. A line clipping less than about a fifth of a hex's width does not enter it. Near a vertex, judge by how
   far the line runs INSIDE each hex; the draft's presence (0.10 or more = a real stretch) beats a crop.
4. A junction is in the hex where the branch point is printed, even beside a city.
5. A line running ALONG a hexside never makes that hexside a step. Record the chain of hexes on ONE side (the
   side its continuation enters) and an unclear entry with both options in the note. <Worked example.>
6. A river along a hexside is on it even when it wobbles 10-15 px into a hex. A river cutting a hex from
   vertex to vertex: the two hexsides closest to its course. Check the third side where a river wraps a hex.
7. Road and rail are separate kinds.
8. Follow every chain (river, railway, road) to where the print ends it, including off the map edge.
9. Hard call: crop.py --side A-B --radius 0.8 --scale 3 --grid, or --hex ID. Still undecidable: leave it out
   and add {"at": ONE token, "feature", "note": what you see and the options}. Delete resolved draft unclear
   entries and write the resolution in "notes".
WORKED EXAMPLES: <paths>; compare each with its tiles before starting.
OUTPUT: the JSON files, then one line per tile ("T: changes vs draft, unclear left N") and systematic problems.
```

### 5c. Running readers (the rules that survived a connection drop and a usage limit)
- Launch reader helpers with at most TWO running at once, 8 tiles each, each told to skip tiles already read
  and to write each record the moment the tile is done.
- Before launching and after every batch, a drop or a limit: `python merge.py MAP --status` (complete, bad,
  missing). Relaunch only missing tiles; delete and re-read a bad record. Write the list of complete tiles to
  the task file after every batch. On a usage limit stop cleanly with that list current.
- Spot-check the first record of a new reader (diff it against its draft). If a pattern is wrong, correct the
  prompt file and message the running readers.
- On PGG all four first helpers died together in one connection drop with 6 of 60 tiles on disk; the
  relaunch died on a usage limit with none. Small batches and per-tile writes lost nothing after that.

### 5d. Edge markers (20 minutes, done by the worker, not a tile reader)
Tiles do not reach the margin. Crop the four edge strips with the grid (`crop.py MAP primary edges/west-0.jpg
--box 0 0 420 780 --grid --scale 1.5`, and so on round the map; zoom small boxes to 3x) and write
`work/MAP/catalogue/markers.json`: a list of `{"kind": "entrance"|"vp"|"line"|"other", "text", "hexes",
"label_xy": [x, y] in source pixels, "note"}`. A crop pixel converts to source as `x0 + px / scale`; a hex
centre is `ox + (c-1) * 1.5 * size` across and `oy + (r-1) * sqrt(3) * size` (+ half a row in shifted
columns) down, for flat grids. PGG: entrance brackets and boxes on every edge, two of them blue lines inside
the first column; an entrance box printed inside an edge hex; victory-point texts on two hexes with no city.

## Stage 6 -- merge (the agreement gate)
- While readers run: `python merge.py MAP --partial --out SCRATCH` after every batch, then settle its open
  items at once. Merging batch by batch keeps each look to two or three crops.
- `work/MAP/catalogue/disagreements.json` lists overlap disagreements (the same hex or hexside read differently
  in two tiles, where a tile whose area holds both hexes and does not list a hexside reads "absent") and every
  reader `unclear` entry still open.
- Settle each with a focused crop (`crop.py ... --side A-B` or `--hex ID`, into `work/MAP/look/res/`) and the
  numbers in `candidates.json` (coverage, presence), and add a resolution to
  `work/MAP/catalogue/resolutions.json`: `{"at", "feature", "value" (terrain id; place or null; true or false
  for a hexside), "reason": what the look showed, "look": the crop, "exception": true for a named exception}`.
  A reader question whose `at` is not one token is closed by a resolution with the same verbatim `at` and
  `"value": null`.
- A resolution may also add a hexside no tile read, when the look shows the print needs it (PGG 1008-1109,
  a river cutting inside a hex that both overlapping readers had skipped).
- Named exceptions are decisions the print cannot settle, with the reason: on PGG a road or railway running
  exactly along a hexside (the chain is drawn on one side of it) and a railway leaving the map at a corner.
GATE: `python merge.py MAP` exits 0: every tile read, every hex covered, no open item.
Checks after the gate, against `candidates.json` (an agreement between overlapping readers can be a shared
mistake, which the gate cannot see), each followed by a look at every hex or hexside it lists:
- hexsides with river "along" >= 0.8 (lake edges left out) that the catalogue lacks: readers delete correct
  candidates as "wobbles" or "vertex passes" (PGG: six, all real, each breaking a printed river);
- lake hexes undercounted: lake fill >= 0.2 and fill + river colour >= 0.45 but not lake (the outline is lake);
- woods under links: woods < 0.5 but woods + railway + road >= 0.6 (a line crossing the blob hides woods; a line
  beside it does not);
- one-hex symbols: blob share and cover at the terrain rule's thresholds, measured with the same tool
  (`candidates.json`), with every hex within 0.1 of a threshold looked at;
- link chain ends that stop in a hex holding the other link kind (a road beside a railway loses steps), and every
  other railway end that is not a city or a map-edge exit (a branch whose junction lies just past a hexside);
- pairs of neighbouring road ends: where roads stop at dots on both sides of a river, the shared hexside must be
  a river; if the catalogue has none there, the river is misread (PGG 0911: a river cutting a hex corner to
  corner, measured "along" only 0.25-0.35, invisible to the threshold check);
- every `network_check.py` "does not drain" or "short" river piece: treat it as a reading gap until crops at both
  ends of the piece show a rounded printed end. On PGG all twelve such pieces first taken for printed sources were
  gaps (deleted candidates, a wrong-side substitution, rivers cutting through hexes); none is left as an exception.
Patterns on PGG (35 resolutions over 96 records): most disagreements were one reader following a chain to its
end and the other stopping at its area's edge; borderline woods; readers recording a road that runs along
hexsides as crossings of them (caught only by the overlap; a mask check cannot see it, because the black grid
line breaks the grey road in the mask); a line through a column of hex corners (settled by presence numbers,
not by crops, which jitter).

## Stage 7 -- assemble
`python assemble.py MAP` writes the sheet from `catalogue.json`, `markers.json` and the kept parts of the
existing sheet, and validates it against hexsheet.xsd. It carries the existing `<sheet>` root attributes and
writes `urban` from the config (it throws without it). Terrain goes in `<hexes>` lists, places as `<hex>` with
a glyph and a name label on the city's first hex, hexside lines as `<edge>`, link steps as `<link>` chains
split at every end and junction. A step that leaves the map (`HEX:DIR`) has no `<link>` form and is listed in
an XML comment. Dry run on the drafts at any time: `python merge.py MAP --records work/MAP/catalogue/draft --out
SCRATCH` then `python assemble.py MAP --catalogue SCRATCH/catalogue.json --out SCRATCH/dry.xml` (on PGG this
caught a naming bug before the catalogue existed).

## Stage 8 -- render
From `map_graphics/xml`: `python hexsheet2svg.py SHEET.xml --png --scale 1` (Inkscape; 3 s on PGG; must report
0 warnings). Then `python tile.py MAP render` cuts the render PNG into `work/MAP/tiles/render/` in exactly the
scan tiles' boxes.

## Stage 9 -- verify
Verify readers (Sonnet, two at a time, 8 tiles each, same resume rules as stage 5c) compare
`tiles/scan/T.jpg` with `tiles/render/T.jpg` and write `work/MAP/verify/ROUND/T.json` (`{"tile", "reader",
"differences": [{"at", "feature", "print", "render"}]}`), from a verify prompt written like the reader prompt:
- The render is schematic: a whole-hex terrain fill, links centre to centre, rounded rivers. Only these are
  differences: a terrain the half rule contradicts; a hex with or without a city or town, or of the wrong kind
  or name; a river on a hexside on one side only; a railway or road crossing a hexside on one side only.
- How a city is DRAWN is a deliberate style (with `urban="buildings"` a pale grey hex with a few black
  buildings against the print's many blocks): not a difference. Neither are label positions, colours, margin
  furniture, the render's short stub where a line leaves the map, or named exceptions.
The verify prompt, written like the reader prompt (the pilot's is `examples/pgg/verify-prompt.txt`):

```text
You are a map VERIFIER in the image2sheet process. A printed hex map was catalogued into JSON and rendered
as a new sheet. You compare the render with the scan, tile by tile, and list every difference. Check the
render against the IMAGE, never against the catalogue.
MAP: <title>. <Flat|Pointy>-topped hexes, ids "<format>". Folder: <this folder>. Python: <path>.
HARD RULES: never Read any image except the tile JPEGs named below and crops you make with crop.py (under
300 KB); edit no file except your own records; no git; use the Write tool.
FOR EACH TILE T: look at work/MAP/tiles/scan/T.jpg (the scan with our overlay) and work/MAP/tiles/render/T.jpg
(the render cut in the same box, same overlay); write work/MAP/verify/ROUND/T.json:
  {"tile": T, "reader": "sonnet", "differences": [{"at": "0419-0420", "feature": "rail",
   "print": "railway crosses from 0419 into 0420", "render": "no railway"}]}
An empty list means the tile matches. ROUND is given in your task. Write each record as its tile is done.
READING THE OVERLAY: our id is magenta (core) or orange (margin) on <side>; the printed id is on the other
side, on both images. Name a hex by its id, a hexside or step by its two hexes "A-B", a map-edge hexside
"HEX:DIR". Check every hex with a magenta or orange id.
WHAT TO COMPARE (the render is schematic: that alone is never a difference)
- terrain: the render fills a whole hex with one terrain; the print draws blobs. A difference only when the
  render's terrain disagrees with the terrain rule on the print. EYE ESTIMATES OF COVER RUN HIGH: for any
  blob near the threshold, read the MEASURED cover in work/MAP/candidates.json ("hexes" -> HEX -> mask, with
  "<mask>_blob", 1.0 = a symbol drawn wholly inside the hex), and report only if that measure disagrees or
  the mask plainly missed the blob. Hexes already looked at are in work/MAP/verify/exceptions.json.
- place: WHETHER a hex holds a city or town, its kind and its printed name. HOW the renderer draws it
  (<the sheet's urban style>) is a deliberate style, not a difference; neither is where a label sits.
- hexside lines: a printed river on a hexside the render leaves empty, or the reverse.
- links: a hexside the printed line crosses that the render's line does not, or the reverse. A line through
  a vertex counts as entering the hex it continues in.
- Ignore: line positions inside a hex, fonts, colours, the render's own ids, margin furniture, <this map's
  printed furniture>, and the render's short stub where a line leaves the map.
- Entries with "exception": true in resolutions.json are decided on purpose; do not report them again.
Unsure: crop both images at the same place, crop.py MAP primary ... --side A-B --radius 0.8 --scale 3 --grid
and crop.py MAP render ... the same. OUTPUT: the JSON files, then one line per tile ("T: N differences") and
any systematic problem.
```

Spot-check the first verify record as you would a reader's: on PGG it exposed the one-hex woods symbols (stage
4). Verifiers estimate cover by eye and run high (55-100 per cent for hexes measured at 0.42-0.44), so the verify
prompt points them at the measured cover in `candidates.json` (an image measurement, not the catalogue) for any
blob near half a hex, and at `verify/exceptions.json` for borderline hexes already looked at: each borderline
hex gets one focused look and one named exception. On PGG every difference in the first 16 tiles was terrain. Re-assemble the XML whenever a resolution lands, but render and re-tile only while no verifier is running:
a re-render rewrites the render tiles they are reading. `merge.py` refuses two resolutions for the same hex or
hexside and feature, so a superseded resolution must be removed, not shadowed.
`python verify.py MAP ROUND` collects the round, drops `work/MAP/verify/exceptions.json` entries, merges
duplicates and gates. Every difference goes back into the catalogue as a resolution (with a focused look at
the scan), then the sheet is re-assembled and re-rendered between batches.
Round 2 is focused, not a second pass over whole tiles: `python contact.py MAP work/MAP/verify/r2 --per-sheet 6`
puts the scan and the new render side by side for every resolved place (captioned with the decided value) on
contact sheets under 300 KB, and one reader looks through them for any pair that still disagrees. On PGG 99
places fit on 17 sheets (about 40k tokens, against an estimated 1.3M to re-verify the 63 touched tiles); any
disagreement becomes a new resolution and a new contact sheet. `verify.py` counts a round-1 difference at a
place that has a resolution as settled (the focused round re-checks it), so the gate reads the state after the
resolutions, not the state the verifier saw. GATE: round 1 has a record for every tile, and no difference is left
except named exceptions after the focused round.
Terrain audit (when a person says "some terrain is wrong" without naming hexes): first compare every hex's
catalogue terrain with the measured rule from `candidates.json`; then list every hex within reach of a threshold
(woods 0.29-0.65, a one-hex blob share of 0.4 or more, lake and swamp edges) in a JSON list and run
`python contact.py MAP work/MAP/verify/terrain --list LIST.json --terrain --per-sheet 8 --radius 0.75 --scale 0.8`:
the caption carries the catalogue terrain and the measured woods, blob share and cover, lake, swamp, rail and road.
Read every sheet and correct by resolution. On PGG, 282 hexes on 36 sheets gave 0 corrections: the doubtful
hexes were all within 0.05 of a threshold, a rule choice for the person to make, so list them in the task file
and ask for hex names rather than moving thresholds.

LESSON (PGG follow-up): calibrate terrain thresholds on a person's labelled examples, never on an untested "at
least half" rule. The audit above passed because it checked the sheet against its own rule. Ben then named 13
hexes (two water, eleven forest) that the print shows as that terrain at measured shares of 0.27-0.49 woods and
0.31-0.34 lake, and all 13 were clear. The procedure:
1. Get labelled hexes from the person at stage 0 (step 5), before any threshold is set: several per terrain,
   including the faintest they would still call that terrain, and a few clear hexes. `check_config.py`
   reports a threshold left at exactly 0.5 as an error, because that is this mistake written down.
2. Print their measured shares from `candidates.json`. Before blaming the threshold, take a colour census of the
   pixels no mask holds inside those hexes. On PGG it was paper and river only, so the masks were right. The one
   mask error was a light-blue compass rose counted as lake, fixed by opening the lake mask 9 px.
3. Sweep the threshold, counting the hexes that flip at each value, and put it in a gap of the measured
   distribution just under the lowest labelled example. On PGG that was 0.24 for both woods (5808 at 0.229, 3012
   at 0.253, Ben's lowest 0.266) and lake (0503 at 0.228, 0502 at 0.252).
4. Put a hatch or symbol rule (swamp) ahead of the blob-colour rules its strokes partly match.
5. Look at every flip on contact sheets (`contact.py --list --terrain`), before and after the render, and write
   the flips as resolutions. Then run `network_check`: a new lake hex can cut a printed road (PGG 0703, kept clear
   as a named exception) or leave a river on a lake shore (1602, named exceptions).
On PGG this moved 119 hexes clear -> woods and 4 clear -> lake (5 before 0703 was kept clear for its road), and
nothing the other way. Record the numbers in `work/MAP/threshold/`.

## Stage 10 -- report
In the task file: residuals, tile count, disagreements and resolutions, differences per verify round, feature
counts, the named exceptions, before/after crops for known errors, and places for a person to spot-check
(borderline terrain, named exceptions). `tools/network_check.py SHEET.xml` runs as a report only: a
disconnected piece, a stub or a river source is a question ("is this printed?"), never a reason to change the
sheet; the map's profile names the printed exceptions.
Golden scripts that no longer replay on the new sheet need new hexes or paths. Search for them with the engine's
own movement model (for PGG `games/pgg/engine/PggTerrain.cpp` and `PggMovement.cpp`: class-specific woods cost,
road steps, river cost per side, column limits, half allowance beyond a leader's radius, the full-allowance march
of turn 1, and the entrance-area table), not with a guess; `hexgames_cli --record` names only the first refusal
per run. After re-recording, check each golden still shows the event it exists for (on PGG the supply golden's
German division was first moved to a hex that is now supplied, and its "isolated" event vanished silently).

## Other map kinds
- Pointy-topped grids: the scripts use the renderer's geometry, so calibrate, tile, candidates and merge work
  unchanged; directions are `ne e se sw w nw`, and shifted ROWS replace shifted columns in the reader prompt.
  The printed id may sit at the hex's top or side: set `id-side` so our label goes opposite it.
- Two grids on one sheet (Dai Senso): the configuration and `common.make_grid` handle one grid today. Extend
  both to a list of grids (each with its own control points and fit) and name hexsides across the seam from
  the network_check model before running such a map; the stages after calibration then read tile by tile per
  grid.
- Very large maps: keep tiles at 4 x 4 core hexes; batch readers by tile row.

Copyright Ben Paul Wise. All Rights Reserved.
