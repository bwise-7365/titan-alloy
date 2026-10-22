Copyright Ben Paul Wise. All Rights Reserved.

# Test list: the grammar-first map reader (design contract D3, 2026-09-18)

ctest labels: `reader` (fast, synthetic inputs) and `reader-long` (the example maps). Fixtures are
sheets rendered by hexsheet2svg.py, so every fast test has an exact expected structure and no model.

## lattice.py
- L1 synthetic pointy: render a fixture sheet at three scales; recovered size, ox, oy within 0.02 hex;
  orientation pointy; rotation under 0.2 degrees.
- L2 synthetic flat: same for a flat sheet.
- L3 rotated render by 1.5 degrees: rotation reported within 0.2 degrees; size within 0.02 hex.
- L4 extent: a sheet with a clip; printed set equals the sheet's hexes; clip_index equals its clip.
- L5 furniture: a panel drawn over live hexes; those hexes fall out of printed.
- L6 numbering: two anchors fix id-format, starts and steps for each of the four printed conventions
  in the fixtures (col-row, row-col, lettered rows, reversed columns); a wrong anchor fails `passed`.
- L7 (long) the eighteen example maps: a report table; SMW and PGG within 0.05 hex of their
  image2sheet calibrations; Olympic reports `piecewise` or fails with a reason, never a silent fit.
- L8 determinism: two runs, byte-identical lattice.json.

## legend.py
- G1 synthetic: a rendered sheet with a legend panel of its own terrains, lines and marks; the panel is
  found, one swatch per declaration, each swatch's dominant colour equals the palette value.
- G2 missing legend: vocabulary rows carry `from` the library and the report lists them.
- G3 (long) the thirteen example maps with a legend: swatch count equals a count by eye recorded in
  `example maps/notes.txt`.

## cells.py
- C1 round trip on fixtures: scores for every true feature above 0.8 and for every absent feature below
  0.2, all address kinds, pointy and flat.
- C2 confusers: a fixture with hex ids printed, labels across hexsides and city blocks at centres; no
  false along, across or glyph score above 0.5.
- C3 coincident lines: a river and a border on one hexside both score above 0.8.
- C4 rings and vertex marks on a fixture that declares them.
- C5 masking: the printed id patch is masked; a fixture with ids on the west side scores the same
  along-strips as one with ids off.

## structure.py
- S1 hysteresis never invents: synthetic scores with a one-hexside gap of zero evidence between two
  strong runs give two chains and one doubt, never one chain.
- S2 hysteresis joins: a gap scored 0.45 between two runs scored 0.9 joins at low threshold 0.4.
- S3 end reasons: ends at the edge, at a place, at a junction, at a river bank classified from cells.
- S4 regions: membership from tint; the derived outline equals the fixture's region outline.
- S5 doubts: every score in the middle band becomes exactly one doubt with a look path.
- S6 chain files pass chain_check.py with zero non-adjacent pairs on every fixture.

## compare.py: the three rendering tests
- R1 round trip on every fixture and, long, on every example map read so far: structure identical.
- R2 swap: every (structure, style) pair in the library validates and renders with zero warnings.
- R3 common style: every structure under the neutral style shows every declared kind (counted in the
  SVG by class).

## Cost ledger
- K1 every long run writes looks and tokens per map to the task file; a map over ten looks fails the
  run with the reason, which is the stop rule.

Copyright Ben Paul Wise. All Rights Reserved.
