Copyright Ben Paul Wise. All Rights Reserved.

# The lattice finder's first run over the eighteen example maps

Night of 2026-09-18, for Ben's review in the morning. Tool: `map_graphics/xml/tools/reader/lattice.py`
(B1 of the plan), built against the contract in `tools/reader/README.md`. Nothing else was built; no
schema was touched; no sheet was changed; no agent was launched; no git command was run. Overlays for
your eye are in the session scratchpad under `reader/<map>/overlay.jpg` (magenta = printed cell,
orange = lattice cell with no printed outline), and `reader/batch-report.txt` is the table below.

## What the tool does

1. Edge map of the scan; autocorrelation; every peak that has all six lattice-ring peaks is a candidate.
2. Each candidate's seven-hex outline template is correlated with the edge map; the score is ink on the
   outline MINUS ink inside the hexes, relative to the image mean. A halftone screen, a ruled table, a
   doubled lattice, the root-three second ring and Downfall's decorative sea honeycomb all have ink
   inside; the printed grid has paper inside. On a near tie the coarser lattice wins, since a texture is
   finer than the grid it decorates. A period under 1.5 percent of the short side is never a candidate.
3. Orientation from the ring angle (pointy has a neighbour due east); rotation is the residue.
4. Phase from the template peak; then Ben's rule applied literally: the perfect lattice that minimises
   the RMS deviation from the printed outlines, by refining every cell's centre locally and fitting the
   lattice to all of them by least squares, three times.
5. A smooth per-cell PHASE FIELD absorbs folds and warps: each cell's outline is matched within a few
   pixels, the displacements are median-filtered over the index grid. SMW's scan has a fold across the
   Warsaw row that shifts the two halves by four pixels; the field handles it and no pixel is moved.
6. Printed cells: the intensity profile across each hexside, averaged along it, shows a line as a dip
   or peak at the outline that is absent four pixels to either side; median over six edges; Otsu over all
   cells. Texture, mottle and halftone average out along the edge.

Cost: 3 to 20 seconds per map on the whole image, no model, no looks. The whole set runs in five minutes.

## The table

| map | orientation | spacing px | size px | rotation | ring | fit cells | field px | cells | printed | verdict |
|---|---|---|---|---|---|---|---|---|---|---|
| Arctic Disaster | pointy | 57.05 | 32.94 | -0.02 | 6/6 | 1410 | 4.2 | 2234 | 1070 | passed; overlay not yet checked by eye |
| BFM map 1 | flat | 236.50 | 136.54 | -0.18 | 6/6 | 135 | 6.7 | 216 | 135 | passed; overlay checked, on the lines |
| BFM redone | flat | 108.28 | 62.52 | 0.20 | 6/6 | 199 | 8.1 | 276 | 127 | passed |
| Borodino | flat | 35.75 | 20.64 | 0.02 | 6/6 | 1792 | 4.2 | 2201 | 959 | passed; low resolution |
| Tannenberg | flat | 68.60 | 39.61 | 0.12 | 6/6 | 1563 | 5.7 | 2226 | 931 | passed |
| D-Day at Tarawa | pointy | 43.22 | 24.96 | 0.01 | 6/6 | 1107 | 4.2 | 1360 | 823 | passed; 25 px hexes |
| Dai Senso | pointy | 77.26 | 44.61 | 0.01 | 6/6 | 980 | 7.1 | 3217 | 728 | passed; overlay checked, both sheets on one lattice; sea outlines too faint for the printed test |
| Downfall | pointy | 56.45 | 32.59 | 0.02 | 6/6 | 1079 | 4.2 | 1845 | 578 | passed after the coarser-wins rule; the sea honeycomb texture was the rival |
| Olympic, wrinkled photo | flat | 45.28 | 26.14 | 0.00 | 6/6 | 1457 | 4.2 | 2236 | 733 | passed by the numbers; NOT verified by eye; needs your marked cells as anchors |
| PGG Russian | flat | 106.96 | 61.75 | -0.01 | 6/6 | 1931 | 9.2 | 2232 | 1851 | passed; overlay checked; image2sheet had 61.76 |
| Red Dragon Blue Dragon | pointy | 76.94 | 44.42 | -0.04 | 6/6 | 407 | 5.8 | 813 | 227 | passed |
| Siege of Tallinn | flat | 94.56 | 54.59 | -0.07 | 6/6 | 475 | 7.2 | 567 | 481 | passed; overlay checked, on the lines |
| Stalin Moves West | pointy | 92.40 | 53.35 | -0.08 | 6/6 | 441 | 7.8 | 646 | 301 | passed; overlay checked at full resolution across the fold; image2sheet had 53.29 with a recorded drift |
| Target Leningrad | flat | 66.53 | 38.41 | 0.09 | 6/6 | 196 | 5.0 | 275 | 52 | passed; printed count low, grid drawn only over land and faint |
| Invasion of Russia, map 1 | flat | 70.52 | 40.71 | -0.11 | 6/6 | 118 | 4.2 | 168 | 118 | FAILED the contrast gate (0.89); the 35 px candidate is the more likely lattice on a hazy 938 px image; a person's anchor settles it |
| Invasion of Russia, map 2 | flat | 23.92 | 13.81 | -0.02 | 6/6 | 540 | 3.6 | 640 | 91 | passed; 640 px image, at the limit |
| Russian Campaign 5th | pointy | 35.85 | 20.70 | 0.00 | 6/6 | 1447 | 4.2 | 1739 | 157 | passed; printed count low, faint lines |
| Totaler Krieg east | pointy | 107.98 | 62.34 | 0.01 | 6/6 | 1253 | 9.9 | 1616 | 619 | passed; the two scans agree within 0.2 percent |
| Totaler Krieg west | pointy | 107.81 | 62.24 | 0.07 | 6/6 | 598 | 9.9 | 1622 | 497 | passed |
| Bagration Stopped | flat | 92.10 | 53.18 | -0.07 | 6/6 | 1128 | 3.6 | 1276 | 1103 | passed |

Two of my own beliefs from the survey were wrong and the tool was right: Tallinn and Battle for Moscow
are flat-topped. The overlays settled it in one look each.

## What the run teaches

- The lattice is found by the scan's own periodicity on every map, including the wrinkled photo, the
  two-sheet maps and the low-resolution ones, in seconds. Candidate selection needed four rules, each
  from a real map: all six ring peaks (halftone screens fail it), outline-minus-interior ink (doubled
  and root-three lattices fail it), coarser-wins on a tie (Downfall's texture), and the 1.5 percent
  bound (Dai Senso is at 2.2 percent).
- The printed-cell test is the weak part: right on clean maps (PGG 1851 of about 1829 real hexes plus
  a few furniture cells, Tallinn 481), poor where the grid is faint over sea or drawn only over land
  (Target Leningrad 52, TRC 157, Dai Senso's oceans). Extent will need the legend's sea swatch or a
  person's clip in those cases; the tool must report, which it does.
- Scans are not flat: every map shows a phase field of 4 to 10 px. A single rigid lattice would have
  drifted on all of them; the tile process's calibration drift on SMW was this fold.
- Rotation is under 0.2 degrees on every map except none; the earlier "rotated 1.8 degrees" on Dai
  Senso was a wrong candidate, not the scan.

## What is not done

- Numbering (printed ids) is not read; `numbering` is null in every lattice.json. Two anchors per map
  from a person or one crop are the next step (L6 in the test list).
- The Olympic and Arctic overlays are unchecked by eye; Invasion map 1 fails its gate and needs an anchor.
- No ctest wiring yet; the test list `doc/2026-09-12-design/test-lists/reader-tests.md` names the tests
  (L1 to L8) and the synthetic fixtures they need.
- The other four tools of the plan are contracts only.

## What I would like from you

1. A look at three overlays: Olympic (is the lattice on the photo's grid?), Arctic (are the land hexes,
   which the print barely outlines, placed right?), and Invasion map 1 (which candidate is the grid?).
2. Your go for the design gate (D1 to D4 are written: README contract, test list, UML, and the eight
   schema proposals in PLAN.md) and for B2, the legend reader.


## Second run, 2026-09-19: refit on printed cells only

Ben trimmed Borodino to its main section and asked why it had the worst fit. It had not: the refit and
its slip count ran over every lattice cell, and the 1239 cells lying over Borodino's chart panels and
set-up maps (each with its own hex texture) gave 14 px RMS while the 962 map cells gave 3.8. lattice.py
now runs the printed test on the grown positions first, fits the perfect lattice to printed cells only,
places every other cell at the fit plus its printed neighbours' median residual (confidence 0), and
runs the printed test again on the final positions. batch.py runs the folder. RMS in source pixels and
slips after the last round, whole-sheet figures from the 2026-09-18/19 run beside the new ones:

| map | printed | cells | RMS before | slips before | RMS after | slips after | note |
|---|---|---|---|---|---|---|---|
| Arctic Disaster map | 1373 | 2234 | 6.7 | 11 | 2.6 | 0 |  |
| BFM map 1 | 140 | 214 | 5.1 | 0 | 4.6 | 0 |  |
| BFM map redone | 129 | 276 | 9.8 | 0 | 1.5 | 0 |  |
| Battle of Borodino main section | 977 | 1775 | - | - | 1.4 | 0 | trimmed scan, charts still on the left |
| Battle of Tannenberg game map | 947 | 2226 | 11.6 | 4 | 2.1 | 0 |  |
| D Day at Tarawa map1 | 835 | 1360 | 13.8 | 6 | 1.7 | 0 |  |
| Dai Senso | 513 | 3132 | 42.3 | 24 | 3.0 | 0 | --spacing hint |
| Downfall Map larger | 547 | 1845 | 20.9 | 9 | 2.0 | 0 |  |
| Downfall Map smaller | 605 | 1856 | 15.4 | 19 | 2.5 | 0 |  |
| Olympic map wrinkled and marked | 1708 | 2268 | 11.6 | 10 | 8.8 | 0 | wrinkled photo: the residual is the wrinkle, not a misfit |
| Olympic map wrinkled | 1676 | 2268 | - | - | 8.4 | 0 | wrinkled photo: the residual is the wrinkle, not a misfit |
| Panzergruppe Guderian map Russian redesign compressed | 1865 | 2232 | 4.0 | 0 | 1.5 | 0 |  |
| Red Dragon Blue Dragon map | 234 | 813 | 10.2 | 0 | 1.4 | 0 |  |
| Siege of Tallinn map | 485 | 567 | 4.2 | 0 | 2.0 | 0 |  |
| Stalin Moves West map | 320 | 646 | 16.4 | 1 | 2.0 | 0 |  |
| Target Leningrad map | 137 | 275 | 4.3 | 0 | 1.6 | 0 |  |
| The Invasion of Russia 1812 map 1 | 477 | 640 | 22.2 | 2 | 2.3 | 0 | --spacing hint; gate: phase score under 1.0 for the hinted lattice; fit and overlay are right |
| The Invasion of Russia 1812 map 2 | 135 | 640 | 3.3 | 0 | 2.1 | 0 |  |
| The Russian Campaign 5th map | 174 | 1739 | 5.0 | 4 | 1.3 | 0 |  |
| Totaler Krieg map eastern | 688 | 1616 | 13.0 | 1 | 2.2 | 0 |  |
| Totaler Krieg map western | 769 | 1647 | 34.3 | 83 | 2.8 | 0 | --spacing hint; gate: phase score under 1.0 for the hinted lattice; fit and overlay are right |
| bagration stopped map | 1105 | 1275 | 2.8 | 0 | 1.5 | 0 |  |

Every map now fits its printed cells to 1.3-4.6 px with no slips left, except the two Olympic photographs
at 8.4-8.8 px, which is the paper's wrinkle. The three hinted maps are unchanged in kind: the hint gives the
right lattice (overlays checked for Totaler Krieg west and Invasion map 1), and two of them fail the
phase-score gate. The hints were not a person's measurement: the coordinator took them from the tool's
own output (Dai Senso from the first run's unaided figure, Totaler Krieg west from the east sheet,
Invasion map 1 from the search's rejected second candidate). So the FAILED is informative: it marks the
maps whose period the search does not find unaided, and the gate stays as it is.


## Anchors, 2026-09-19: printed ids on 18 of 22 scans

Printed-id anchors were read by eye from crops of each scan with the lattice index drawn (three cells
per map, more where the grid offset needed it) and recorded in tools/reader/anchors.json, which batch.py
passes to lattice.py --anchor. The numbering code learnt two things on the way. A sheet may pair its
printed columns (pointy) or rows (flat) the other zigzag way from the lattice index, so the numbering
now tries both offsets and both parities, and anchors must include both row parities (pointy) or column
parities (flat): Tarawa's first four anchors were all odd rows, fitted, and left every even row one
column out. And the two anchors that fix the steps must differ in both axes, which BFM redone's bottom
row alone could not give. Tarawa's ids are five pixels tall on the scan; Ben photographed one section
and named a unique symbol, and five symbols matched between photograph and scan numbered the sheet.
PGG prints its id in the lower half of the hex. Dai Senso carries two column numberings on one sheet
(west col = c - 1, east col = c - 28, rows shared); the tool takes one. Without anchors: Borodino and
BFM map 1 (no printed ids), Invasion maps 1 and 2 (unreadable at 938 and 640 px; a detail photograph
would do it), The Russian Campaign (letter columns Y23, AA24: digits only for now). Spot checks on
eight maps at three non-anchor cells each: every drawn id matched the print. The Olympic yellow oval, written
as 2011 in notes.txt, sits at lattice (32, 12), which the red and green ovals make 3211; Ben confirmed
3211 by his own hex count, and it is the third Olympic anchor.

Copyright Ben Paul Wise. All Rights Reserved.
