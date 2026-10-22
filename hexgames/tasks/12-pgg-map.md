Copyright Ben Paul Wise. All Rights Reserved.

# Task 12: an accurate PGG sheet by the image2sheet process (milestone M6e)

status: review
worker: W5 (opus; Ben chose Opus for this pilot)   started: 2026-09-14
resume: FOLLOW-UP W5b (2026-09-15, Ben's labelled terrain hexes; section "Terrain threshold follow-up"): stage 1
  scores done (work/pgg/threshold/thresholds.md: masks right except the compass rose 5527 in the lake mask, fixed
  by open 9); pgg.json now lake 0.24, woods 0.24, swamp before woods; candidates re-run; 17 contact sheets read
  (119 -> woods, 5 -> lake, all right); 8 stale verify exceptions removed. STAGE 2-4 DONE: resolutions 219
  (117 added, 7 updated; 0703 kept clear as a named exception: the printed road runs through it), merge gate
  passed, assembled (clear 1433, woods 356, lake 15, swamp 25), render 0 warnings, render tiles re-cut, verify r1
  gate passed, 16 after-sheets (work/pgg/threshold/after) match, network_check 0 broken (3 new PGG exceptions for
  the river on 1602's shore). validate-xml 11 valid; banner-check 0 failures. GOLDENS (replayed 17:00): combat-split,
  untried-reveal, interdiction, supply match unchanged; overrun RE-RECORDED (only "spent" 12 -> 14: 2409 now woods);
  full-turn re-pathed (move 8 by 0208-0308-0408; move 14 by 0214-0314-0413 into 0412) and RE-RECORDED, replays.
  README lesson written. DONE (status review): build current, ctest --preset win-msvc-debug 224/224, banner-check
  0 failures, validate-xml 11 valid. Report: section "Terrain threshold follow-up". Waiting on Ben: open questions
  1-5 and the new 6 (the hexes just under 0.24: woods 5808 0.229, 4516 0.220, lake 0503 0.228).
  PREVIOUS: DONE, status review (2026-09-15). Every stage passed its gate: calibrate (max residual 0.008 hex), 120
  tiles catalogued, merge (102 resolutions, 0 open), assemble + render (0 warnings), verify round 1 (120 of 120
  records, 0 differences left) and round 2 (contact sheets, 0 differences), network_check 0 broken on TRC, PGG
  and Dai Senso. After Ben's feedback: renderer id fix by the coordinator (id-side unchanged), render tiles re-cut,
  terrain audit of 282 borderline hexes: 0 corrections (Stage results). Six PGG goldens re-recorded (Ripple),
  build current, ctest 224/224. Waiting on Ben: open questions 1-5, and the names of any terrain hexes he still
  finds wrong. The history below is kept for the record.
  HISTORY: stages 1-4 done (calibrate gate passed; 120 scan tiles, drafts, candidate tiles). Stage 5 catalogue:
  `python merge.py pgg --status` lists complete, bad and missing tiles (the source of truth; relaunch readers
  for missing tiles only). STAGE 5 DONE: 120 of 120 read; merge GATE PASSED (53 resolutions, 0 open).
  STAGES 7-8 DONE: assemble.py wrote map_graphics/xml/panzergruppe-guderian.xml (valid; clear 1579, woods 214,
  swamp 25, lake 11; rivers 637, rail 276, road 138); render 0 warnings; `tile.py pgg render` cut 120 render
  tiles. network_check: 24 broken rules (report only; to become the PGG profile's named exceptions after
  verify). Old sheet kept for before/after crops in the scratchpad (git show :path, read-only).
  Re-assembled after resolution 5701-5801 (54 resolutions, gate passed, rivers 636, network_check 22 named).
  Then the one-hex woods symbol rule (open question 3): 74 resolutions, gate passed, XML re-assembled (clear
  1555, woods 238, swamp 25, lake 11; every hex listed for the package loader). Re-rendered and re-tiled after
  V1 and V2 (render 0 warnings, network_check 0 broken with the named PGG exceptions).
  STAGE 9 IN PROGRESS: verify round 1 (prompt work/pgg/verify/verify-prompt.txt, records work/pgg/verify/r1/,
  exceptions work/pgg/verify/exceptions.json, `python verify.py pgg r1`): 81 of 120 records (rows 1-5 except
  0117_0420 and 2117_2420; row 6 0121_0424 .. 2521_2824); running V11 (0117_0420 2117_2420 2921_3224 3321_3624
  3721_4024 4121_4424 4521_4824 4921_5224) and V12 (5321_5624 5721_5924 0125_0428 0525_0828 0925_1228 1325_1628
  1725_2028 2125_2428); still to launch: 2525_2828 .. 5725_5928 and row 8 (24 tiles), two helpers of 8.
  Latest: 5620 back to clear (end of a forest arm, blob share 0.73 under the rule), 2624 excepted; river gap at
  4922 (two east hexsides) and five more reader-deleted rivers from the after-merge strong-river check; 93
  resolutions, gate passed, rivers 645 in 15 pieces, XML clear 1556 / woods 237 / swamp 25 / lake 11; five stale
  river exceptions removed from network_check's PGG list; TRC, PGG, Dai Senso 0 broken. Round 1 at 95 of 120
  records. Then 4818-4918 and 2627-2728 (reader-rejected river candidates at the ends of "sources"), the
  Roslavl railway 2526-2626 (V12), the Усвяча through 0911 (road-end pairing check) and the river through 3117
  (a reader's wrong-side substitution): 101 resolutions, gate passed, rivers 650 in 11 pieces, rail 276 in one
  piece, road 141 in 11 pieces; network_check 0 broken on TRC, PGG and Dai Senso; PGG_EXCEPTED no longer names
  any river source (only the 5903 loop beside the lake, Mogilev off the rail, road pieces and unroaded places).
  Verify: V1-V14 done (112 records); V14's and V13's difference was real: rail 4229-4330 was a corner clip
  (resolution false, 102 resolutions; link-triangle check now 0). RE-RENDERED and re-tiled (render 0 warnings)
  with every resolution so far. Running V15 on the last 8 tiles (0129_0431 0529_0831 0929_1231 1729_2031
  4529_4831 4929_5231 5329_5631 5729_5931) on the fresh render. Round 2 tile list:
  work/pgg/verify/round2-tiles.json (tiles with r1 differences or holding a hex resolved during verify);
  launch round 2 (records work/pgg/verify/r2/) when V15 ends, two helpers of 8. Being checked: road ends that face
  each other across no river. RENDER STILL PENDING.
  Resolutions now 86 (latest: railway along the 4215|4216 hexside moved to the 4216 side, named exception);
  merge gate passed, XML re-assembled, network_check 0 broken, rail one piece. RENDER STILL PENDING (0112, 1111,
  0910 river, roads 1416-1516 and 5807-5907, rail 4216): re-render at the first moment no verifier runs.
  New resolutions from r1: 0112 woods, 1111 clear, river 0910-1010 + 0910-0911, road 1416-1516, road 5807-5907 +
  5907:se (link-end check); 81 resolutions, merged, XML re-assembled, network_check re-run. RENDER NOT YET:
  re-render and re-tile at the first moment no verifier is running.
  Engine and tests: debug build clean; fast ctest 215/221 before the fixes; package test fixed and passing;
  five golden scripts updated and replaying; goldens NOT yet re-recorded (after the last verify round).
  Then resolutions for real differences, re-assemble, verify the differing tiles; network_check profile,
  ripple (tests partly edited, not built), goldens, build and ctest, report crops (compare.py), README fold.
  Next: batch N, full merge gate, assemble, render, tile render, verify rounds, network_check profile,
  ripple, build and ctest, README fold (a first standalone README is written; finish its verify, report and
  ripple sections). Reader cost so far: 216k-269k tokens and 40-83 minutes per 8-tile batch.
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
- 2026-09-15 W5: all stages done; terrain audit after Ben's feedback (0 corrections); goldens re-recorded; ctest
  224/224; status review

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

### Stage 5c -- what the first 16 Sonnet tiles taught (tune candidates BEFORE the next map's readers start)
- Readers are right where drafts are wrong, but slow: about 30k tokens and 8 minutes a tile, most of it in
  focused crops on draft "unclear" entries. Cheaper next time, by fixing the drafts rather than the readers:
  - lake outlines: exclude river candidates on hexsides that touch a hex with lake coverage >= 0.2
    (1301_1604: six lake-outline "unclear" rivers, all rejected);
  - river dips round one hex scoring "along" 0.65-0.8 were real every time: lower along_sure to about 0.65;
  - a river crossing a hex corner to corner is missed (0905_1208: 1008-1009) and a vertex pass is counted as
    a hexside (0901_1204: 0803-0904): a candidate that follows the river mask's thinned centreline from
    vertex to vertex would catch both;
  - printed lines that are not features (the dotted limit line) need no candidate, but name them in the
    reader prompt's ignore list, or readers add markers for them.
- Chains stop short: the masks under-run the last one or two hexsides of a river (at its rounded source) or
  a road (at a hexside where the grey meets a black grid line at a shallow angle): 3 of 8 tiles in batch C.
  Tell readers to follow every chain to where the print ends it; next map, grow the road mask by 3 and score
  river ends with a lower "along" threshold when the hexside continues a proposed chain.
- Woods under links: where a railway or road crosses a woods blob its pixels are not woods, so the measured
  coverage is low (5309: woods 0.447, rail 0.165, road 0.142; the print is about 0.7 woods). Next map: score
  woods + link coverage against the half rule when the hex is mostly woods round the link. Also: a "corner
  crossing" at t 0.8-1.0 on one hexside with near-zero presence in the far hex was a real step three times
  (5208-5309, 5309-5408, 5408-5508); promote it to a step when it continues a proposed chain.
- Towns: one reader's summary said "town marker"; its records were right, but the prompt now says places
  are never markers.
- A reader correctly placed РЖЕВ at 3901 (blocks), not 4001 (its name label), with the rule from the worked
  examples: the two worked examples paid for themselves.

### Stage 9 -- verify, first lessons
- The first verify record (3301_3604) flagged 3302 and 3705 as woods where the sheet had clear: each is one
  woods blob drawn wholly inside one hex, covering 0.31-0.44 of it. The half-hex rule misses the map's one-hex
  forest symbols. Rule added (maps/pgg.json terrain.woods "blob_share" 0.75, "blob_cover" 0.2; candidates.py
  measures "<mask>_blob" and "<mask>_blob_cover" per hex): a hex holding at least 75 per cent of a woods blob
  that covers at least 0.2 of it is woods. A scan over the catalogue found 25 such hexes; 24 became woods by
  resolution (open question 3), and 0708 is a swamp hatch the woods mask partly counts, so blob rules run only
  after every coverage rule has failed.
- PITFALL: a re-render during a verify round rewrites the render tiles readers are reading. Re-assemble the
  XML freely, but render and re-tile only between rounds.
- merge.py now refuses two resolutions for the same hex or hexside and feature (a later one silently
  replaced an earlier one when the blob rule superseded four half-rule resolutions).
- PITFALL: verifiers estimate cover by eye, and their estimates run high ("roughly 55-100 per cent" for hexes
  measured at 0.42-0.44). Every difference in the first 16 tiles was terrain; rivers, railways, roads and places
  all matched. Rule: point verifiers at the measured cover in candidates.json (an image measurement, not the
  catalogue) for any blob near half a hex, and at verify/exceptions.json for borderline hexes already looked
  at; each borderline hex gets one focused look and one exception, never a debate per round.
- Verify cost: V1 204k tokens / 55 minutes, V2 199k / 40 minutes for 8 tiles each, less than reading.
- Between re-renders a resolved hex is reported again by every verifier whose tile holds it (0112 twice: V4
  found it, V6 saw the not-yet-re-rendered render). Expected, not a new difference: verify.py merges duplicate
  readings, and round 2 re-checks the differing tiles on the new render. Launch batches that do not hold a
  freshly resolved hex while a re-render is pending.
- Two verifier readings were not differences: a railway drawn over two separate road ends looks like a joined
  road (5508-5607, named in verify/exceptions.json), and V3 recognised the pond at 0907/1006/1007 as the named
  catalogue exception. A second river strand stopping at a confluence vertex (0910) was real: rule 6.
- PITFALL: a verifier (V5) reported five swamp hexes (0808 1310 1512 1612 1613) as a "systematic" render error,
  woods drawn as swamp; the crops show plain swamp hatch symbols, so the catalogue and render were right. A
  claimed systematic error needs one crop per hex before anything changes; the verify prompt now says how swamp
  and woods are printed. The same batch caught a real one: 1111 woods at 0.469, overridden by its readers.
- PITFALL: a road running close beside a railway loses steps. Both overlapping readers of 1416 had the road into
  and out of the stretch but not 1416-1516, where the grey line rides just above the railway; the render showed
  the gap as a road stub ending in a hex centre, and the verifier caught it (V7). Next map: after merge, list
  every link chain end that stops in a hex holding a crossing of another link kind and look at each.
  Done on PGG after verify batch V7: six road ends in railway hexes. 5508, 5607, 2814/2915 and 3314/3414 are
  printed dead ends (dots, most beside a river); 5807 is not: the road runs on along the 5907|5908 hexside to
  the east map edge, and the catalogue had stopped it in 5807.
- A verifier reading a render cut before a resolution reports the old state again, sometimes with a wrong
  explanation (V8: "the road runs via 1415" where road presence in 1415 is 0.0). Settle such claims from the
  presence numbers first, then one crop; record them in verify/exceptions.json with the numbers, and let round
  2 re-check the tiles on the new render.
- A railway along a hexside that the readers put on the wrong side shows in the render as a peak or zigzag
  into a hex the print only touches (V7 at 4215: main line along the 4215|4216 hexside, spur from 4217 meeting
  it there). Rule, as for 0130: take the side with the larger presence (4216 0.161 against 4215 0.079), name it
  an exception, and check the junction lands in the hex the spur enters.
- PITFALL: apply a measured rule with one measurement. The first one-hex-woods scan measured blob share on the
  full hex; candidates.json (and the verifiers) use the inset hex. 5620 read 0.76 in the scan and 0.73 in
  candidates.json, so it was made woods while its twin 2624 (0.70) stayed clear, and a verifier rightly pointed
  at the inconsistency. Both are ends of forest arms, not symbols; 5620 went back to clear. Scan with the tool of
  record (candidates.json), and look at every hex within 0.1 of a threshold before resolving.
- PITFALL: readers delete correct draft rivers. Verify V11's report at 4921 led to a gap at 4922 (two east
  hexsides at river "along" 1.00 missing); a check over the whole catalogue then found five more hexsides at
  along >= 0.8 that readers had removed as "wobbles" or "vertex passes" (0830-0931, 1831-1931, 4103-4202,
  4103-4203, 4203-4304). Every one was real: each missing hexside broke a printed river. Neither overlap nor
  network_check caught them, because both overlapping readers dropped the same hexside and the broken ends
  looked like river sources. Rule: after merge, list every hexside with river along >= 0.8 (lake edges left
  out) that the catalogue lacks and look at each; a reader's removal of a strong candidate needs a crop.
  PITFALL (network_check exceptions): five of the named PGG exceptions ("river sources" at 0830, 3807, 4828,
  4303 and the "short piece" at 0731) were really these reading gaps. After the seven hexsides were added the
  rivers drained and the exceptions stopped occurring (network_check flags a stale exception, which is how it
  showed). A "does not drain" or "short" report is a question; before naming it an exception, look for a
  strong river candidate the catalogue lacks at the piece's end. River pieces went from 20 to 15.
  Then the same check at the ends of the four remaining "sources" (candidates along >= 0.45 meeting the end
  vertex): two more gaps, both hexsides a reader had explicitly rejected (4818-4918 "mirror", 2627-2728), each a
  river cutting a hex corner to corner (rule 6). The exceptions at 3423 and 2624 stopped occurring; the last two
  sources (3117, 0912) have no candidate at either end and stayed named. River pieces: 13.
  Then a road-end pairing check (two road ends facing each other with no river between them in the catalogue)
  found one more river gap: the Усвяча cutting through 0911 corner to corner (0811-0911 and 0911-0912, measured
  "along" only 0.25 and 0.35 because the river runs inside the hex, far from both hexsides). That joined the
  "source" at 0912 too. A candidate threshold cannot catch a river that cuts through a hex's middle; the pairing
  check (roads end at dots on both sides of a river) found it. The last one, 3117, was a reader's substitution of
  the wrong hexside (3117-3216 on the ne side for 3117-3217 on the se side where the river runs): fixed, the piece
  drains. RESULT: all 12 "does not drain" / "short" river pieces first named as printed exceptions were reading
  gaps; the rebuilt sheet has no river "source" exception left (the printed sources, such as 3605's rounded end,
  are upstream ends of draining rivers). River pieces: 11. RULE: treat every "does not drain" report as a reading
  gap until the crops at both ends of the piece show a rounded printed end.
- A branch railway whose junction lies just past a hexside loses its last step: Roslavl's western line crosses
  2526's se hexside and joins the main line inside 2626, but both readers stopped it in 2526 (V12 found the dead
  end). The link-end check lists such ends; a railway ending in a hex with no dot, city or map edge is a question.
- Labels: river-name labels (Вопь and the others) were kept from the old sheet at their old pixel positions and
  never re-read; a verifier noticed "Вопь" with no river near it. Not a feature; listed under open questions.

### Render style is not a difference (renderer change, 2026-09-14 evening)
- hexsheet2svg.py now by default tints every hex with a city-major, city or capital glyph pale grey and draws
  4-5 scattered black rectangles in it (after panj/tempest); `--urban-symbols` restores the single symbol.
  The XML is unchanged. In verify, how a city is DRAWN is a deliberate style; only whether a hex holds a city
  or town, its kind and its name must match the print. The verify prompt says so; the README will too.
- Schema change (Ben-approved, applied by the coordinator): <sheet> has a REQUIRED urban="buildings|symbol"
  and the renderer follows it (the --urban-symbols flag is gone). A map's config MUST choose it
  ("sheet": {"urban": "buildings"}); assemble.py throws without it, writes it on <sheet>, and carries every
  other root attribute of the existing sheet (id, title, source, width, height, background, font) unchanged.

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

### Where these notes went in the README (map_graphics/xml/tools/image2sheet/README.md)
- Stage 0 (look first, colour census, printed id position, extent, lines crossing hexes off-centre, blobs): README
  "Stage 0 -- look before configuring" and its PITFALLS.
- Stage 1 (approximate centres from rulers, locate, eye check, fit, gate, dense check and its lone outlier):
  README "Stage 1 -- calibrate".
- Stages 2-3 (overview, tiles, the two ids per hex): README "Stage 2-3 -- overlay and tiles".
- Stage 4 (masks that worked, the white-dash railway, grey road with opening, woods hue rule, swamp, presence
  against zigzags, one-hex blob rule, known weaknesses of the candidates): README "Stage 4 -- candidates".
- Stage 5a-5c (worked examples first, the reader prompt, its decision rules, spot-check the first record,
  batches of 8 and two helpers after the connection drop and the usage limit, `merge.py --status`): README
  "Stage 5 -- catalogue" with the reader prompt template; decision rules 1-9 there carry the hard calls.
- Stage 5b edge markers: README "5d. Edge markers".
- Merge (batch-by-batch partial merges, resolutions and named exceptions, verbatim reader questions, duplicate
  guard) and the after-merge checks (strong rivers the catalogue lacks, lake outline, woods under links, one-hex
  blobs measured with one tool, link chain ends, road-end pairs across rivers, every "does not drain" piece):
  README "Stage 6 -- merge".
- Assemble (every hex listed for the package loader, urban attribute, root attributes kept, off-map exits as
  comments, dry run on drafts): README "Stage 7 -- assemble".
- Render timing (never during a verify batch): README "Stage 8 -- render" and "Stage 9 -- verify".
- Verify (render style is not a difference, measured cover over eye estimates, exceptions file, stale-render
  reports, swamp misread, focused round 2 with contact.py): README "Stage 9 -- verify".
- network_check as report only, exceptions named and self-checking, most first "printed" exceptions were gaps:
  README "Stage 6" checks and "Stage 10 -- report".
- Pointy-topped and two-grid maps: README "Other map kinds".
- Not folded (PGG-specific facts, kept here only): the old sheet's errors, city hexes, entrance areas, golden
  script moves, costs per batch.

## Terrain threshold follow-up (W5b, 2026-09-15, after Ben's labelled hexes)
Ben named hexes the render gets wrong: water 1402 1602; forest 1601 1701 1702 1801 1902 2201 0816 1118 1218 1808
2008 "and more". All were clear. Numbers, sweeps and the colour census: work/pgg/threshold/thresholds.md.
- NOT a mask fault. A census of the pixels no mask holds inside those hexes is paper (248,248,248) and river teal:
  no pale green or beige is left out, so the measured shares are right and the HALF RULE was wrong for this map.
  Ben's forests measure 0.266 (1801) to 0.487 (2201) woods; his water 0.308 (1402) and 0.340 (1602) lake fill.
  The one real mask fault: the light-blue compass rose in 5527 counted as lake 0.293 (the only wrong flip in the
  first pass). Fixed with lake mask "open": 9 (5527 -> 0.137; real lake edges lose under 0.005).
- Thresholds (maps/pgg.json). woods at_least/sure/maybe 0.50/0.60/0.30 -> 0.24/0.35/0.15; lake 0.50/0.60/0.20 ->
  0.24/0.35/0.15; blob_share 0.75 and blob_cover 0.2 unchanged; swamp unchanged but now tested BEFORE woods, so
  0708's hatch (woods 0.276) and 2604 (0.240) stay swamp. 0.24 sits in the gap of the measured distribution: woods
  5808 0.229 then 3012 0.253; lake (opened) 0503 0.228 then 0502 0.252.
- Sweep of flips against the threshold: woods 0.50 -> 0, 0.40 -> 74, 0.30 -> 115, 0.25 -> 119, 0.20 -> 121.
- CHANGES: 119 hexes clear -> woods, 4 clear -> lake (0502 1402 1602 5503), NONE the other way; sheet terrain
  clear 1552 -> 1433, woods 237 -> 356, lake 11 -> 15, swamp 25 unchanged. All 124 as resolutions (117 added, 7
  updated: 0405 1109 1111 1602 1701 1902 3025 4717 5620 were "clear" by the half rule), resolutions now 219.
  The 119: 0113 0117 0202 0213 0309 0405 0417 0514 0527 0721 0801 0810 0816 0901 0924 0929 1009 1010 1029 1104
  1106 1109 1111 1118 1203 1208 1209 1218 1601 1611 1701 1702 1706 1712 1801 1806 1808 1902 1906 2002 2008 2102
  2105 2127 2201 2227 2302 2305 2308 2324 2402 2409 2503 2505 2602 2624 2628 2702 2705 2706 2716 2731 2801 2805
  2816 2830 2914 2931 3002 3008 3012 3025 3226 3309 3319 3404 3408 3430 3507 3521 3522 3529 3602 3603 3606 3613
  3622 3628 3707 3708 3722 3729 3731 3802 3912 4123 4328 4427 4502 4519 4619 4717 4810 4905 5004 5120 5208 5220
  5311 5506 5511 5609 5616 5618 5620 5716 5718 5814 5821. Every one of Ben's named hexes is among them.
- NAMED EXCEPTION 0703: lake fill 0.315 over the threshold, but the printed road runs down its land (east) part;
  a lake hex cuts the road (network_check "road enters lake hex"), so 0703 stays clear.
- Looked at, in both directions: 17 contact sheets of the 134 candidates and near-misses before the change
  (work/pgg/threshold/sheets), then 16 sheets of the 124 changed hexes against the new render
  (work/pgg/threshold/after). Every flip is right; the only wrong one found was 5527, fixed by the mask.
  Hexes just under the new thresholds, still clear: woods 5808 0.229 and 4516 0.220 (under a railway); lake 0503
  0.228, 5601 0.217, the 0907/1006/1007 pond (0.205, the old named exception).
- Gates re-run: merge GATE PASSED (0 open), assemble + render 0 warnings, render tiles re-cut, verify r1 GATE
  PASSED (120 records, 0 differences; 8 stale "borderline, clear" exceptions removed from verify/exceptions.json),
  network_check 0 broken on PGG, TRC and Dai Senso (3 new PGG exceptions: the river leaving the lake along 1602's
  shore, 1601:s 1602:ne 1602:se), validate-xml 11 files valid, banner-check 0 failures.
- Ripple: overrun re-recorded (only the German stack's "spent" 12 -> 14; 2409 is now woods). full-turn re-pathed
  and re-recorded: the 26th Armoured goes 0109-0208-0308-0408 (0309 is now woods, and the old path cost 22 of its
  20 halves), the 27th Armoured, on half allowance beyond Kurochkin's radius, goes 0114-0214-0314-0413 and across
  the river into 0412 instead of 0213-0313-0412-0411 (0213 is now woods), and the German 39th Corps' move over
  0213 costs 6 halves instead of 4. combat-split, untried-reveal, interdiction and supply replay unchanged. TRC
  goldens untouched. ctest --preset win-msvc-debug: 100%, 0 failed of 224.
- README: the lesson is in "Stage 9 -- verify" as LESSON (PGG follow-up), with the five-step procedure.
- OPEN QUESTION 6 for Ben: 0.24 is calibrated on his examples, and the hexes just under it (5808 0.229, 4516
  0.220 under a railway, lake 0503 0.228) are the nearest misses; say if any of those should be terrain too.

## Stage results (worker fills in: residuals, tile count, disagreements, differences per round, counts)
- terrain audit after Ben's feedback "some terrain hexes are incorrect" (2026-09-15, no hexes named): first a
  whole-map comparison of every hex's catalogue terrain with the measured rule (candidates.json), which found only
  resolved hexes and four woods-under-links hexes (0817 0822 0917 4616, each looked at: woods). Then every hex
  within reach of a threshold (woods 0.29-0.65 or a one-hex blob share of 0.4 or more, plus lake and swamp
  edges: 282 hexes; 142 woods, 129 clear, 6 swamp, 5 lake) went onto 36 contact sheets of scan and render with
  the measured cover in the caption (`python contact.py pgg work/pgg/verify/terrain --list
  work/pgg/verify/terrain-borderline.json --terrain --per-sheet 8 --radius 0.75 --scale 0.8`), read by W5.
  Result: 0 hexes corrected; every render terrain agrees with the print under rules (a) and (b) of open
  question 3. The hexes a person could read either way, all within 0.05 of a threshold: clear 0117 0213 0721
  1118 2201 2305 2503 3226 3603 3708 4427 5311 5620 (woods at 0.41-0.49 or a blob share at 0.51-0.73); woods
  1901 3614 4817 5607 5807 (0.49-0.55). If Ben's incorrect hexes are among these, the fix is a threshold choice
  (open question 3), not a misread; if they are elsewhere, he needs to name them. After the audit,
  `python verify.py pgg r1` also counts differences at resolved places as settled: GATE PASSED (120 of 120
  records, 0 differences).
- calibrate (2026-09-14): GATE PASSED. 15 control points, all eye-checked. Fit size 61.761, ox 133.39,
  oy 121.34, grid 59 x 31 (old sheet: 61.65 / 138.13 / 125.02, 56 x 31). Residuals: max 0.008 hex (5931,
  0.84 px), worst per region NW 0.007, N 0.000, NE 0.006, W 0.005, C 0.006, E 0.006, SW 0.004, S 0.003,
  SE 0.008. Affine diagnostic: scale x 61.763, y 61.756, rotation 0.0000 deg (the scan is undistorted).
  Extent: all 1829 grid hexes inside the image; every one has at least 3 printed edges; no printed hex
  outline outside the grid. Dense check: 1680 hexes, offset median 0.004, p95 0.009, max 0.143 (3815,
  looked at: see process notes).
- verify round 2 (focused, 2026-09-15): `python contact.py pgg work/pgg/verify/r2 --per-sheet 6` made 17 contact
  sheets of scan and new render side by side for all 99 resolved places (every resolution except closed reader
  questions), read by W5 instead of re-reading 63 whole tiles. Result: 0 differences; every decided value (rivers
  added or removed, railway and road fixes, named exceptions drawn on their chosen side, lake, clear and woods
  decisions, the 24 one-hex woods symbols) shows in the render as in the scan. About 40k tokens against an
  estimated 1.3M for whole-tile re-verification.
- after-merge checks during verify round 1 (against candidates.json and the link graph): strong river candidates
  the catalogue lacked (8 hexsides, all real), "does not drain" river piece ends (4 more hexsides and 1 wrong-side
  hexside), road ends paired across no river (2 river hexsides through 0911), link chain ends (road 5807-5907 to the
  east edge, road 1416-1516, rail 2526-2626), one-hex woods blobs re-measured with candidates.json (0112 added,
  5620 corrected). River pieces 20 -> 11; every river "source" exception first named turned out to be a gap.
- verify round 1, first 16 tiles (V1, V2, on the render before the one-hex woods rule): 11 difference readings,
  all terrain: 3302 3705 4801 (one-hex woods symbols, now woods by resolution), 0901 1602 1701 1902 3802 4502
  4905 (borderline, looked at, clear by measured cover: named in verify/exceptions.json). No river, railway,
  road or place difference.
- catalogue (stage 5): 120 of 120 tiles read (2 worked examples by W5, 118 by 14 Sonnet batches of 8; the first
  4 helpers of 15 died in a connection drop and a usage limit). Sonnet cost 213k-274k tokens and 40-83 minutes
  per 8-tile batch, about 3.0M tokens in all.
- merge GATE PASSED (2026-09-15): 120 records, 1829 hexes, rivers 637 hexsides, rail 276 steps, road 138 steps;
  53 resolutions (named exceptions among them), 0 open.
- after-merge coverage checks on the full catalogue: lake (fill + outline) flags only 1602 and woods (under
  links) only 0326, both already looked at and resolved clear; no woods >= 0.5 or lake >= 0.5 hex left
  un-terrained; no swamp hatch >= 0.03 outside a swamp hex; no city-block colour >= 0.03 without a place.
- assemble + render (first): sheet valid (validate-xml 11 files all valid), render 0 warnings, terrain clear
  1579 / woods 214 / swamp 25 / lake 11; rivers 637, rail 276, road 138; banner-check 0 failures.
  network_check: rail ONE piece of 261 hexes (the old sheet needed tidying to connect); river 21 pieces; road
  12 pieces; 24 broken rules (river sources that do not drain, two river hexsides beside lake hexes, 3 short
  river pieces, Mogilev 0424 off the rail (the rail reaches Mogilev's other hex 0525), 12 road pieces and 9
  places no road reaches). One of them was a reading error: 5701-5801 is the lake's outline, not a river
  (resolution added). The rest go to verify, then become the PGG profile's named exceptions if printed.
- network_check PGG profile (tools/network_check.py): PGG_EXCEPTED names the 22 printed exceptions of the
  rebuilt sheet with their reasons (8 river sources, 2 short river pieces, a river loop beside a lake hex,
  Mogilev 0424 off the rail, 12 road pieces, 9 places no road reaches); they print as EXCEPTED, and an
  exception that stops occurring is itself a broken rule. TRC 0, PGG 0, Dai Senso 0 broken rules (exit 0 for
  the ctest command). FINDING: the Russian redesign draws no road across any river (roads end at a dot
  beside it), hence the 12 road pieces; see open question 4. The list is revised after verify.
- merge, early look (45 of 120 records, into scratch): 803 hexes, 16 open items (14 overlap disagreements:
  terrain 4, rivers 6, rail 2, road 2; 2 reader questions). All 16 settled by 12 focused crops
  (work/pgg/look/res/) into work/pgg/catalogue/resolutions.json: 13 resolutions, plus 1008-1109 (a river
  hexside no tile had read, needed to keep the printed river whole) and three named exceptions (road along the
  4808|4809 hexside; railway leaving at the 3901 corner). Pattern: the two readings of an overlap mostly differ
  where one reader followed a chain to its end and the other stopped at its tile's area edge.
  Batch by batch after that: after 57 records 1 new item (road 2414-2515, real: it follows the 2414|2415
  hexside 10 px inside 2414, then passes a vertex into 2515); after 59 records 2 (terrain 4516 and 4913, both
  clear: a strip under the railway, a central blob). Resolutions now 21; open 0. Resolving each batch as it
  lands keeps each look small: two or three crops, no queue at the end.
  After 66 records: 1 disagreement (road 5615-5616), which uncovered a whole misread stretch. PITFALL: two
  Sonnet readers recorded a road running ALONG hexsides (level along the row 15/16 boundary of columns 54-56)
  as crossings of those hexsides (5415-5416, 5515-5516, 5615-5616); the render would have drawn a zigzag. Rule 5 of
  the reader prompt now says a line along a hexside never makes that hexside a step, with this road as the
  example; the stretch became named exceptions (north side taken). An automatic check for the same
  misreading (catalogue steps whose hexside the link mask covers along >= 45 per cent) found nothing, and
  would not have found this one either: a grey road lying on a black grid line is broken by the grid line in
  the colour mask. Only the overlap disagreement and the focused look caught it, so do not rely on a mask
  check here; verify (render versus scan) is the net for any other stretch. Also after 66 records: ВЯЗЬМА is 4015 (its
  blocks and the road's end dot), not the name's hex 3915; ГЖАТСК 4607; ВИТЕБСК 0412 and 0513.
  After 95 records: 3 items. RULE for a line through a column of hex corners: a crop cannot tell which side it
  is on (pixel jitter), but the candidates' per-hex presence can: the railway down column 03 (rows 22-28) had
  presence 0.13-0.18 in every column-03 hex and 0.00-0.007 in column 02, so it runs through column 03; one
  reader had left it out as "along the boundary". Settle such calls from candidates.json, then confirm the
  chain with one wide crop.
  After 108 records: 8 items (terrain 2525 woods and 3025 clear by woods + link coverage; river 1028-1129 real
  and the river cutting 1128 corner to corner as 1128-1129 + 1128-1228 by rule 6; Roslavl's railway and road
  2525-2625 real; the railway along the 0130|0230 hexside a named exception, side chosen by presence 0.04 vs 0.008).
  RULE for a line along a hexside: choose the side with the larger presence, and say so in the exception.
  After 115 records (no disagreements): a check of the facts the package test pins showed 1401 clear, where the
  print and the old sheet have a lake. PITFALL: the lake mask counts only the cyan fill, not the lake's dark teal
  outline (river colour), so lake-edge hexes are undercounted and no reader or overlap catches it (every tile
  agrees with the same wrong number). Check after merge: every hex with lake fill >= 0.2 and fill + river-colour
  >= 0.45 that the catalogue does not call lake, then look: 1401 (0.446 + 0.127) and 5801 (0.428 + 0.094) are
  lake; 1602 (0.34 + 0.161, most of it a real river) stays clear. A small pond across 0907, 1006 and 1007 covers
  under half of every hex: a named exception, not on the sheet. Next map: add the outline colour to the lake
  mask (or grow the fill mask by the outline width) before drafting.
  The same after-merge check for woods (woods < 0.5 but woods + rail + road >= 0.6, not woods in the catalogue)
  flagged 0326 and 5807. Look before deciding: in 5807 the railway and road cross the blob (woods); in 0326
  the railway runs BESIDE the blob and hides no woods (clear). A sum alone decides nothing.
  PITFALL: a reader question whose "at" is not one token ("3901:nw / 3901:n") is kept verbatim by merge and is
  closed only by a resolution with that same verbatim "at" (value null, pointing to the real decision). The
  reader prompt now asks for one token in "at" and the options in the note.

## Spot-check list (places for a person to compare with the print)
Named exceptions (the print cannot settle them; each in work/pgg/catalogue/resolutions.json with its reason):
- road running exactly along hexsides: 4709-4809-4909 (along 4808|4809); 5316-5415-5516-5615-5716 (along the row
  15/16 boundary, north side taken)
- railway along the 0130|0230 hexside: 0229-0230-0131 (side by presence 0.04 against 0.008)
- railway leaving the map at the 3901 corner: taken as 3901:n
- a small pond across 0907, 1006 and 1007 covering under half of every hex: not on the sheet
Terrain decided by rule rather than by an obvious reading (open question 3):
- one-hex woods symbols made woods: 0112 0212 0224 0312 0326 0422 0512 1014 2228 3010 3017 3123 3302 3705 3817
  4017 4028 4131 4214 4321 4801 4913 5320 5721
- blob share just under the one-hex rule (0.55-0.75), kept clear as ends of forest arms: 0213 2227 2624 4427 5208
  5620 (5620 was woods by an earlier full-hex measurement, corrected)
- 5721 woods by the one-hex rule at a low cover (0.24): the railway splits a forest arm into two blobs, so the
  mask sees the part in 5721 as its own blob; a person may prefer clear
- woods hidden under a railway or road, made woods: 2525 5309 5807
- lake outline counted as lake: 1401 5801
- borderline, kept clear by measured coverage under half: 0405 0901 1109 1111 1602 1701 1902 3025 3802 4502 4516 4717
  4905
Cities placed on their blocks, not their name labels: Smolensk 2216 2217, Vitebsk 0412 0513, Mogilev 0424 0525,
Vyazma 4015, Gzhatsk 4607, Roslavl 2626, Kaluga 5921, Rzhev 3901, Orsha 0420.

## Ben's known errors: before/after crops
Each crop is one JPEG with three panels of the same pixel box: the scan ("primary"), the old sheet
(pgg-old.png, from the git index) and the new render. Folder: map_graphics/xml/tools/image2sheet/work/pgg/report/
(made with `python compare.py pgg OUT --hex ID | --box ... primary OLD.png render`).
- orsha.jpg -- ОРША 0420: old sheet one railway through the city and one river past it; new: four railways
  (north, north-east branching in 0419, south-east branching in 0420, south) and three rivers meeting at 0420,
  as printed.
- east-57-59.jpg -- columns 57-59: absent on the old sheet (grid 56 columns); new grid 59 x 31 with the woods of
  5609-5914 as printed.
- rivers-48-49.jpg -- the river down columns 48/49 (4921-4828) with its twists, and the two railways crossing
  at 4827: absent on the old sheet (it had a different river across the south); new matches the print.
- rivers-57-58.jpg -- the Ока down columns 57/58 to the south edge, КАЛУГА at 5921 (old: 5619, no river):
  new matches the print.
- road-0120.jpg -- the road from the east reaches 0120 and leaves the map across its nw hexside (old: no road
  in 0120); the render draws the road to 0120's centre (no off-map link in the sheet language).
- south-rail-1831.jpg -- railways to the south edge at 1831 (box 6) and 3131 (box Z,5); old: none. All south
  exits: 0131, 1831, 3131, 3831, 4331, 4531, 5431, plus 5907 and 5921 on the east edge.
- vp-5907-5915.jpg -- 5907 (X box, "(20 ПО)") and 5915 ("(20 ПО)") on the sheet; old: beyond its grid. The
  "(20 ПО)" labels are added to markers.json for the next assemble.
- entrance-west.jpg -- entrance areas B-H as printed (labels at their boxes); the engine's area table follows
  markers.json (see Ripple).

## Ripple (worker fills in: facts, tests, scenario, each golden diff)
Done so far (not built yet; builds only after the new sheet lands, because 5907 is not on the old grid):
- games/pgg/engine/PggFactsMap.cpp: the provisional entrance-area table replaced by the printed areas of
  markers.json (listed below); the "entrance areas are provisional" data gap removed; the "no Railroad hex on
  the south edge" gap now reported only when no south-edge hex has a rail link (the 0120, 5907 and 5915 gaps
  were already conditional).
- games/pgg/engine/PggFacts.h: header comment no longer calls the areas provisional.
- games/pgg/engine/PggArrivals.cpp: South-Western Front divisions now enter only on a south-edge RAILROAD hex at
  or east of Z (14.22 as written); before, any south-edge hex, the fallback for the old sheet's missing
  south-edge rail. No test, golden script or scenario issues a south-western entry, so no golden moves.
- games/pgg/test/PggPackageTest.cpp: facts of the printed map (Smolensk 2216/2217, not 2117; Kaluga 5921 on the
  east edge; rail 2116-2216, road 2215-2216, road 0120-0219; 12 Victory Point hexes; no data gaps; X at 5907,
  area 6 at the south-edge rail hex 1831). Built and passing (6/6).
- games/pgg/test/PggRulesTest.cpp: the road-across-a-river movement case uses a plain river hexside into clear
  terrain (the print has no road across a river; same expected costs).
- map_graphics/xml/tools/image2sheet/assemble.py: writes the default terrain's <hexes> list too, because
  hexrules' PackageLoader knows a hex only from a <hexes> list or <hex> element (the pgg-test scenario's 1414
  was "unknown" without it).
- Golden scripts (games/pgg/golden/*.script.xml), each replayed to the end with hexgames_cli --replay:
  - overrun, combat-split, untried-reveal: moved 18 columns east (0608-0811 -> 2408-2611, same pattern) because
    0610-0611 is now a river hexside; every hex used is clear with no river, railway or road (found by a search
    over the catalogue). Notes say why.
  - interdiction: the rail move 0613 0714 0715 becomes 0613 0714 0814 (the printed railway runs 0714-0814).
  - supply: the German 6th Infantry starts in 3401 (was 3014, now on a road): still more than twenty hexes from
    the road network that reaches 0120 (columns 01-14) and from the west edge, so still isolated. Note rewritten.
  - full-turn (label long): replayed after re-recording.
  RE-RECORDED (2026-09-15, after the terrain audit; `hexgames_cli --record SCRIPT GOLDEN`, none hand-edited; TRC
  goldens untouched, `git status games/trc game_records` clean). Each diff:
  - interdiction: the Soviet rail move ends in 0814 (was 0715); everything else identical.
  - overrun, combat-split, untried-reveal: every hex 18 columns east (0608-0811 -> 2408-2611); same odds, results
    (overrun 2-1 D2*, combat-split 2-1 D1*/A1 and 4-1 D2*), retreats and reveals.
  - supply: the German 6th Infantry now starts in 4902 (23 hexes from the road network, which reaches column 28
    on the printed map) and steps to 5002; still "isolated" and marked unsupplied, now spending 2 of its 4 halves
    (was 3014 -> 2916 and 4). The first retry in 3401 was supplied (16 hexes from the roads) and lost the
    golden's point: check the supply event is still there after any re-record of this script.
  - full-turn: the old marches broke on the print in three ways, found one at a time by --record (the engine
    names one refusal per run):
    (a) cost: the 31st and 26th Armoured and the 20th Army HQ crossed the woods of 0406/0506/0606/0706 and the
    river 0705-0804 (11-13 points of 10); new paths go by 0407-0508-0607-0707-0806 (the HQ now ends in 0904, not
    1103; the 26th in 1107), the 50th Rifle ends in 0710 (0708 swamp now costs 7).
    (b) 5.23 "expend the full allowance": each new march spends it exactly; the 2nd Moscow Rifle, 27th Armoured
    and 57th Motorised are beyond Kurochkin's radius at the phase start (half allowance, 6.55), so the 27th goes
    on 0412 -> 0411 (5) and the 57th stops in 0513 (5).
    (c) printed entrance areas (14.3): Reserve Front HQ and two rifles at X 5907 (were 5614/5615), 22nd Army HQ
    and a rifle at V 2101 (were 1101), the 14th and 20th Motorized at C 0112 and 0111 (were 0116 and 0117).
    Everything else in the turn (interdiction targets, army dice, air interdiction) is as before.
  Build and tests (2026-09-15, foreground, no other build running): `cmake --build --preset win-msvc-debug` (no
  work to do), then CLion's ctest.exe `--preset win-msvc-debug`: 100% passed, 0 failed of 224 (hygiene 2 incl.
  hygiene_map_networks and banner check, xsd 5, golden 14, long 3, pgg 37, trc 43). The five PggGoldenTest
  failures and hygiene_map_networks Ben saw mid-run are green.
  PITFALL for the next map: a search for new golden paths must use the engine's own cost model (PggTerrain.cpp:
  Leaders pay woods like foot, a road step off a river costs 1 half for non-foot units, 6.4's column rule, and
  the 6.55 half allowance), not a guess; mine missed the leader rule and the half allowance and cost two runs.
Plan (written before the catalogue is final; each item is confirmed or dropped after merge and verify):
- PggFactsMap.cpp: the provisional area table becomes the printed areas of catalogue/markers.json (A 0101-0501
  on the north edge; B 0101-0112, C 0108-0115, D 0113-0117, E 0118-0122, F 0123-0126, G 0124-0128, H 0128-0131
  on the west edge; V/1 1901-2701; W/2 3901; 3 5101; 4 4331; Z/5 3131; 6 1831; X 5907); the data gaps for
  entrance areas, 0120, 5907, 5915 and the south-edge rail go if the sheet now carries them; smolensk_ follows
  the printed city (2216 and 2217: needs a decision on which hex, or both, the facts name).
- game_rules/xml/panzergruppe-guderian.xml german-city-vp codes and prose: Smolensk 2117 -> printed hexes
  (2216 2217), Kaluga 5619 -> 5921, Vyazma 3915 -> 4015 (blocks and the road's end dot in 4015, name over 3915;
  look/res/p-vyazma.jpg; the 24th Army already sets up in 4015), Gzhatsk 4606 -> 4607, Vitebsk 0513 -> 0412 and
  0513, Mogilev 0424 -> 0424 and 0525 (blocks either side of the river, name over 0624; look/res/p-mogilev.jpg),
  and any other city the catalogue moves (an open question for Ben before editing). Multi-hex cities follow
  the rulebook's "all hexes of the city" (15.12): Vitebsk, Smolensk, Mogilev.
- games/pgg/test: PggPackageTest (2117, 5619, 2016-2117, 10 VP hexes, 5 data gaps), PggStateTest and
  PggSequenceTest (2117 as a target), goldens interdiction and full-turn (script target 2117), PggRulesTest
  (0305, 3310, 4015, 5525 positions: check terrain and edges there after assembly).
- games/pgg/scenario/pgg-1941.xml: 39 hex references; all stay on the grid (the grid only grows); check
  none lands on a hex the new sheet makes lake.
- Goldens: re-record every PGG golden with hexgames_cli --record after the sheet lands; TRC goldens must stay
  byte-identical.

## Open questions for review (worker fills in)
1. Victory Point hex codes in the rules XML (german-city-vp) name the hexes of the old sheet, several of them
   the hex of a city's NAME, not its blocks. The print: Smolensk 2216 + 2217 (rules 2117), Vyazma 4015 (3915),
   Kaluga 5921 (5619), Gzhatsk 4607 (4606), Vitebsk 0412 + 0513 (0513), Mogilev 0424 + 0525 (0424); Orsha 0420,
   Roslavl 2626 (rules 2625), Rzhev 3901 as the rules say. The rulebook scores "all hexes of the city" (15.12)
   and names "both hexes of Smolensk" (13.2). Proposal: change the codes (and PggFacts::smolensk_) to the printed
   hexes, a list for the two-hex cities. Not done: the rules XML is an input of this task, and the engine's
   Smolensk logic names one hex. Until then the German scores Smolensk's 25 by holding 2117, a clear hex.
2. Printed misprint: entrance box W reads "W,1" on the Russian map; the English map reads "W, 2". The sheet
   label keeps the print; the engine takes W = provisional area 2. Agree?
3. Terrain of a partly covered hex: no rule in the rulebook, errata or amendments. The sheet uses two rules:
   (a) a hex covered at least half by woods, lake or swamp takes that terrain (lake outline counted as lake;
   woods hidden under a railway or road counted as woods after a look); (b) a hex holding most of a terrain
   symbol drawn inside it takes that terrain: the swamp hatch symbol, and a woods blob lying at least 75 per
   cent inside one hex and covering at least 0.2 of it (the redesign draws many one-hex forest symbols; the
   first verifier flagged them as woods on sight). Rule (b) turned 24 hexes woods: 0112 0212 0224 0312 0326 0422
   0512 1014 2228 3010 3017 3123 3302 3705 3817 4017 4028 4131 4214 4321 4801 4913 5320 5721 (most of them
   woods on the old sheet too); the ends of narrow forest arms just under the share threshold stay clear (0213
   2227 2624 4427 5208 5620). Agree with (b), or keep the half rule alone (these 24 clear)?
4. The Russian redesign draws NO road across a river anywhere: every road ends at a dot beside the river (at
   Smolensk the road from the north ends in 2216 and the road south starts in 2217; at 3025 the road ends at
   the Десна). The sheet follows the print, so the road network is 12 pieces and German supply by road (11.11)
   cannot reach 0120 from east of the first river, although 11.13 says roads count as crossing rivers for
   supply. Options: (a) the engine treats a road ending at a river hexside as joined to the road across it
   (a rules/engine change, not a map change); (b) the sheet adds the road crossings the 1976 original prints
   (a second source, not this map); (c) accept the print. Recommendation: (a). Nothing was invented on the sheet.
   PggRulesTest's road-across-a-river case now uses a plain river hexside (same costs).
5. River-name labels, the title and the panels were kept from the old sheet (the task's "keep ... labels and
   panels unless the print shows them wrong"); the process never re-read them, and a verifier noticed "Вопь"
   placed where no river runs. Re-read and re-place the river names (a short pass over the print), or leave the
   labels as decoration? They carry no game data.

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
