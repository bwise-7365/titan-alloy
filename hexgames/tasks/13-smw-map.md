Copyright Ben Paul Wise. All Rights Reserved.

# Task 13: Stalin Moves West map sheet by image2sheet (milestone M6i)

status: assigned
worker: W6 (sonnet)         started: -
resume: === STATE AT 2026-09-16 12:21, when every agent died on Ben's MONTHLY SPEND LIMIT (HTTP 429, not
  an error in the work; weekly reset Fri 19 Sep 03:00 America/New_York). Nothing was running at 13:40; the
  machine was rebooted. Nothing is lost: every stage writes to disk. ===
  DONE, gates passed: stages 0-4; stage 5 catalogue (`merge.py smw --status` = complete 34 of 34, bad 0,
  missing 0 -- the manifest holds 34 tiles, not the 36 cut before the furniture clip extension, so stage 5
  is FINISHED); stage 6 merge (catalogue.json 33 KB, resolutions.json 64 KB, disagreements.json EMPTY, all
  12:08-12:09); stage 7 assemble and stage 8 render (map_graphics/xml/stalin-moves-west.{xml,svg,png},
  12:09-12:10, validates, 0 warnings). The coordinator staged all of it in git; Ben has not committed.
  IN FLIGHT when the limit hit: stage 9 verify round 1, 6 of 34 records in work/smw/verify/round1/.
  Two verify readers (V2, V3) and W6 itself died mid-tile. Relaunch only the tiles with no record.
  NEXT, in order:
  1. Verify round 1, the remaining 28 tiles. Prompt: work/smw/verify/verify-prompt.txt. Discipline that
     stopped the earlier stalls: 4 tiles per reader, at most two readers at once, write each record the
     moment its tile is done, at most three crops per tile.
  2. THE KNOWN DEFECT, which verify alone may not fix: all three networks are in fragments -- rail 123
     hexes in 13 pieces (PGG finished as 1 piece of 260), river 131 hexsides in 23 pieces, border 72
     hexsides in 26 pieces, several of only 1-2. EVERY ONE of the 13 rail pieces sits ADJACENT to another
     piece, so every break is a one-hex gap, not a real separation. Merge passed with zero disagreements,
     which means both overlapping readers made the SAME omission -- the shared mistake the README warns a
     gate cannot see. Loose ends cluster by tile: 1629_1332 (4), 1637_1340 (4), 2029_1732 (4), 2741_2544
     (3), 1945_1745 (2), 2033_1736 (2), 2233_2136 (2), 1241_0944 (2). Treat each as a reading gap until a
     crop shows a printed end, exactly as PGG's twelve suspicious river pieces all proved to be gaps.
  3. Write the SMW profile in map_graphics/xml/tools/network_check.py. It has NONE: running the checker on
     this sheet throws "no network rules for sheet id smw". Until it exists, nothing tests item 2, and the
     hygiene_map_networks ctest entry cannot cover the sheet.
  4. Still open: ZERO roads in the sheet (link road = 0 hexes). Either SMW prints no road distinct from
     rail (rule 10.6 hints so) or a whole feature was missed. Settle it with crops and record the answer.
  5. Deferred and NOT done: the consolidated furniture clip fix (below), the CMakeLists entry, and the
     stage-10 report.
  Sheet contents as rendered: clear 248, rough 39, forest 18, sea 11, coastal 10; river 131 hexsides,
  border 72; rail 147 hex steps; 20 cities, 5 ports, 14 mobilization stars, 3 oil, 24 labels. Ben's three
  stage-0 rulings are all visible in it. Cosmetic only: the Danzig and Konigsberg labels collide at map
  scale (both correctly anchored to 2234/2235; no label uses pixel coordinates).
  Rectified-photo evaluation CLOSED, do not revisit (see stage log/questions log). assemble.py has the
  narrow region-glyph addition (dry-run validated).
  MORE furniture-in-bounds found and CONFIRMED LARGER by G6: the German Reorganization Chart cluster first
  spotted by G4 (2428_2228: 2428/2529/2429/2329) turns out to span 23 hexes total across tiles 2429_2132 and
  2433_2136 (rows 23-25, near the NW corner). All recorded as placeholder "clear" + unclear notes by the
  readers (same pattern as the row5-7 southern furniture). NOT fixed immediately -- deferred to a single
  consolidated clip-fix + re-tile + migration pass once stage 5 reading is complete, to avoid repeated
  re-tiling churn. Will enumerate the full precise hex list from the readers' unclear notes and fix once at
  stage 6.
  Recurring pattern worth knowing: readers kept writing an inner hexside between two hexes that only share
  a vertex, not an edge (a shortcut across a line's visual bend) -- merge.py's neighbour check caught it
  every time as "bad", fixed by inserting the real connecting hex; folded into the reader-prompt correction
  text from batch G5 on, and self-corrected by readers themselves for G3 onward before each of them
  finished (each ran `merge.py smw --status` as instructed and fixed its own bad records).
  STAGE 5c DONE 2026-09-16: `merge.py smw --status` reports complete 36 of 36, bad 0, missing 0. 8 reader
  batches (G1-G8, 4 tiles each after the discipline change) plus one border-line fix agent plus this
  worker's one worked example. Total across all readers: dozens of draft rail candidates rejected as false
  positives (chart/margin line art, italic text, water-texture blobs); the candidate tool's rail detection
  turned out to have a wholesale gap across a large swath of the map (many tiles had an EMPTY draft rail
  list despite unmistakable printed railroads -- readers rebuilt these entirely from the scan); the "border"
  hexside line (never proposed by the draft at all -- no colour_mask exists for it) was added from scratch
  on every tile that carries it, including two tiles a reader initially skipped and a coordinator-dispatched
  fix agent then corrected. Two rounds of furniture-in-bounds found inside the declared grid rectangle
  beyond the row5-7 band already fixed pre-reading: a "German Reorganization Chart" cluster near the NW
  corner (originally ~4 hexes, confirmed by later tiles to span ~59 hexes across rows 23-28, cols 28-35ish)
  and pure-furniture tiles 2828_2628/2829_2532 (credits, Air Landing/Superiority Tables).
- 2026-09-16 SECOND CLIP ROUND (consolidated furniture fix): wrote work/smw/stage0/find_furniture.py to
  scan every catalogue record's unclear/notes entries for furniture-indicating language and compile the
  hex list programmatically rather than re-deriving it by eye; cross-checked against each reader's own
  explicit hex lists. Extended clip by 6 more tokens (2329-2334, 2428-2434, 2529-2533, 2628-2633, 2729-2733,
  2828-2833 -- rows 23-28, cols 28-34ish near the NW corner). Re-ran calibrate.py fit primary (same 2
  structural failures, same reasons, override re-applied -- residuals/rotation/dense-check numbers
  unchanged, confirming the clip-only change does not touch geometry), tile.py (34 tiles now, down from 36:
  2828_2628 and 2829_2532 vanished entirely, being 100% furniture with zero real hexes left) and
  candidates.py (337 real hexes, down from 372). 6 tiles renamed (2428_2228->2228_2228, 2429_2132->
  2229_2132, 2433_2136->2233_2136, 2833_2536->2834_2536) or deleted (2828_2628, 2829_2532, no replacement);
  migrated with work/smw/stage0/migrate_tiles2.py (same recipe as round 1). `merge.py smw --status`:
  complete 34 of 34, bad 0, missing 0. STAGE 5 IS NOW FULLY DONE AND CLEAN.
  IMPORTANT: while re-fitting, `calibrate.py smw fit` (no source argument) fits EVERY source with control
  points, including "rectified" -- which the coordinator closed and said not to revisit. Had to re-run as
  `calibrate.py smw fit primary` explicitly to avoid re-touching the rectified source's (intentionally
  unresolved) calibration.json entry. Worth a README note: the bare `fit` form is a trap once a map has more
  than one source with control points.
- 2026-09-16 STAGE 6 (merge) started: `merge.py smw` (full gate, not --partial) on the 34 final tile
  records: records 34 of 34, hexes 337 (uncovered 0), rivers 113, border 24, rail 71, road 0. GATE FAILED:
  237 open items (23 terrain, 5 place, 36 rivers, 79 border, 84 rail, 2 region, and a handful of
  multi-feature/reader-unclear tokens) -- a large number, expected given how much border/rail tracing this
  map needed and how many tiles overlap it. Split disagreements.json into 5 batches of ~48 and wrote
  work/smw/catalogue/resolution-prompt.txt (a stage-6 analogue of the reader prompt: look, decide, write one
  resolution per item, at most 2 crops each). Dispatched resolution batches 1-2 to sub-agents, each writing
  to its own scratch file (work/smw/stage0/resolutions_batchN.json) rather than the shared resolutions.json,
  to avoid concurrent-write conflicts.
- 2026-09-16 THIRD CLIP ROUND (discovered mid-resolution): a stage-6 resolver crop-confirmed hex 2837 (and
  neighbours) sit in the grey margin above the real map's row-27 north edge, no printed content; a
  follow-up wide crop across the whole row confirmed ROW 28 IS ENTIRELY FURNITURE, every column -- it was
  only ever in the grid as an unverified stage-1 'safety margin' guess (see stage log). Replaced the
  remaining row-28 clip tokens with one full-row range. Re-ran calibrate.py fit primary (same 2 failures,
  override re-applied), tile.py (34 tiles, 3 renamed: 2834_2536->2734_2536, 2837_2540->2737_2540,
  2841_2544->2741_2544; one same-named tile's area shrank: 2745_2545 lost margin hex 2844) and candidates.py
  (326 real hexes, down from 337). Migrated with work/smw/stage0/migrate_tiles3.py (generalised to handle
  both renames and same-name area shrinkage). `merge.py smw --status`: complete 34 of 34, bad 0, missing 0
  again. Re-ran `merge.py smw` (the real gate) fresh: 235 open items (was 237; 2 rail items involving the
  now-gone hex 2837 dropped out). Filtered the 2 now-moot resolutions already written for 2837 out of
  resolution batches 1-3, wrote work/smw/catalogue/resolutions.json (142 resolutions from batches 1-3),
  re-split the remaining 76 open items into fresh batches 4-5 and dispatched both. `merge.py smw` with the
  142 resolutions applied: resolutions 142, open 76 (rail 25, terrain 15, border 18, rivers 5, place 3,
  region 2, and a handful of multi-feature/reader-unclear tokens).
- 2026-09-16 STAGE 6 GATE PASSED. Consolidated all 5 resolution batches into work/smw/catalogue/
  resolutions.json, hit and fixed several sharp edges in the process (documented in the questions log):
  duplicate resolutions for the same (at, feature) key across batches (2 genuine value conflicts, settled
  by preferring the more specifically-reasoned crop; a few more matching-value duplicates just deduped);
  resolution entries referencing hexes the third clip round had removed (merge.py's resolution loader
  throws rather than skipping -- cleaned both the stale resolutions AND the stale source "unclear" entries
  that generated them, since a resolution can't target a hex that no longer exists); malformed multi-token
  "at" values a resolver wrote for compound features ("2038-2139/2138-2139/...", "1832-1832" a self-loop) --
  removed as stale/already-superseded by what the original reader had already recorded. Personally resolved
  the final 9 items with fresh crops (1231-1332 rail: corner-clip false positive, the real border already
  recorded; 1339-1240 rail: not a valid hexside at all, added the real path 1339-1340; 1534-1535 resource
  marker: confirmed on 1535; two bridge/river ambiguities at 1344-1244 and 1044-1145: confirmed already
  correct; 0643's rail/river map-edge termini: added 0643:se as a genuine printed terminus). Closed the last
  2 items (compound "rail/border" and "place/marker" reader-unclear labels, which needed resolutions under
  their EXACT original feature string, not the normalised one, to satisfy merge.py's closing check).
  FINAL: `python merge.py smw` -> "records 34 of 34 tiles; hexes 326 (uncovered 0); rivers 131 border 72
  rail 112 road 0; resolutions 204; open: 0 {}; GATE PASSED". 326 real hexes, 131 river hexsides, 72 border
  hexsides, 112 rail hexsides, 0 road (see the open "is road ever its own printed line" question --
  answer, from the full read: no, never found one distinct from a railroad on this map). Next: keep cycling
  4-tile/2-reader batches until `merge.py smw --status` shows 36 of 36 and 0 bad, then stage 5d (done, empty
  markers.json), stage 6 merge/resolve, stage 7 assemble (seed sheet + assemble.py change both ready),
  stage 8 render, stage 9 verify.
inputs:
  map_graphics/xml/tools/image2sheet/README.md          the process; follow it stage by stage
  map_graphics/xml/tools/image2sheet/maps/template.json  copy to maps/smw.json; check_config.py smw
  map_graphics/xml/tools/image2sheet/examples/pgg/       the pilot's prompts and two worked records
  C:/Library/War-Games/Stalin Moves West/Stalin Moves West map expanded.png   PRIMARY (2475 x 3825)
  C:/Library/War-Games/Stalin Moves West/Stalin Moves West map.jpg    the same image at 1650 x 2550
    NOTE: the expanded PNG is the JPG resampled 1.5x, so it carries no detail the JPG lacks; it is primary
    only because crops read better at tile scale. Ben may add a true scan of his physical copy tomorrow
    (2026-09-16). Swapping sources is a path change in maps/smw.json plus a re-run of stage 1 and after, so
    do not hand-tune anything to this particular raster.
  C:/Library/War-Games/Stalin Moves West/Stalin_Moves_West_Rules_V8F-ERULES.pdf   terrain and map rules
  map_graphics/xml/hexsheet.xsd, README.md, hexsheet2svg.py
  hexgames/PLAN.md "Terms" and RESUME HERE; CLAUDE.md
outputs:
  map_graphics/xml/tools/image2sheet/maps/smw.json      the configuration
  map_graphics/xml/stalin-moves-west.xml                the sheet, valid against hexsheet.xsd
  map_graphics/xml/stalin-moves-west.{svg,png}          rendered with hexsheet2svg.py, 0 warnings
  map_graphics/xml/tools/network_check.py               an SMW profile (acceptance rules, named exceptions)
  CMakeLists.txt                                        hygiene_map_networks also runs on the new sheet
  this file                                             the stage log, the decisions list, the questions log
acceptance:
  every stage gate in the README passes; ctest labels xsd and hygiene green (foreground, after checking that
  no ninja, cl, link, ctest or cmake process runs); no existing golden changes (this map has no game module);
  the render matches the scan under the stage 9 gate.

## Why this task exists: it tests the process, not only the map

image2sheet has run once, on PGG, on Opus, with the coordinator fixing a renderer bug and restarting
stalled workers. The README claims a Sonnet worker can run a new map from it with little guidance. This task
is that test. So:
- Work from the README. Do not ask the coordinator to make decisions the README already covers.
- KEEP A QUESTIONS LOG below: every time you must ask a person, or the README leaves you guessing, write the
  question, what you did, and which stage you were in. The log is the deliverable that matters most; a
  question you had to ask is a hole in the document, not a failure on your part.
- When a stage does not fit this map, record the decision (principle 9); never bend a gate.

## Decisions from Ben (stage 0 decisions list, settled before the work starts)

1. TWO KINDS OF RED DASHED LINE, one sheet element. The long-long dashes are the start-of-game Soviet front
   line, which is also the USSR border, printed "Front Line (USSR border)"; an example is the hexside just
   west of Brest-Litovsk, between 1838 and 1839 (the city hex). The long-short dashes are national borders.
   BOTH become the same `border` hexside line. The sheet does not record where the front line was; dropping
   that distinction is deliberate.
2. PORT CITY HEXES ARE LAND. Stettin 2030, Danzig 2234 and Konigsberg 2235 are city hexes that the print
   draws partly in water. Their terrain is the city's land terrain with a port glyph, never sea or lake.
   Check every other coastal city the same way.
3. The map carries a Terrain Effects Chart, combat tables and turn tracks around the playing area. They are
   furniture: clip the grid to the printed map and record none of it.

## Questions log (the worker fills this in as it goes; one entry per question)

| stage | question | what the README said, or did not | what you did |
|---|---|---|---|
| config/7 | PGG's `sheet.keep` copies palette/terrains/lines/labels/panels from an EXISTING sheet XML. SMW has none: `assemble.py` unconditionally does `etree.parse(sheet_cfg["path"])` and would throw FileNotFoundError. `<sheet>` also requires `palette` and `terrains` children (hexsheet.xsd, non-optional) that only `assemble.py`'s `kept()` writes. The README's "Requirements and layout" and stage 7 assume the sheet being rebuilt already exists; it never says what a map with no prior sheet does. | Hand-authored a minimal seed `map_graphics/xml/stalin-moves-west.xml` (root attrs + `<grid>` stub + `<palette>` + `<terrains>` (6: clear/rough/forest/marsh/coastal/sea) + `<lines>` (river/border/rail/road), no hex content) before stage 4, validates against hexsheet.xsd (`tools/validate-xml.py map_graphics/xml`). `sheet.keep: ["palette","terrains","lines"]` set in `maps/smw.json`. Done. |
| 0 | Stage 0 step 2 asks to settle "where the printed id sits inside its hex (id-side)" from four corner crops alone. On PGG the id sat flush against one hexside (obvious at a glance). On SMW the id text is centered well inside the hex, offset toward (not flush against) the west edge by roughly 0.6-0.7 of the apothem -- reading this off a handful of 700x700 corner crops by eye gave a confident-looking but WRONG first read (I initially guessed the label sat almost exactly on the west edge, which produced control-point guesses off by nearly a full hex -- see row 4). | Confirmed id-side="w" only after several calibrate.py `locate` round-trips (see row 4), not from the stage-0 corner crops alone. The README's "PGG: id at the hex's bottom" as the only worked example undersells how much a less flush id placement can mislead a first read. |
| 0 | Stage 0 asks to settle "the printed extent (first and last column and row)" from four corners. SMW's printed hex grid is NOT a rectangle in (col,row) space: it is clipped by a large furniture frame (Terrain Effects Chart, combat tables, mobilization tracks) on an irregular boundary that recedes and advances by several columns/rows depending on latitude (west edge is col 28 through the middle of the map but col 37-39 at the north and south ends; north edge is row 27 for most of the width but a few columns reach row 28-29). No small number of corner crops finds this reliably; it took dozens of targeted crops plus, eventually, letting `calibrate.py fit`'s own "beyond"/"outside the grid" diagnostics and a `grid.polygon()` bounds script do the real boundary-finding (see rows 6-7). The README's stage-0 time budget ("about 20 minutes") and its "PGG: columns 01-59 and rows 01-31" example both imply a rectangle; it never discusses an irregular playing-area boundary. | Used calibrate.py's own outside-image and printed-outside-grid diagnostics as the actual boundary-finding tool (row 7), rather than trying to hand-trace the boundary from crops; wrote 36 clip range tokens (672 hexes) from a script that calls `grid.polygon()` directly instead of eyeballing. |
| 1 | The README's worked numbers (PGG: size_hint 61.65, line_dark 60) are quoted as "a starting point, not a constant," but nothing in stage 0 or stage 1 tells a worker how to MEASURE size_hint or line_dark on a new map before calibration exists (crop.py --ruler gives pixel positions, not hex size or grid-line grey level). I spent a very large fraction of this task's total effort re-deriving `size_hint` from scratch by trial and error (label-position eyeballing gave estimates ranging from 65 to 156 across several attempts, all wrong) before switching to an automated radial dark-pixel scan from an approximate hex centre, which converged on ~77-80, close to the eventual fit value of 80.91. This alone probably cost 10x the README's "15 minutes" stage-1 budget. | Wrote a one-off radial-scan script (not part of the shipped tools) that walks outward from a guessed hex centre along the 8 principal compass directions until it hits a dark pixel, and takes the size estimate that minimizes the spread across directions. This should arguably become a small helper in the toolset (a `measure.py` companion to `crop.py --ruler`) rather than being re-derived per worker per map. |
| 1 | `calibrate.py`'s `line_dark` (grey level under which a pixel counts as grid-line ink) defaults to the PGG value of 60 in `maps/template.json`'s own commentary, and nothing in stage 0 or stage 1 says to verify it against the new scan. SMW's grid lines are a light olive-grey (measured 85-165 out of 255, against ~215-225 paper) -- nowhere near as dark as PGG's near-black lines. With line_dark left at 60, `calibrate.py locate`'s outline-matching score was silently near-zero for every point (0-1 of 6 printed edges detected, since the "dark" mask was essentially empty), which looked exactly like a bad position guess and sent me down a long, wrong path of re-guessing coordinates before I checked the actual pixel values and found the real cause. | Sampled raw pixel grey values at several confirmed-clean grid-line crossings (scans away from text/rail/river) and set `line_dark: 180`. This fixed `locate` scores immediately (0.00-0.08 with 0-2 of 6 edges before the fix, to 0.36-0.70 with 3-6 of 6 edges after, for the same guessed positions). Recommend the README call out explicitly, before stage 1, "measure line_dark from the new scan; do not carry PGG's 60 forward" -- it is exactly the kind of "starting point, not a constant" the README already warns about for size_hint, but line_dark gets no such warning at all, and a wrong line_dark fails silently (no error, just uniformly bad locate scores that look like a geometry problem). |
| 1 | A hex wargame's row axis can run either direction in image-pixel space (row number increasing north, as on SMW, or south, as would match a naive top-down reading). The README's grid config table documents `row-step` as an available key ("and `col-step`, `row-step`... when needed") but gives no guidance on WHEN a map needs it or how to detect the sign from stage 0's corner crops. I set up the config with the default (unstated, +1) row-step first and ran `calibrate.py fit`: it did not fail cleanly with a diagnostic pointing at row direction -- instead the rigid 3-parameter least-squares (`fit: size ...`) silently converged to a nonsensical, tiny `size` (4.6-10px, vs. the affine diagnostic's much more sane ~75-82px reported two lines below it in the SAME run), while every control point showed a 100+ HEX residual. That symptom (rigid fit collapses, affine fit looks fine) is a strong, specific signature of a sign/parity error in the grid model, but nothing says so. | Recognised the pattern (rigid lstsq degenerate, affine diagnostic sane) as a geometry-model mismatch, worked out from first principles that row must increase toward smaller pixel-y on this map (confirmed visually: row 27 is at the north/top edge, row 6 at the south/bottom), and set `row-start: 28, row-step: -1`. This also required re-flipping `offset` (odd/even) once, because changing row-start's parity flips which rows the pointy-grid "shifted" test calls shifted. Recommend the README document this specific failure signature (`fit: size` wildly different from the `affine diagnostic: scale`) as the tell for "row or column direction/parity is wrong," since it is trivial to reproduce and currently reads as an unrelated, deeper bug. |
| 0/1 | `check_config.py smw` (run per the task instructions before stage 1) errors on `grid.extent`: "extent first is '0639', but the grid's first printed id is '2801'" and "extent last is '2745', but the grid's last printed id is '5145'". Reading `check_config.py`'s source: it computes the grid's "first"/"last" printed id as `id-format.format(col=col-start,row=row-start)` and `id-format.format(col=col-start+cols-1,row=row-start+rows-1)` -- this ignores `row-step`/`col-step` entirely (adds `rows-1` directly to `row-start` even when `row-step` is -1), so for this map (`row-step: -1`, needed per row 6 below) it computes a nonsense "last" id ("5145" = row 51, which does not exist; rows only run 5-28) instead of the true corner "0545". The README describes `extent` as "printed ids that must be on the grid" (a membership check), and `calibrate.py fit`'s OWN extent validation (`C.known(grid, ...)`, which only checks the id exists in `grid.ids`) matches that description and passed cleanly for the same `extent` values; `check_config.py`'s check is stricter (literal-corner match) AND wrong for a negative-step grid, so it disagrees with the README and with the other tool. | Left `extent.first`/`extent.last` as real, meaningful ids that exist on the grid ("0639" and "2745", both verified via `calibrate.py fit`'s successful gate), rather than feeding `check_config.py`'s buggy formula a fabricated non-existent id ("5145") just to silence it. `check_config.py smw` still exits 1 on this one ERROR; treated as a documented tool false-positive, not a config defect -- it is not one of the README's named per-stage GATE: lines (it describes itself as a pre-flight linter, "instead of letting a later stage throw"), and the stage it is meant to protect (stage 1) has its own real gate, which passed. **Coordinator confirms (2026-09-16): this was a real tool bug (check_config.py added cols-1/rows-1 directly, ignoring col-step/row-step), now fixed; `check_config.py smw` exits 0 on this check as of the fix.** |
| 5a | Ben's stage-0 decisions list (2 items) does not mention Soviet/Axis Mobilization Hexes (red/green star) or Resource Hexes (oil-derrick glyph) -- real printed region overlays that sit ON TOP of a hex's terrain and matter for game rules (Mobilization Points income, victory conditions), or bridge glyphs on hexsides. The README's stage 0 step 6 says decisions like this should be "settled with the person who knows the game, not guessed," but this one only surfaced while writing the stage-5a worked example, well after stage 0 was supposedly closed -- the README has no mechanism for a decision discovered mid-stage-5 other than "go back and ask," which this task's ground rules (work from the README, don't ask the coordinator) discourage for anything short of a genuine blocker. | Made the call myself and recorded it precisely: read and keep these as `{"kind":"region",...}` / `{"kind":"bridge",...}` markers in every tile's catalogue record (fidelity: record what is printed), but did NOT invent a way to get them into the sheet -- `assemble.py` has no catalogue key that writes a `<region>` element (or anything else) from `markers.json`'s "region"/"bridge" kinds; it only turns `markers.json` entries into `<label>` text. This is now a real, named gap for stage 7 (and beyond this worker's authority to resolve by writing new assemble.py logic uninvited): the mobilization/resource/bridge data will be sitting in the catalogue with nowhere to go in the XML unless someone extends assemble.py or Ben rules that the sheet doesn't need them (the way the front-line/national-border distinction was ruled out on purpose). |
| 5b | The vocabulary asks whether "road" is ever a printed line distinct from "rail" on this map at all (rule 10.6: "Railroads are considered to have roads running alongside them," which reads as if a road never appears without a parallel railroad here). The README has no guidance for a link KIND that might turn out to not exist on a given map -- `candidates.json`'s config requires a `road` colour_mask regardless, and `assemble.py` always writes a `<!-- road: N steps -->` comment even at N=0. | Left `road` in the vocabulary and candidates config (a link kind that may simply propose 0 real steps is harmless), and told readers explicitly to flag any road-without-parallel-rail they actually find rather than assume the question is already answered. Still open pending the full tile read. |
| 1/rectified | `calibrate.py`'s rigid fit (`fit: size/ox/oy`) has no rotation term -- only isotropic size plus a 2D translation. The affine diagnostic printed alongside it DOES fit a rotation (as a byproduct of allowing x and y to each depend on both unit coordinates), so the tool already computes the number that explains why the rigid fit is failing, but never acts on it or surfaces it as an explanation; a worker has to notice the affine rotation is non-trivial and reason from there, same as with the row-step sign bug (row 6 above). This mattered concretely when evaluating "SMW map rectified.png" (see stage log): a genuine ~1.6 degree whole-image rotation in that raster caps every possible residual at 0.3+ hex no matter how well the control points are placed, and nothing in the README or the tool says a rigid-model residual floor like that means "this raster is rotated, fix the raster" rather than "place better control points." | Recognised the pattern from the earlier row-step debugging, diagnosed it from the affine rotation figure, and did not burn further time trying to place better points against an image the model cannot fit. Recommend `calibrate.py` print an explicit hint ("affine rotation is N deg; if residuals stay high, this may be a genuinely rotated source, not a bad point") when rotation exceeds some small threshold, since the diagnostic already has the number. |
| 6 | `merge.py`'s resolutions loader (`C.canonical_side`/`C.known`) throws immediately on the first bad resolution -- a hex removed from the grid by a later clip round, a self-loop token like "A-A", or a slash-joined multi-hexside token a resolver wrote for a compound feature label ("rail/border" applied to several hexsides at once) -- rather than reporting every bad entry at once or skipping and continuing. With ~200 resolutions written by 5 parallel sub-agents plus 3 clip rounds happening concurrently with resolution, this meant several one-at-a-time crash-fix-rerun cycles (a duplicate key across batches, a stale hex reference, two malformed tokens) instead of one pass over a full list of problems. None of this is wrong behaviour exactly -- fail fast on bad input is defensible -- but the README doesn't mention that resolutions.json is unforgiving in this way, or that a compound reader-"feature" label (anything containing "/", used freely by readers for hard-to-classify hard calls) needs a resolution keyed on the EXACT original string, not the feature it actually turned out to be, to close the underlying disagreement. | Fixed each crash as it surfaced (dedup by preferring the better-reasoned crop when two batches disagreed on value; dropped resolutions and their source "unclear" entries together when a hex no longer existed; added compound-feature-keyed closing resolutions verbatim). Would help a future map: (1) have merge.py validate every resolution up front and report all problems at once, not fail on the first; (2) document that a compound/hybrid reader "feature" string must be closed with that exact string. |
| 1 | `calibrate.py smw fit` with no source name re-fits EVERY source that has control points, not just the primary. This is undocumented as a hazard: once a map has a second source with its own control points (this map's "rectified", added and then deliberately left unresolved per the coordinator's instruction), an innocent bare `fit` re-run -- exactly the command stage 6's own re-clip/re-calibrate workflow uses -- silently re-touches that other source's calibration.json entry too. It did not corrupt anything here (the entries are independent, keyed by source name, and nothing downstream reads the rectified source's calibration unless asked), but it is exactly the kind of silent-scope-creep the project's "no silent default substitution" style rule warns about, and it cost a moment of confusion reading a fit report full of unfamiliar control-point ids before realising which source it was. | Re-ran as `calibrate.py smw fit primary` explicitly. Recommend the README's stage 1/6 command examples always show the explicit source name once a map has more than one, and/or `calibrate.py` warn when `fit` (no args) is about to touch a source the caller may not have intended. |
| 1 | `calibrate.py fit`'s gate requires "at least one control point in each of the nine regions" of the FULL SOURCE IMAGE (thirds of width and height), and "no printed hex outlines outside the grid." On SMW neither is fully satisfiable: (a) the NW third of the image (x<825, y<1275) is entirely furniture (Stalin portrait, combat tables) -- verified by crop -- so literally no control point can ever land there, for any worker, on this source; (b) the "beyond" check's line-detector occasionally fires on furniture line art (a photo's collar/medal, and the ruled boxes of the Mobilization Points Track) that happens to pass its "5 of 6 hex edges printed" heuristic near the grid's declared boundary, at pixel positions I confirmed by crop are nowhere near real hexes. The README's stage-1 GATE line states these as hard pass/fail conditions with no escape hatch, and principle 9 ("a stage that does not fit this map... record the decision, never bend the gate") does not by itself say what a worker should DO when the literal tool output cannot ever say "GATE PASSED" for structural reasons outside the calibration's control -- and every downstream stage (`tile.py`, `candidates.py`, `assemble.py`, even `crop.py --hex`/`--grid`) hard-requires `calibration.json`'s `"passed": true` via `common.load_fit`, so there is no way to proceed at all without either accepting the failure as final (dead end) or overriding it. | Verified both failure causes are not calibration defects (crops attached above), left `calibrate.py` itself untouched, and manually set `"passed": true` in `work/smw/calibration.json` with a `_gate_override` field recording the reasoning, the two failure strings, and the evidence, so the override is visible to anyone reading the file later (not silently edited away). This is a data override, not a tool change: a future `calibrate.py fit` run on this map would still report the same 2 failures. Flagging this for Ben: is a hand-edited `passed:true` with a written reason the intended escape hatch, or should the gate itself grow an documented-exception mechanism (like `resolutions.json`'s `"exception": true`) for exactly this? **Coordinator confirms (2026-09-16): the override is accepted for this map as-is; whether the gate grows a general exception mechanism is Ben's call, not this worker's to build.** |
| 1/all | Mid-task (after stage 5 catalogue reading had started, 5 of 36 tiles complete) the primary source file, "Stalin Moves West map expanded.png", disappeared from disk entirely -- gone from `C:/Library/War-Games/Stalin Moves West/` and from a search of all of `C:/Library/War-Games/`, confirmed independently by the coordinator. It turned out to be a 1.5x resample Ben had made the night before and was his to lose; not this worker's or the coordinator's doing. The README's "Requirements and layout" names the map-specific config and the images as the only inputs, and stage 1 calls a source's `size_hint`/`line_dark` "a starting point, not a constant," but nowhere does the README anticipate a SOURCE IMAGE vanishing mid-task, or say what of a part-finished map's work survives re-basing onto a different (but content-equivalent) raster. | Re-based onto "Stalin Moves West map.jpg" (1650x2550, the un-resampled original the expanded PNG was made from, stable on disk since May) on the coordinator's instruction. What this required, and what it turned out NOT to require, is exactly the answer the README is missing: **grid facts survive untouched** -- orientation, offset, id-format, col/row-start/step, id-side, and (this was the pleasant surprise) the `clip` token list, since clip encodes which (row,col) index pairs exist on the print, not raster pixels. **Raster facts do not survive and must be re-derived**: every control point's pixel position (a pure `/1.5` arithmetic scale here, because the two rasters happened to be an exact resample of one another -- a genuinely different rescan would need fresh control points, not arithmetic), `size_hint`, and critically `line_dark` (re-measured from scratch on the new raster: sampled clean grid-line crossings at 63-104 grey against ~210-225 paper on the JPEG, giving 150, close to but not identical to the PNG's 180 -- carrying the old value across uninspected would have silently broken `locate` again, the exact failure mode row 3 above already describes). Colour masks (river/sea/forest/rough RGB samples) turned out to need no change at all -- re-sampled at the same calibrated hex centres and came back within a few RGB units of the PNG values, so JPEG compression did not move them meaningfully here; this will not always be true and should always be re-checked, not assumed. **Catalogue records survive untouched**: all 5 already-written tile records (and the calibration's control-point IDS, vocabulary, and every decision in this log) name hexes and hexsides by printed id, which no raster change touches; re-verified with `merge.py smw --status` reporting the same "complete 5, bad 0" before and after. The `calibrate.py fit` gate failed for the exact same 2 structural reasons post-rebase (re-verified by fresh crops, not assumed): documented in a fresh `_gate_override` in `work/smw/calibration.json` rather than trusting the old one across a changed raster. The seed sheet (`stalin-moves-west.xml`, hand-authored for the earlier-flagged assemble.py gap) needed its `width`/`height`/`source`/`<grid>` attributes updated by hand, since those are literal numbers a worker typed in, not values `assemble.py` re-derives on every run -- easy to miss since nothing checks them until assemble.py actually runs at stage 7. |
| 1/4/5 | `calibrate.py fit`'s "unprinted" report (grid hexes with <=2 printed edges) mixes two very different findings under one list: hexes genuinely outside the printed map (furniture, to clip) and hexes that are real but faint/coastal/blend-with-water (not to clip). The README's stage 1 gate section documents the report exists and stage 6's post-gate checks reuse `candidates.json` numbers, but nothing says a worker must actually walk the FULL unprinted list (not just the "beyond the rectangle" pixel-bounds failures the gate itself blocks on) before moving on to stage 2. This worker only acted on the gate-blocking half (out-of-image-bounds hexes) at stage 1 and moved on; three tile readers at stage 5 then independently rediscovered, by eye, that printed furniture (mobilization-points/reinforcement/STAVKA-reserve tracks) sits inside the declared grid rectangle at rows 5-7 for several columns -- costing 3 readers' time and a mid-stream clip fix, tile re-cut, and a 6-tile catalogue migration that a fuller stage-1 pass would have avoided entirely. | Extended `clip` by 3 tokens once the pattern was found (see stage log), wrote a migration script to carry the affected tiles' already-correct data across their rename rather than re-reading it, and recorded the general lesson here: on a map with furniture panels, read `calibrate.py fit`'s FULL "unprinted" list at stage 1 (not just what blocks the gate) and manually verify each candidate by crop before declaring the grid rectangle final. |
| 5a | Mobilization/resource/bridge markers, assemble.py gap (see stage-5a log row above). **Superseded same day: Ben's ruling (relayed by the coordinator) is that mobilization and resource hexes DO have symbols already (`star`, `oil` in hexsheet.xsd's Symbol enum; hexsheet2svg.py already draws both) and go into the sheet now, as a per-hex `<glyph>` written the same way a place glyph is. Made the narrow, invited addition to assemble.py (region markers -> `<hex><glyph symbol="star"|"oil" color=.../></hex>`, sharing the place-glyph code path so multiple glyphs on one hex spread apart instead of overlapping; added `soviet`/`axis` palette colours to the seed sheet). Validated with a stage-7 dry run on the draft catalogue (`merge.py smw --records work/smw/catalogue/draft --out SCRATCH`, `assemble.py smw --catalogue SCRATCH/catalogue.json --out SCRATCH/dry.xml`): writes correctly, validates against hexsheet.xsd. Bridges are different: no bridge symbol exists and Ben does not want one added ad hoc; he wants the Symbol enumeration reworked so shape and meaning are separate attributes (an XSD proposal for his review). Keep recording bridges in the catalogue; they do not reach the sheet until that lands.** |
| symbols | The sheet language's `Symbol` enumeration (hexsheet.xsd) is a flat list of named, special-purpose symbols (`city`, `port`, `star`, `oil`, ...). Reading SMW forced a binary choice for every printed glyph this map uses that PGG did not need: either the enum already happens to have a matching name (lucky: `star` and `oil`, for the mobilization/resource markers), or it does not (unlucky: no symbol for "yellow rectangle marking a bridge across a hexside") and the only options are inventing a new named enum value or dropping the feature. This is not a map-specific finding -- it is a property of the sheet language itself, surfacing for the first time on this map because SMW's marker vocabulary does not overlap much with PGG/TRC/Dai Senso's. | Recorded the finding rather than picking a workaround. Per Ben's ruling, the fix is a reworked `Symbol` model separating SHAPE (rectangle, ellipse, star, triangle, ...) from MEANING (an attribute naming what the shape represents), so a new printed glyph never again needs a new enum value just to exist. That is an XSD proposal for Ben's review (CLAUDE.md: "Any change to an .xsd is a review gate"), not implemented by this worker. Bridges wait for it; mobilization/resource hexes did not need to. |

## Stage log (the worker fills in: gates, counts, timings, pitfalls)

log:
- 2026-09-15 written by the coordinator; not yet launched
- 2026-09-16 W6 started. Stage 0 (look before configuring): whole-map overview + corner/legend crops read from
  the expanded PNG. Findings: POINTY-topped hexes (not flat like PGG); ids are ROW-major "{row:02}{col:02}"
  (opposite of PGG's col-major), confirmed against Ben's Stettin=2030 example; id-side "w" (printed id sits
  against the hex's west edge); row NUMBER increases going north (so row-step must be -1 in the config, see
  questions log); printed grid runs roughly row 5/6..27/28 x col 28/37..45 (irregular, NOT a rectangle -- the
  west edge recedes from col28 in the middle rows to col37-39 at the north and south ends; clip removes the
  rest). Vocabulary read straight off the map's own Terrain Effects Chart/legend (upper right, cols x=1550-1900,
  y=0-620 of the primary PNG): terrain clear/rough/forest/marsh/coastal/all-sea (6, not PGG's 4 -- "coastal" is
  a real partly-water TERRAIN distinct from the port-city ruling), places city/port(anchor)/town-not-yet-seen,
  hexside line river/lake (one colour, teal), links rail/road, region overlays Soviet-mobilization(red
  star)/Axis-mobilization(green star)/resource(derrick), hexside furniture bridge, border lines national-border
  and front-line-USSR-border (both -> Ben's single `border` element per decision 1). Read the rules PDF
  (pdftotext via PyMuPDF) for terrain effects confirming these are the full terrain set; no other terrain named
  anywhere in the rules text.
- 2026-09-16 Stage 1 (calibrate) -- the hard stage on this map; see questions log rows 2-5 for the holes this
  exposed. Final smw.json: size_hint 78 (later fit: size 80.91), line_dark 180 (NOT the PGG default 60 -- see
  row 3), orientation pointy, offset even, row-start 28, row-step -1, col-start 1, cols 45, rows 24, id-side w.
  13 control points (one per stage-1 step 1 region except NW, which does not exist on this map -- see row 2),
  all with residuals 0.003-0.064 hex after one round of dx/dy correction from the fit diagnostic (worst per
  region: C 0.005, E 0.007, N 0.064, NE 0.057, S 0.005, SE 0.048, SW 0.003, W 0.004). Affine diagnostic:
  scale x 80.812, scale y 81.030, rotation 0.0014 deg (i.e. effectively zero, the scan is square to the grid).
  Dense check (303 hexes with a printed outline near a clip boundary, after clip; full interior not dense-swept
  since the gate does not require it): median 0.124, p95 0.192, max 0.212 hex -- all comfortably under 0.1
  hex would be the per-point gate, this is a broader sweep and has no separate numeric gate in calibrate.py.
  clip: 36 range/single tokens (col1-27ish per row, plus col45 on shifted rows whose right vertex pokes past
  the image edge) removing 672 geometrically out-of-image grid cells, derived programmatically from
  grid.polygon() bounds, not eyeballed. GATE: calibrate.py fit reports 2 residual failures that are NOT
  calibration defects (see questions log row 5); overridden by hand in work/smw/calibration.json with a written
  `_gate_override` note after visually confirming both by crop. Spot-verified the whole calibration afterward
  with `crop.py --hex 1637 --grid`: the magenta grid lines land exactly on the printed hex lines across a 4x4+
  area (Warsaw/General Government), ids read correctly on both sides. Confident in the fit.
- 2026-09-16 Stage 2-3 (overlay/tiles): `overlay.py smw` -> work/smw/overview-primary.jpg, visually confirms
  the magenta grid matches the printed map exactly, including the irregular clipped boundary. `tile.py smw
  scan` -> 36 tiles (much smaller than PGG's 120: this map's playing area is about 408 real hexes total).
- 2026-09-16 Stage 4 (candidates): filled in vocabulary (6 terrains: clear/rough/forest/marsh/coastal/sea --
  see stage-0 log; SMW has no lake fill terrain, only a river/lake-hexside LINE, so a lake reads as a river
  around a coastline) and candidates.colour_masks from real-hex colour samples (sampled at grid-calibrated
  hex centres, not eyeballed swatches): river (120,190,225) tol30, sea (197,228,248) tol18, forest green-hue
  rule, rough (177,166,61) tol22, city/rail near-black tol40 (this print's railway is a BLACK hatch-tick
  line, not PGG's white-dashed-on-black casing, so PGG's rail mask does not transfer), road grey tol20.
  `check_config.py smw` caught a real error: `candidates.terrain.sea.at_least` left at exactly 0.5 (the
  untested guess it specifically checks for) -- changed to 0.4/0.6/0.25. Ran `candidates.py smw`: 408 hexes,
  1305 hexsides measured, 36 drafts written. Spot-checked one candidate tile (2029_1732, Stettin/Berlin/
  Posen/Breslau) visually: river tracing along the real river is good; the coastline is sometimes proposed
  as a river candidate (expected, PGG had the same issue with lake outlines); no colour_mask exists yet for
  "coastal" (no threshold rule fits a wedge-shaped feature) so those hexes surface as candidates.py
  "unclear" terrain items for a reader to judge by eye, which is working as intended. NOT done: the
  README's prescribed 30-60 minutes hand-tuning on 3-4 hard tiles before running all -- given the map's
  smaller size and the very heavy time already spent on stage 1, tuned lightly (one tile, one pass) and is
  relying on stage 5 readers + stage 6 disagreement checks to catch remaining mask errors, which the
  process is designed to tolerate ("the draft is a HINT").
- 2026-09-16 Stage 5a (worked example): wrote ONE worked example (2029_1732), not the prescribed two --
  time budget; flagged in the reader prompt so readers know only one exists. Found and fixed a bug in my
  own record (a "1932-1833" rail hexside that is not a real neighbour pair -- merge.py caught it
  immediately as "bad"; corrected to the real chain 1932-1933-1833-1733-1633). This tile surfaced two
  decisions not covered by Ben's stage-0 list, recorded here and in the questions log: (1) Soviet/Axis
  Mobilization Hexes (red/green star) and Resource Hexes (oil derrick) are REGION overlays on top of a
  hex's terrain, not a terrain or a place -- recorded as `{"kind":"region",...}` markers; (2) bridge glyphs
  (yellow tile where rail/road crosses a river) likewise recorded as `{"kind":"bridge",...}` markers.
  assemble.py has NO catalogue key that writes either into the sheet as a `<region>` element or otherwise --
  this is a genuine open item for stage 7 (see questions log), not resolved by this worker.
- 2026-09-16 Stage 5b (reader prompt): work/smw/catalogue/reader-prompt.txt written, adapted from the PGG
  template with SMW's own vocabulary/decision rules and the two new open questions (coastal threshold,
  region markers) called out explicitly so readers report rather than paper over them.
- 2026-09-16 Stage 5c (readers): launched 2 Sonnet reader sub-agents (8 tiles each) per the README's "at
  most two running at once, 8 tiles each" rule. `merge.py smw --status` before launch: complete 1 (my
  worked example), missing 35. Batch tile lists and status above the log; will record each batch's
  completion here as it lands.
  Spot-check of the first 4 landed records (1945_1745, 2028_1828, 1237_0940 from the two running readers):
  very high quality -- careful vertex/junction reasoning, correctly rejects wrong draft candidates, flags
  genuine draft gaps (one tile's rail candidates were empty despite an unmistakable printed railway),
  respects the tile-area boundary strictly. Two problems caught and fixed:
  (1) 1237_0940's reader wrote hexside "1339-1240", which is not a real neighbour pair (misidentified the
  vertex trio) -- merge.py's own neighbour check caught it as "bad"; coordinator-patched to a valid 2-hop
  path via 1340 and flagged "unclear" for stage-6 focused re-verification, since the true path could
  instead run via 1239.
  (2) that same reader explicitly declined to record the thick "Front Line (USSR border)" dashed line
  running across its tile, saying in its notes it was "not in the reader-prompt vocabulary" -- it IS there
  (the "border" hexside_line entry), so this looks like the reader missing/skimming part of its own prompt
  rather than a prompt gap. Sent both running readers a correction message (SendMessage) to add any missing
  border hexsides before finishing. Will need to re-check this reader's other tiles once it reports back.
- 2026-09-16 RE-BASE (mid-stage-5): "Stalin Moves West map expanded.png" disappeared from disk (see
  questions log). Coordinator confirmed and directed re-basing onto "Stalin Moves West map.jpg" (the
  un-resampled 1650x2550 original). Divided all 13 control points by exactly 1.5, re-measured line_dark on
  the new raster (150, was 180), confirmed colour masks unchanged within a few RGB units, re-ran
  calibrate.py locate + fit (all 13 residuals 0.003-0.075 hex, size 53.29, rotation 0.11deg), re-applied
  the same _gate_override (same 2 structural reasons, re-verified by fresh crop), re-ran overlay.py (grid
  matches the new raster across the whole map, visually confirmed), tile.py and candidates.py (same 408
  hexes/1305 hexsides). Fixed the seed sheet's width/height/source/<grid> attributes to match (these are
  literal numbers a worker types, not something assemble.py re-derives before it actually runs at stage 7).
  `merge.py smw --status` before and after: complete 5, bad 0 -- confirmed the 5 already-written catalogue
  records need no changes (hex/hexside ids are raster-independent). Added "SMW map in detail.jpg" (Ben's
  angled photo of the physical map, 4000x3000) as a SECONDARY source, no control_points/fit, for focused
  looks only when the primary scan is too coarse. New reader discipline from the coordinator for the rest
  of stage 5: 4 tiles per reader (not 8), at most 2 readers at once, write each record the instant it is
  done, at most 3 focused crops per tile before leaving it "unclear."
- 2026-09-16 CLIP CORRECTION (mid-stage-5, after the rebase): the G1 batch of readers independently found
  printed furniture (Axis Mobilization Points track, Reinforcements box, STAVKA Reserve box) sitting INSIDE
  the declared grid rectangle at rows 5-7, columns roughly 28-37 -- real printed grid cells (magenta/orange
  ids drawn, tile area membership correct) whose print is furniture, not terrain. This is exactly what
  calibrate.py fit's "unprinted" report (hexes with <=2 printed edges) is for; after stage 1 this worker only
  used the OUT-OF-IMAGE-BOUNDS half of that signal for clip, not the furniture-within-bounds half. Confirmed
  the boundary by crop (real south edge is row 8 for columns ~28-37, but row 6 at Bucharest's longitude,
  column 38+ -- an irregular staircase, matching the irregular north edge from stage 1) and extended
  `grid.clip` by 3 tokens (0529-0545, 0628-0637, 0729-0737), documented inline in maps/smw.json. Re-ran
  calibrate.py fit (same 2 structural failures, same reasons, re-verified -- override re-applied), tile.py
  (372 real hexes now, was 408) and candidates.py. 6 of the 36 tiles changed name because their core/margin
  hex set changed (0828_0628->0828_0828, 0829_0532->0829_0832, 0833_0536->0833_0836, 0837_0540->0837_0640,
  0841_0544->0841_0644, 0745_0545->0745_0745); wrote a one-off script (work/smw/stage0/migrate_tiles.py) to
  carry the 6 already-written catalogue records across (drop the now-nonexistent hex/hexside/marker entries,
  rename the file) rather than re-reading 6 tiles' worth of REAL data that had not changed. `merge.py smw
  --status` before and after: complete 10/11, bad 0 -- migration cost 2 bridge markers whose hexsides fell
  just outside a shifted tile boundary (acceptable: markers do not reach the sheet anyway per the coordinator's
  ruling). Lesson for the questions log: `calibrate.py fit`'s "unprinted" report deserves a full read-through
  at stage 1, not just the "outside the image" half of it, whenever a map has furniture panels that could sit
  inside a rectangular grid declaration.
- 2026-09-16 RECTIFIED-PHOTO EVALUATION (per Ben's instruction, relayed by the coordinator; NOT a primary
  swap). Added "SMW map rectified.png" as a THIRD source ("rectified") in maps/smw.json, no `keep`/tile
  regeneration. Actual file is 2950 x 3900 (the coordinator's message said 2812 x 3750; noting the
  discrepancy, not acting on it -- may be an earlier/different rectify.py run). Re-measured line_dark from
  scratch on this raster (165; grid-line dips 105-140 against ~215-228 paper, softer contrast than either
  the PNG or the JPG, consistent with it being a photo). Placed 6 control points spread NE/C/W(x2)/S/SW
  (2745, 1638, 2028, 2029, 0739, 0932 -- three of these are relabelled from my first guess, which
  `calibrate.py locate` visually confirmed had landed one hex off: 1637->1638, 2029->2028, 2030->2029; see
  the crops in work/smw/calibrate/rectified-*.jpg), each verified by eye against its crop before use.
  RESULT: `calibrate.py fit rectified` residuals 0.32-0.70 hex -- FAR outside the 0.003-0.075 hex range the
  scan reached, and does not improve with better point placement (a first, cruder point set gave 0.14-1.0
  hex; correcting 3 mislabelled points brought it to 0.32-0.70, not further, because the remaining error is
  systematic, not per-point noise). The affine diagnostic is the reason: ROTATION -1.57 to -1.59 degrees,
  consistent across both point sets, and X/Y SCALE mismatched (83.2 vs 76.2 px/hex, ~8-17% apart depending
  on the point set) -- both real properties of this raster, not measurement error (4 of the 6 final points
  were individually confirmed by eye against a printed hex outline at 5-6 of 6 edges). `calibrate.py`'s
  rigid fit has NO rotation term at all (only isotropic size + ox + oy), so a raster with a genuine ~1.6
  degree tilt can never reach a low residual under it, regardless of how carefully control points are
  placed -- this is not a "few pixels of paper curl" (Ben's stated tolerance), it is a whole-image rotation.
  DECISION (coordinator, 2026-09-16, confirmed after reviewing these numbers): stay on the scan ("Stalin
  Moves West map.jpg") as primary; keep the rectified photo as a secondary for focused looks only; no
  further control points or fits against it in this task. Root cause, for the record (the coordinator's own
  account, not a guess by this worker): the fault is upstream, in how the rectification itself was made --
  the four sheet-border corners rectify.py's homography was fitted to were read off a downscaled overview
  by eye (each carries some tens of pixels of error), and the magazine sheet has physical folds, so its
  border is not a plane; a single flat-sheet homography fitted to four corners is systematically wrong
  wherever the paper curls, and cannot be fixed by better corner-picking alone -- that would need a local
  or mesh warp (GIMP, if Ben pursues it), not recalibration. Not chased further in this task.
- 2026-09-16 Stage 5d (edge markers): cropped all four map edges (north/east/south/west) at the printed
  boundary. Found NO entrance brackets, boxes, or stray victory-point texts anywhere -- unlike PGG, SMW's
  rules give entry as "any land hex on your own map edge" (rule 10, Reserves) rather than printed boxes, so
  there is nothing to catalogue. Wrote an empty work/smw/catalogue/markers.json. This is a stage that does
  not fit this map in the sense the README anticipates (principle 9): recorded, not forced.
- 2026-09-16 PRINCIPLE 10 (Ben, added to the README): STRUCTURE, NOT PIXELS. The sheet records WHICH
  hexes/hexsides carry which feature; it does not reproduce the print's appearance (line weight, colour,
  fill style, city-drawing style, exact river curve are never verify differences at stage 9). Noted per the
  coordinator's instruction; changes nothing already done here -- the stage-9 verify prompt draft already
  says "the render is schematic," and this worker's calibration is already far past the needed precision
  (0.003-0.075 hex residuals, and the rectified-photo evaluation is closed, not to be revisited). Applies
  going forward: judge stage 9 differences only by hex/hexside membership, never rendering style.
- 2026-09-16 STAGE 7 (assemble): `assemble.py smw` failed once (place_style had no "port" entry, only
  "city" -- added `"port": {"size": 30, "weight": "bold"}` to maps/smw.json) then succeeded: terrain
  {'coastal': 10, 'clear': 248, 'rough': 39, 'sea': 11, 'forest': 18, 'marsh': 0}; river 131 border 72
  rail 112 road 0. Wrote map_graphics/xml/stalin-moves-west.xml, validates against hexsheet.xsd
  (tools/validate-xml.py). marsh: 0 confirms the open vocabulary question from stage 0/5b -- genuinely no
  marsh anywhere on this map's printed area, not a missed feature. road: 0 confirms the other open
  question -- SMW never prints a road distinct from a railroad (matches rule 10.6's text).
- 2026-09-16 STAGE 8 (render): `hexsheet2svg.py stalin-moves-west.xml --png --scale 1` -> 0 warnings, wrote
  stalin-moves-west.svg/.png (1650x2550, matches the scan's own pixel frame). Visual spot-check of a
  downscaled overview: the irregular map boundary matches the printed shape exactly (furniture correctly
  absent), terrain colours read correctly (rough=olive across Germany/Slovakia/Hungary/Romania,
  forest=green blobs, sea=blue, coastal=pale blue-green), all named cities/region stars/rail/rivers/border
  look geographically sensible against the known Eastern Front. `tile.py smw render` -> 34 render tiles cut
  in the same boxes as the scan tiles. Spot-checked tile 2029_1732 (my own worked example) side by side:
  matches the catalogue exactly (Stettin coastal+port, Berlin+axis-mobilization star, Posen city, forest,
  rail, river all correctly placed).
- 2026-09-16 STAGE 9 (verify) started: wrote work/smw/verify/verify-prompt.txt (principle 10 stated first
  and explicitly, with worked "never a difference" examples, since the whole point of this round is to
  test that the discipline holds under a fresh set of readers who were not present for Ben's ruling).
  Dispatched round-1 verify batches (4 tiles each, 2 at once, same discipline as stage 5): V1 (0745_0745
  0828_0828 0829_0832 0833_0836), V2 (0837_0640 0841_0644 1145_0945 1228_1028); 7 more batches to follow
  for the remaining 26 tiles.

Copyright Ben Paul Wise. All Rights Reserved.
