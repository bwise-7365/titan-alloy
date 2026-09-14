Copyright Ben Paul Wise. All Rights Reserved.

# Task 12: an accurate PGG sheet by the image2sheet process (milestone M6e)

status: in-progress
worker: W5 (opus; Ben chose Opus for this pilot)   started: 2026-09-14
resume: stages 1-4 done (calibrate gate passed; 120 scan tiles, drafts and candidate tiles); stage 5: worked
  examples read/0117_0420.json and read/2117_2420.json written; reader prompt work/pgg/catalogue/reader-prompt.txt;
  first helpers died in a connection drop (8 complete records: 0101_0404 0501_0804 0105_0408 0505_0808 0109_0412
  0113_0416 0117_0420 2117_2420); relaunch also stopped (usage limit) with nothing written; 19:50 two helpers
  running, batch A 0901_1204..3701_4004 and batch B 0905_1208..3705_4008 (8 tiles each); every other tile is
  still to read; edge-marker pass done (catalogue/markers.json);
  merge.py, assemble.py, verify.py and verify prompt written, not yet run on a full catalogue;
  next: launch rows 5-8 when 1-4 finish, then merge; a relaunch reads only tiles with no read/*.json
inputs:
  hexgames/PLAN.md "Terms", RESUME HERE, decision log 2026-09-14 "Building sheets from images" and the M6e line;
    CLAUDE.md
  hexgames/map_graphics/xml/tools/image2sheet/README.md   (THE PROCESS: follow it; improve it where it is wrong or
                                                            vague, and say so in this file)
  hexgames/map_graphics/xml/hexsheet.xsd, README.md, hexsheet2svg.py, tools/network_check.py, tools/tidy_networks.py
    (tidy_networks.py is NOT used on PGG any more; read it only to see what it did wrong)
  hexgames/map_graphics/xml/panzergruppe-guderian.xml       (the sheet to rebuild; keep palette, terrains, lines,
                                                              labels and panels unless the print shows them wrong)
  hexgames/game_rules/panzergruppe-guderian.md, game_rules/xml/panzergruppe-guderian.xml  (legend meanings, place names)
  images in C:\Library\War-Games\Panzergruppe Guderian (READ ONLY THROUGH TILES OR CROPS OF AT MOST ~300 KB):
    primary: "PGG - Cyrillic.png" 5615 x 3727 (the current sheet's pixel frame; "PGG map, Russian.png" and
      "Panzergruppe Guderian map Russian redesign.png" have the same size and are probably the same map)
    secondary: "PGG map, English.png" 4838 x 3338, "PGG - Roman.png" 3591 x 2410, "Panzergruppe Guderian map
      original.jpg" 1900 x 1275, "PGG - low res.png" 1829 x 1174 (for anything hidden or unclear on the primary)
  for the ripple only: games/pgg/ (engine facts and data gaps, tests, scenario, goldens), game_records/xml/pgg-test.xml,
    packages/xml/pgg.package.xml, tools/HexGamesCli.cpp (to record goldens)
outputs:
  hexgames/map_graphics/xml/tools/image2sheet/*.py   the stage scripts of the process, general (no PGG special cases)
  hexgames/map_graphics/xml/tools/image2sheet/maps/pgg.json   the PGG configuration (sources, grid, control points,
    vocabulary, candidates, kept sheet parts)
  hexgames/map_graphics/xml/panzergruppe-guderian.xml, .svg, .png   rebuilt by assemble and render
  the process README updated with anything the first run taught (so the next map runs better)
  network_check.py's PGG profile: the printed exceptions named; it reports, it does not drive changes
  the ripple: games/pgg facts (PggFacts data gaps and provisional entrance areas replaced by what the print
    shows), tests that pinned the old sheet, scenario and pgg-test.xml hex ids if they change, and the PGG
    goldens RE-RECORDED with hexgames_cli --record, each diff summarised; commit reason "maps: PGG accuracy"
acceptance:
  calibrate: every control-point residual under 0.1 hex; the printed extent (including columns 57-59) inside the grid
  every tile catalogued; every disagreement and "unclear" resolved or a named exception with its reason
  verify: no remaining render-versus-scan differences except named exceptions
  Ben's known errors fixed and shown in the report with before/after crops: Orsha (ОРША) as a junction of four
    railways and three rivers; columns 57-59 present; the rivers running down around columns 48/49 and 57/58 with
    their twists and angled stretches; a road reaching hex 0120 if printed; rail to the south edge if printed;
    Victory Point hexes 5907 and 5915 on the sheet; the entrance areas as printed
  python tools/validate-xml.py map_graphics/xml packages/xml game_records/xml -> all valid; hexsheet2svg.py renders
    with no warnings; tools/banner-check.py . -> 0 failures
  ctest --preset win-msvc-debug 100% with CLion's ctest.exe (PGG goldens re-recorded; TRC goldens byte-identical)
  no file touched outside the outputs and the ripple; no .xsd edited (proposals only)
log:
- 2026-09-14 created by the coordinator after Ben accepted the new method; not launched

## Model
The process checks itself twice (tile overlaps, render versus scan), so a Sonnet worker is enough for the
catalogue and verify passes. This first run also writes the process's scripts and settles its details,
which future maps inherit; the coordinator recommends Opus for this pilot and Sonnet for later maps that
reuse the finished process, sending only flagged tiles to a stronger model. Ben chooses.

## Process notes for Sonnet (worker fills in AS IT GOES, not at the end)
Ben's goal for this pilot: afterwards, a Sonnet worker must be able to run the process on TRC, Dai Senso, DDaT
and out-of-sample maps from map_graphics/xml/tools/image2sheet/README.md alone. Record here, stage by stage:
the exact commands run; how control points were found and checked; the tile-reading instructions actually
given to readers (the prompt text, the vocabulary, examples of hard calls and how they were settled); every
pitfall and false start, with the rule that avoids it; what each gate caught; how long each stage took and
roughly what it cost; what should be different on a pointy-topped or two-grid map. At the end, fold these
notes into the README (commands, reader prompts, decision rules, pitfalls) so it stands alone, and say here
which notes went where.

Working folder: `hexgames/map_graphics/xml/tools/image2sheet` (all commands below run from there, with
`python` = `C:/Users/bwise/AppData/Local/Python/bin/python.exe`; needs lxml, numpy, scipy, Pillow; PyMuPDF optional).

### Stage 0 -- look before configuring (about 20 minutes)
- Make a whole-map overview under 300 KB first (resize to 1300 px wide, JPEG quality 55; quality 70 at 1400 px
  was 330 KB, too big) and 700 x 700 px full-resolution crops of the four corners and one dense place.
  From these alone, settle: orientation (PGG: flat), which columns are shifted (PGG: even-numbered printed
  columns, i.e. odd column INDEX, offset "odd"), where the printed id sits in its hex (PGG: inside the hex,
  near its BOTTOM edge, so id-side "s"; the old sheet said "n"), the printed extent (PGG: columns 01-59,
  rows 01-31; the old sheet stopped at column 56), and how each feature is drawn.
- PITFALL (the reason for the old sheet's errors): on PGG the railways and roads are straight lines that
  cross hexes off-centre, and rivers wobble along hexsides with rounded bends. A tool that samples hex
  centres or exact hexside lines cannot read them; a reader must decide which hexes a line passes through.
- Colour census: quantise a 700 px crop to 12-level bins and list the most common colours, then census
  small boxes placed on one feature each (swamp, lake, road, rail, city). PGG: paper (243,250,243), woods
  (138,174,90)+(78,102,54), swamp hatch (15,135,65), lake (5,185,245), river (6,126,150), road
  (105,105,115), rail black (5,5,5) with PURE white dashes (255,255,255, distinct from paper), city blocks
  (65,65,65), margin (195,195,155). Record them in the map's vocabulary.
- Woods and swamp are blobs that do not follow hexes (the 1976 original draws them the same way), so the
  sheet needs a stated rule for a partly covered hex (see stage 5).

### Stage 1 -- calibrate (about 15 minutes, ~40k tokens including the 15 eye-check crops)
1. Approximate centres. For a new map: `python crop.py MAP primary look.jpg --box 0 0 700 700 --ruler 50
   --scale 1` and read hex-centre pixel positions off the red rulers. For PGG the old sheet's grid gave them.
   Pick at least 12 hexes: the four corners, the middle of each edge, the centre, plus a ring between
   (PGG used 0101 3001 5901 0116 3016 5916 0131 3031 5931 1508 4508 1524 4524 2117 5816).
2. `python calibrate.py pgg locate primary 0101 138.1 125.0 3001 2819.9 178.4 ...` (triples ID X Y).
   Each refined centre comes from matching a hex outline to the dark grid-line pixels (grey < line_dark);
   a crop `work/pgg/calibrate/primary-ID.jpg` shows a red cross and the magenta outline.
3. Eye check every crop: the crossed hex must hold the expected number and the magenta outline must lie on
   the printed lines. All 15 passed on PGG, including hexes filled with woods or lake and hexes under a
   city name (2117) or a railway (0131). Score 0.2-0.6 and "printed edges 5-6 of 6" are normal.
4. Paste the printed JSON lines into `sources.primary.control_points`, then `python calibrate.py pgg fit`.
- Already visible at step 2: 0101 and 5901 are 5374 px apart over 58 columns, 92.66 px per column (size
  61.77); the old sheet's 61.65 drifted about 5 px by column 59.
- The fit prints a dense check (every hex centre refined). Look at its worst hex with
  `python crop.py pgg primary look.jpg --hex 3815 --grid --scale 1.5`: on PGG it was a railway drawn along
  a hexside pulling the outline match (0.143 hex), not a grid error. Rule: a single dense outlier with the
  rest under 0.01 is a feature on the grid line; many outliers in one region mean a warped scan (then add
  control points there, or split the source into calibrated pieces).
- Whole run: `calibrate.py fit` takes 6 s; locate 2 s for 15 points.

### Stages 2-3 -- overlay and tile (5 minutes)
- `python overlay.py pgg` -> `work/pgg/overview-primary.jpg` (215 KB), orientation only.
- `python tile.py pgg scan` -> 120 tiles in `work/pgg/tiles/scan/` (4 x 4 core hexes, 1 margin hex, scale
  1.5; 77-232 KB each) and `work/pgg/tiles/manifest.json`. Tile name = first and last core hex
  (`0117_0420` holds columns 01-04, rows 17-20). 3 s.
- Each hex carries TWO ids: the printed one (black, at the bottom of the hex on PGG) and ours (magenta for
  core hexes, orange for margin hexes) on the opposite side (the top). Both name the same hex. A printed id
  just above a hexside belongs to the hex above it; tell readers so.

### Stage 4 -- candidates (tuning took about 40 minutes; a run takes 10 s)
- `python candidates.py pgg [TILE ...]` measures all hexes and hexsides every time (8 s) and writes drafts
  and candidate tiles (`work/pgg/tiles/cand/`) for the named tiles, or all.
- Masks (maps/pgg.json "candidates.colour_masks"): colour within a tolerance, then optional closing/opening/
  growth. What worked on PGG:
  - rail: the PURE white of the dashes (tol 3) grown by 5 px. Paper is not pure white, so nothing else
    matches. A dark-pixel mask does not work: grid lines, text and city blocks are just as dark.
  - road: grey (105,105,115) tol 16, opened 3 (removes anti-aliased grid and casing edges), grown 2 (bridges
    the gap where the black grid line crosses the road).
  - river: (6,126,150) tol 34. Scored per hexside as "along": the share of 20 bins along the hexside that
    hold river pixels within 0.3 x size beside it (a river crossing a hexside fills only 2-3 bins).
    >= 0.8 proposed, 0.45-0.8 "maybe". Lake outlines are the river colour: expect "maybe" strokes round
    lakes and reject them.
  - woods: a list of five mottle colours (tol 18) read a fully wooded hex as 0.76 (PITFALL). A hue rule
    (green - red >= 14, green - blue >= 22, red >= 40; red keeps swamp's dark hatch out) plus a closing of 5
    px is the fix.
  - swamp: the hatch colour (15,135,65) tol 36 covers only 6-12 per cent of a swamp hex; any hex >= 0.03
    is proposed.
- Railway and road steps: runs of the link mask along each hexside segment (+-2 px). A run in the middle
  of the hexside proposes a step; a run within 12 per cent of an end is a "corner" crossing for the reader;
  a run of 6 per cent or less at the very end is ignored (PITFALL: a railway passing beside a vertex
  flagged every hexside that ends there); a run over 60 per cent of the hexside is "along".
- PITFALL: steps from crossings alone zigzag where a line runs near hex corners (Smolensk 2217-2318-2218).
  Fix: a hex must also hold the line inside it ("presence": share of the inset hex covered by the link mask,
  rail >= 0.04, road >= 0.025); a crossing whose hex holds only a corner becomes an "unclear" hint.

### Stage 5a -- worked examples and the reader prompt (about 45 minutes)
- Before launching readers, read two hard tiles yourself and write their records as worked examples:
  a dense junction (PGG: Orsha, 0117_0420) and a two-hex city with vertex passes (Smolensk, 2117_2420).
  Their "notes" arrays show the reasoning (work/pgg/catalogue/read/). Every hard call became a numbered
  decision rule in the reader prompt (work/pgg/catalogue/reader-prompt.txt; folded into the README at the
  end).
- Hard calls settled there: a junction is in the hex holding the branch point (Orsha's NE line branches in
  0419, not 0420); a line through a vertex steps straight to the hex beyond (0120-0219); a corner crossing
  15 px from a hexside end is a real step; a city's name label does not make its hex the city (СМОЛЕНСК
  label over 2117, blocks in 2216 and 2217; the rules and the old sheet said 2117); a road ending at a dot
  on each side of a river is two roads, not a crossing (focused 3x crop settled it).
- Readers: 8 Sonnet agents, one per tile row (15 tiles), 4 at a time, each given only the prompt file path
  and its tile names; each skips tiles already read, so a relaunch is safe.
- Spot-check the FIRST record a reader writes (diff it against its draft with a few lines of Python) before
  the batch gets far. On PGG the first Sonnet record (0105_0408) was right on every line and river, but it
  overrode a measured woods coverage just under 0.5 by eye (0405 -> woods, "~0.55"). PITFALL: eye estimates
  near the threshold disagree with the overlapping tile and flood merge with terrain disagreements. Rule
  (now decision rule 1): between 0.35 and 0.65 the measured coverage decides; override only when the mask
  counted the wrong thing or is off by more than 0.2. The correction went to the running readers by message
  and into the prompt file for later batches.
- PITFALL (2026-09-14, connection drop): all four reader helpers died together when the API connection
  dropped (ECONNRESET); only 6 of their 60 tiles were on disk. Rules: run at most TWO helpers at a time;
  tell each to write a tile's record the moment that tile is done; after any batch (or a drop), count
  complete records with merge.check_record (parses, every area hex has a terrain, every token valid) and
  relaunch helpers with only the missing tiles. A record that fails the check is deleted and re-read.
- PITFALL (same evening, usage limit): the relaunched helpers and this worker stopped on the session usage
  limit before writing a single tile. Limits and drops are the same problem, with the same rules: batches of
  about 8 tiles per helper, at most two helpers at once, a record written per tile as it is finished, and
  after every batch the list of complete tiles written to the task file's resume line. On a limit, stop
  cleanly with the resume line current; the next run checks read/ and relaunches only the missing tiles.
  Check command (prints complete, bad and missing tiles):
  `python -c "import json,os,common as C,merge as M; ..."` (the loop over the manifest calling
  M.check_record, as in the resume procedure of the README).

### Stage 5b -- edge-marker pass (the coordinator/worker, about 20 minutes)
- Tiles do not reach the margin furniture (brackets and boxes outside the grid), so read the four edge
  strips separately: `python crop.py pgg primary work/pgg/edges/west-0.jpg --box 0 0 420 780 --grid
  --scale 1.5` and so on (west and east strips 420 px wide in 780 px steps; north and south strips 340-370 px
  tall in 900 px steps; 24 crops). Convert a crop position to source pixels: x0 + px / scale.
- Map a bracket arrow to a hex from its source pixel: row r of a column has centre y = oy + (r-1) x 106.97
  (+ 53.5 in shifted columns); column c has centre x = ox + (c-1) x 92.64.
- Zoom any small box at 3x (`--box` 120 x 80 px): "W,1" could not be read reliably at 1.5x.
- Record the result in work/pgg/catalogue/markers.json (entrance boxes with their hexes and box position,
  VP texts, printed lines). PGG findings: two entrance areas (C, G) are blue lines INSIDE column 01, the
  others black brackets outside; X is printed inside 5907 with "(20 ПО)"; КАЛУГА is 5921 (the old sheet had
  5619); railways leave the south edge at 0131, 1831, 3131, 3831, near 4331 and 5431.

### Dry run while readers work (5 minutes, cheap)
- Run merge and assemble on the machine drafts into a scratch folder, so the later stages are tested before
  the catalogue exists: `python merge.py pgg --records work/pgg/catalogue/draft --out SCRATCH` then
  `python assemble.py pgg --catalogue SCRATCH/catalogue.json --out SCRATCH/pgg-dry.xml`, render it, and run
  `tools/network_check.py SCRATCH/pgg-dry.xml --quiet`. On PGG this caught a naming bug (readers say
  "river", the catalogue kind is "rivers") before it could hide resolutions. Drafts: 764 open items (mostly
  draft "unclear" terrain), valid XML, render 0 warnings in 3 s, network report 28 items (river sources that do
  not reach water, separate rail pieces running off the south edge, 21 road pieces): hints only.

- The candidate tile draws proposals over the scan: magenta hexside strokes (dashed = maybe), red (rail)
  and yellow (road) centre-to-centre steps, rings for corner crossings, W/S/L terrain letters (lower case or
  ? = borderline).

## Stage results (worker fills in: residuals, tile count, disagreements, differences per round, counts)
- calibrate (2026-09-14): GATE PASSED. 15 control points, all eye-checked. Fit size 61.761, ox 133.39,
  oy 121.34, grid 59 x 31 (old sheet: 61.65 / 138.13 / 125.02, 56 x 31). Residuals: max 0.008 hex (5931,
  0.84 px), worst per region NW 0.007, N 0.000, NE 0.006, W 0.005, C 0.006, E 0.006, SW 0.004, S 0.003,
  SE 0.008. Affine diagnostic: scale x 61.763, y 61.756, rotation 0.0000 deg (the scan is undistorted).
  Extent: all 1829 grid hexes inside the image; every one has at least 3 printed edges; no printed hex
  outline outside the grid. Dense check: 1680 hexes, offset median 0.004, p95 0.009, max 0.143 (3815,
  looked at: see process notes).

## Ripple (worker fills in: facts, tests, scenario, each golden diff)

## Open questions for review (worker fills in)

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
Read this task file and map_graphics/xml/tools/image2sheet/README.md first, and stay within the inputs. Rebuild the
Panzergruppe Guderian sheet so that it matches the printed map hex by hex and hexside by hexside, by following that
process and writing its stage scripts as general tools that later maps (DDaT, TRC, Dai Senso, out-of-sample maps)
will reuse unchanged. Fidelity over plausibility: record what is printed, never invent or delete a feature to satisfy
a check. Geometry first: pass the calibrate gate before reading any feature. Look at images only through tiles or
crops of at most about 300 KB made with PyMuPDF (fitz); never Read a larger image. Record progress in this file after
every stage so a relaunch loses nothing. Run builds and ctest in the FOREGROUND with a long timeout, after checking
that no ninja, cl, link, ctest or cmake process is running. Finish with status: review, the stage results, the
before/after crops for Ben's known errors, the ripple and open questions.

Copyright Ben Paul Wise. All Rights Reserved.
