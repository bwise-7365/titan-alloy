Copyright Ben Paul Wise. All Rights Reserved.

# Plan: design, build and test the grammar-first map reader

Written 2026-09-18 for Ben's review. Companion to `2026-09-18-hexmap-reading-primitives.md` (what the
primitives are and how each is measured) and `2026-09-17-map-reading-efficiency.md` (why the tile and
chain processes cost what they did). Nothing here is started.

## 1. Principles the plan must keep

- The grammar drives the loop: cells are enumerated from the lattice, never found in the picture.
- Structure and style are separate documents (task 14, mechanism A); the reader writes structure and
  derives style from the legend; the rendering-swap test (primitives doc, section 10) is the proof.
- Model use is bounded and named: one look to name legend swatches per map, contact sheets of doubts.
  Nothing else. The ledger records every look.
- Priors rank, never invent. A cell with no evidence is never written.
- A person finishes the map in the editor (task 16). The reader's job is to make that short.
- One worker per stage, no sub-agents, every stage a separate launch, as task 15 set out; the difference
  is that the stages are now script runs with a cap of a few looks, not reading sessions.

## 2. Design phase (documents and contracts, no tools)

D1. Structure model on disk: the cell-score file (one row per cell per kind), the chain and region files
    (chain_check.py and chain2catalogue.py already read the chain form), the doubts file, the style
    document derived from the legend. Written as schemas in `map_graphics/xml/tools/reader/README.md`
    with one worked example per file.
D2. XSD proposals from the primitives doc, section 8, in PLAN.md's review list. Ben decides which land
    before the tools; the vertex mark and the end reasons are the two the reader itself needs.
D3. Test lists (`doc/.../test-lists/reader.md`): the three rendering tests, the oracles in section 5
    below, and the lattice gate. UML for the reader's modules in `uml/reader.puml`.
D4. The style library's shape: a directory of swatch crops per house and kind, with a JSON index, so
    that a map with no legend borrows rows and the document records the borrowing.

Gate: Ben approves D1 to D4. Cost: a day of writing, no model looks.

## 3. Build phase, in five tools

Each tool is a script in the existing style, headless, with a `--report` mode that prints what it
measured so a person can check it without images, and a ctest entry that runs it on the fixture maps.

B1. `lattice.py`: period, orientation and phase from autocorrelation; extent from periodic energy;
    furniture rectangles; two printed ids to fix numbering; a piecewise mode anchored by counted hexes
    for photographs. Writes the grid element and clip. Output for a person: one whole-map overlay.
B2. `legend.py`: find the panel by its swatch rhythm, cut the swatches, hand one crop to a model or a
    person to name them, write the vocabulary and the style document. Missing legend: borrow from the
    library and say so.
B3. `cells.py`: cut every strip and patch, score against the swatches, write the cell-score file. Pure
    CPU. Masks the printed hex id before measuring.
B4. `structure.py`: terrain clusters labelled from the legend, chains by hysteresis with end reasons,
    regions by membership, glyphs and rings, the doubts file. Writes the chain files the checker reads,
    then assemble.py runs unchanged.
B5. `compare.py` extended: render-to-scan difference image, render-to-render structure diff, and the
    round-trip test driver.

The editor (task 16) is built alongside from B3 on, since the doubts file is its input.

## 4. Map order: cheap to debug first, largest last

The order is chosen so that each map adds one new difficulty and comes with an oracle, an independent
answer the reader's output can be checked against without spending looks.

| step | map | hexes | new difficulty | oracle |
|---|---|---|---|---|
| 1 | Battle for Moscow, both renderings | about 180 | no ids, no legend, thick clean lines, blobs clipped to hexes | the two renderings must read to the SAME structure; the first swap test for free |
| 2 | Target Leningrad | about 220 | ids in every hex, grid only over land (clip), river bands, standard symbology | Ben's eye; small enough to check whole |
| 3 | Siege of Tallinn | about 600 | three link kinds, ports, lakes, marsh blobs | Ben's eye; a clean map where the link separator is proved |
| 4 | Stalin Moves West | 319 | legend with a TEC, thin rail, two border dash styles, bridges, irregular outline | two earlier readings, Ben's corrections, the doubts already recorded |
| 5 | Red Dragon Blue Dragon | about 450 | no legend again, double-line links, furniture for two sides | Ben's eye |
| 6 | Panzergruppe Guderian | 1829 | the only FLAT grid, off-centre straight rail, blobs ignoring hexes, terminus dots, labelled terrain thresholds | the existing sheet and its network profile; Ben's 1006 swamp ruling |
| 7 | Tannenberg | about 1200 | two rail kinds, blocked hexsides, rings, off-map numbered exits, trenches, full legend | legend is complete, so every kind is declared before reading |
| 8 | Invasion of Russia, both editions | about 500 | haze; borders as coloured bands; bridges as bars | two editions must read to the same structure |
| 9 | Downfall | about 1500 | coincident border bands, objective rings, sea areas, abstract links, tracks over hexes | Ben's eye; the region-membership method is proved here |
| 10 | D-Day at Tarawa | 1050 | side glyphs in thousands, colour as data, irregular outline, 25 px hexes | the existing sheet; decide first whether to rescan |
| 11 | Russian Campaign | about 1200 | rule-bearing coast line, districts, low resolution | the existing sheet |
| 12 | Borodino | about 900 | rotated print, no ids, redoubt side glyphs, low resolution | Ben's eye |
| 13 | Olympic, wrinkled photo | about 1000 | piecewise lattice from marked cells; letter codes | Ben's three marked cells; lattice only, features optional |
| 14 | Arctic Disaster | about 1500 | vertex marks, grid invisible over land, ice out of play, boxes over hexes | Ben's eye |
| 15 | Bagration Stopped | about 1800 | dense road mesh in two weights, front line, tiny ids | Ben's eye; the hysteresis chain builder under load |
| 16 | Totaler Krieg | about 2500 | two scans spliced by ids, DS symbology | the splice must agree on every shared id |
| 17 | Dai Senso | 2968 | two grids, six border kinds, four glyphs a hex, boxes over hexes | the existing sheet and its profile |

Steps 1 to 5 are small maps; every one can be checked whole by eye in minutes and each proves one tool.
Nothing on steps 6 onward is attempted until the round-trip test passes on all of steps 1 to 5. Dai Senso
and Totaler Krieg come last because they add nothing new except size and the seam, and size is exactly
what the method is supposed to make free.

## 5. Tests and gates

- **Lattice gate**: two printed ids agree with the fitted numbering; Ben's overlay look. Runs on all
  eighteen overviews as one batch before any cell is scored, since a lattice failure is cheap to see and
  expensive to discover later.
- **Round trip** on every map at every build: read, render, read back, identical structure.
- **Swap and common style** over all pairs, as ctest entries once more than two styles exist.
- **Oracles** per map as in section 4: a second rendering, an existing sheet, or Ben's eye on a
  difference image. A disagreement with an oracle is a doubt with evidence, never a silent choice.
- **Structural checks**: chain_check.py and network_check.py, report and ends-explained gate.
- **Determinism**: same inputs, byte-identical structure; goldens are structure files, so a rendering
  change never moves a golden.
- **Cost ledger**: looks and tokens per map recorded in the task file, with the caps: one look for the
  legend, at most one contact sheet per thirty doubts.

## 6. Cost and time, estimated

| phase | Ben's time | model looks | wall clock |
|---|---|---|---|
| Design D1 to D4 | one review | 0 | 1 day |
| Build B1 to B5 | four reviews at the gates | 0 | 4 to 6 days of script work |
| Maps 1 to 5 | five eye checks | about 10 | 1 day |
| Maps 6 to 12 | seven eye checks, PGG labels | about 30 | 2 days |
| Maps 13 to 17 | five eye checks | about 40 | 2 days, mostly the seam and the photo |

Model cost for the whole set is on the order of a few hundred thousand tokens in total, dominated by
legend naming and doubt sheets. If any map exceeds ten looks the plan stops and the reason is written
down before continuing; that is the same stop rule task 15 used, now cheap enough to obey.

## 7. What is deliberately not in this plan

- No tile reading, no reader prompts, no per-map colour thresholds set by hand.
- No image warping or retouching; the wrinkled photo is met by counting hexes, not by flattening.
- No pixel-perfect reproduction anywhere; the difference image is a list of places to look, not a score.
- No second model reading as a cross-check; oracles are renderings, sheets and Ben.

Copyright Ben Paul Wise. All Rights Reserved.
