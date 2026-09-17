Copyright Ben Paul Wise. All Rights Reserved.

# hexgames — plan of record (live tracking copy)

Approved 2026-09-12 from `2026-09-12-2002-plan-request.txt`. Dated snapshots: `doc/2026-09-12-plan.md`.
The coordinator (Fable) rewrites RESUME HERE before every delegation and after every hand-off, ticks
milestone boxes, and appends to the decision log. Workers write only their own `tasks/NN-slug.md`.

## Terms

People and agents
- **Ben**: the author and reviewer; approves plans, XSD changes and commits.
- **Coordinator** (also "Fable" in the approved plan): the main Claude Code session Ben talks to. It
  writes contracts, XSD changes, task files and this plan, reviews workers' results, and makes small
  fixes. It is the only writer of PLAN.md.
- **Worker, W1-W6**: a background Claude agent launched by the coordinator to do one task. The
  number names a lane of work, not a persistent agent: every launch starts with no memory, and its
  only continuity is its task file. Opus and Sonnet are the Claude models a worker runs on.
  W1 (Opus) hexcoord, hexsearch, hexengine; W2 (Sonnet) hexxml, hexmodel, package loader; W3 (Sonnet)
  hexrecord; W4 (Opus) the TRC module, the M6b engine API, then DS; W5 the PGG digest (Sonnet), the M6c map
  networks (Opus), then the PGG and DDaT modules; W6 (Sonnet, later) hexview and hexqt. At most five run at once.

Work tracking
- **M0-M14**: the milestones listed below; M6b and M7a/M7b split a milestone in two.
- **Phase 1 / 2 / 3**: core libraries (M2-M5), game modules (M6-M9), GUI (M10-M12).
- **Task file** `tasks/NN-slug.md`: one per delegated task; its inputs, outputs, acceptance tests,
  the worker's brief, and the worker's report. **status**: assigned, in-progress, blocked, review, done.
  **resume line**: the worker's one-line note of where it stopped, rewritten after every green build,
  so a relaunched worker can continue.
- **RESUME HERE**: the block below; the state of the work for whoever picks it up next.
- **Review gate**: a change that waits for Ben's approval before it lands (every XSD edit).
- **TODO(decide)**: a marker in code for an open design decision; each is also listed here.

Games
- **TRC** The Russian Campaign; **PGG** Panzergruppe Guderian; **DS** Axis Empires: Dai Senso!;
  **DDaT** D-Day at Tarawa.
- **Digest**: a plain-English summary of a game's rules with rule references (`game_rules/*.md`),
  written before the rules XML.

Engine and records
- **ABC coordinates**: Ben's hex coordinate system (from `panj\hexmap\tricoord`), implemented in
  `hexcoord`; printed hex ids like "KK19" are the identity everywhere else.
- **Rules / sheet / counters / package / hexsave**: the five XML languages (`hexrules.xsd`,
  `hexsheet.xsd`, `hexcounters.xsd`, `hexpackage.xsd`, `hexsave.xsd`); a package ties one game's rules,
  sheet and counters together.
- **Policy**: a pluggable rule component a game can replace (ZOC, movement, supply, combat ...).
  **Adjudicator**: a pure function that turns a position and a command into the next position.
  **PendingDecision**: a choice the rules need from a player before play can continue.
- **Golden**: a recorded game (`*.golden.xml`) that a replay must reproduce byte for byte. **Bless /
  re-record**: regenerate a golden with `hexgames_cli --record`, never by hand.
- **Ledger**: the per-game list accounting for every prose `<rule>` as Implemented, Common (engine
  default), OutOfScope or OptionalNotImplemented; `<game>_ledger_test` enforces it.
- **ctest labels**: groups of tests (`core`, `trc`, `golden`, `long` ...); `-LE long` is the fast set.

## RESUME HERE

- phase: 2 (games)                   milestone: M2-M5 done (2026-09-13); M6 (TRC) and M7a (PGG digest) in flight
- M6i PAUSED AT A CLEAN POINT (2026-09-17 00:30), tasks/13-smw-map.md -- READ ITS resume: LINE FIRST.
  W6 stopped at its time box after the spend-limit stop of 2026-09-16 22:03 and Ben's limit increase.
  Done in that session: verify round 1 has 31 of 34 records (2734_2536 2737_2540 2741_2544 2745_2545 were
  never dispatched); network gaps closed by one-hop crops, rail 13 -> 6 pieces, river 23 -> 19, border
  26 -> 19, broken rules 113 -> 75; two merge.py bugs fixed (resolutions loader ignored HEX:DIR tokens for
  inner hexsides; markers were never deduplicated, so stars and derricks rendered 2-4 times); SMW profile
  in network_check.py; roads settled (none printed); stage-10 report in the task file; CMakeLists.txt
  carries a comment, not the entry, until the sheet is clean. W1 verify reader found MARSH catalogued as
  forest (Pripyat block, rows 17-19 cols 40-44; smw.json has no marsh rule) and border bulges cut
  straight; both are in the task file for the next worker. Open: the 4 unread verify tiles, the
  verify.py feature-name mismatch ("link" vs "rail"), about 40 border/river broken rules, the 3-hex rail
  piece 1535/1736 (question for Ben). All of it is UNCOMMITTED in the working tree for Ben's review:
  CMakeLists.txt, stalin-moves-west.{xml,svg,png}, merge.py, network_check.py, tasks/13, PLAN.md, and
  the new tasks/15. Ben's ruling 2026-09-16: this run is NOT abandoned; it finishes, its output is kept.
- PENDING, NOT TO BE STARTED BY ANY AGENT: tasks/15-smw-chain-reading.md, the second map-reading process
  (chains end to end instead of tiles; one worker, no sub-agents; image-read ledger with caps; one stage
  per session that Ben launches himself). Written 2026-09-16 after the analysis of the tile process.
  Stage S (two scripts, no images) comes first and needs Ben's go. The tile process's SMW run (task 13)
  continues to its end and its output is kept; the trial writes only under work/smw/chain/.
- PENDING, all recorded: M6j (tasks/14-sheet-layers.md, structure/style/layout split, mechanism A chosen
  2026-09-16, XSD proposals are a review gate, after M6i); the symbol shape/meaning proposal in "XSD
  proposals awaiting review"; M6f TRC accuracy; M6g Dai Senso round 2; M6h markers option B; M15 card AI.
  Also proposed and not yet applied: calibrate.py should fit an affine or projective mapping instead of a
  rigid one, and its gate be restated as "every printed hex unambiguously identified" (decision log
  2026-09-16); and the README should stop calling network_check report-only -- on the SMW and PGG evidence
  a broken network invariant is EVIDENCE OF MISREADING and demands a look, presumed a gap until a crop
  shows a printed end (Ben has not ruled on this last one).
- M6e terrain follow-up DONE (W5b, 2026-09-15, 13:45-19:45; W5 had stalled twice
  on a 54 MB transcript, so a fresh worker took it with a short brief). From Ben's labelled hexes (water
  1402 1602; forest 1601 1701 1702 1801 1902 2201 0816 1118 1218 1808 2008) the "at least half" rule was
  simply wrong for this map: his forests measure 0.266-0.487 and his water 0.308-0.340. Woods and lake
  thresholds are now 0.24/0.35/0.15 (were 0.50/0.60/0.30 and 0.50/0.60/0.20), swamp is tested before
  woods, and the lake mask is opened 9 px so the light-blue compass rose in 5527 stops reading as lake.
  The masks were otherwise sound (a colour census found no beige or green left out). 119 hexes clear ->
  woods, 4 -> lake (0502 1402 1602 5503), none the other way; sheet now clear 1433, woods 356, lake 15,
  swamp 25. 0703 is a named exception, kept clear: its lake fill is 0.315 but the printed road runs down
  its land side. All gates passed: merge 219 resolutions 0 open, render 0 warnings, verify r1 0
  differences, network_check 0 broken on PGG/TRC/DS (3 new PGG exceptions for the river along 1602's
  shore), validate-xml 11 valid, banner-check 0. ctest 224/224. Two goldens re-recorded with --record:
  overrun (spent 12 -> 14, 2409 now woods) and full-turn, whose scenario script needed new paths for the
  26th and 27th Armoured around the new woods at 0309 and 0213. README lesson added under "Stage 9 -
  verify". Awaiting Ben: review of the full-turn.script.xml paths, open questions 1-5, and new question 6
  (nearest misses just under 0.24: woods 5808 0.229 and 4516 0.220 under a railway, lake 0503 0.228).
- pending (2026-09-15): is image2sheet fit for a Sonnet worker with little guidance? The process is written
  up in map_graphics/xml/tools/image2sheet/README.md (10 stages, commands, gates, timings, pitfalls). Three
  gaps closed today: examples/pgg/ now tracks the pilot's filled-in reader and verify prompts, two worked
  catalogue records and the marker pass (work/ is gitignored, so nothing survived a run); maps/template.json
  documents every configuration key and check_config.py reports missing or inconsistent keys before stage 1
  (it errors on a terrain threshold left at exactly 0.5, the mistake behind the follow-up); the ask for the
  person's labelled hexes moved to stage 0 step 5, ahead of any threshold. The verify prompt now has a
  template like the reader prompt. What is NOT settled: stage 4 (designing masks and thresholds on a new
  print) and stage 5a (reading two hard tiles to write the decision rules) are judgment work, and the whole
  process has run once, on Opus, with the coordinator fixing the renderer and restarting stalled workers.
  TEST BEFORE TRUSTING IT: give a Sonnet worker the README alone and log every question it has to ask; those
  questions are the remaining holes. Ben chose the map (2026-09-15): Stalin Moves West (C:/Library/War-Games/
  Stalin Moves West, map 1650 x 2550), smaller than DDaT and closer to PGG in spirit -- rail, roads, rivers,
  cities, ports, marsh, forest, rough, national borders -- written as tasks/13-smw-map.md (M6i). His stage-0
  rulings: the two red dashed line styles (long-long = the start-of-game Soviet front line, which is also the
  USSR border, as at 1838-1839 west of Brest-Litovsk; long-short = national borders) both become the same
  `border` element, the front-line distinction dropped on purpose; the part-water city hexes Stettin 2030,
  Danzig 2234 and Konigsberg 2235 are land with a port glyph, never water; the charts round the playing area
  are furniture to clip away. His wider point, now principle 9 of the README and a new stage 0 step: there is
  a BASE process, customised per map after a scan for puzzling elements, not a universal one; each map ends
  stage 0 with a written decisions list, settled with the person who knows the game. Dai Senso (M6g) still
  needs two grids on one sheet in the tools first.
- 2026-09-16 SMW sources, and what rectification was worth. Ben cannot rescan (the sheet is about 18 x 24
  inches) and deleted the 1.5x "expanded" PNG mid-run, which stopped W6 until it re-based onto "Stalin
  Moves West map.jpg" (1650 x 2550, stable since May): the process had no account of a SOURCE IMAGE
  CHANGING under a part-finished map, now a questions-log finding, with W6's own note of what survives a
  re-base (printed ids, vocabulary, decisions, catalogue records) and what does not (line_dark, colour
  masks, the fit). Ben then photographed the sheet ("SMW map in detail.jpg", 4000 x 3000), visibly sharper
  per hex. I wrote tools/image2sheet/rectify.py -- a homography fitted to four corners of a printed
  rectangle, with --margin to keep what lies outside, added after my first run cut an edge off. VERDICT:
  not worth it on this map, and I oversold it before measuring. The photograph was already nearly
  square-on (corner angles 87.1-95.1 deg, opposite sides differing 0.1% and 2.6%), so there was almost
  nothing to correct, and W6's calibration of the rectified raster gives residuals 0.32-0.70 hex against
  0.003-0.075 on the scan, with a 1.6 deg rotation and an 8-17% x/y scale mismatch in the affine
  diagnostic. Causes are mine, not W6's: the corners were read by eye off a downscaled overview, and a
  magazine sheet has folds, so its border is not a plane and one homography cannot fit it. DECISION: the
  scan stays primary, the photograph is a secondary source for focused looks, no more effort on
  rectification during M6i. Two tool findings worth keeping: calibrate.py's rigid fit has no rotation
  term, so a rotated raster cannot be rescued by better control points; and a curled sheet needs a local
  or mesh warp, not a homography. rectify.py stays in the toolset for a genuinely angled photograph.
- 2026-09-16 Ben's correction, and it is the important one: THIS IS AN XML-CONSTRUCTION PROBLEM, NOT AN
  IMAGE-MANIPULATION PROBLEM. The sheet's hex grid is perfect by construction, so nothing needs to line
  up in pixels. All the image has to do is ADDRESS: if a reader can see a city in hex 2030 of a distorted
  photograph, the sheet gets a city at 2030, and it lands correctly because the XML grid is ideal. A
  warped raster is never required; I spent an hour on rectify.py before seeing this.
  What follows, measured on SMW's own control points (residual, worst point, in hexes):
    source              rigid (what calibrate.py fits)   affine (6)   projective (8)
    scan, 13 points     0.075                            0.023        0.023
    photo, 6 points     0.701                            0.384        0.174
  So the photograph the rigid fit could not use is addressable to 0.17 hex under a projective mapping,
  with no warping of the image at all. The defect is calibrate.py's MODEL (size, ox, oy: isotropic, no
  rotation, no shear), not the raster; its own affine diagnostic already computes the numbers it then
  discards. PROPOSED, after M6i (changing the geometry layer under a running worker would invalidate its
  tiles and candidates): calibrate.py fits an affine, or projective, image mapping; common.py applies it
  at the boundary as a wrapper over the renderer's ideal Grid, which does NOT change (hexsheet2svg.Grid
  stays the sheet's geometry, untouched); tile.py, crop.py, candidates.py and overlay.py keep calling
  grid.centre/polygon and get transformed pixel coordinates. Also reconsider the gate: 0.1 hex per
  control point is stricter than the work needs. The real requirement is that every printed hex is
  UNAMBIGUOUSLY identified (about 0.3 hex, since beyond half a hex an id lands in the neighbour) and that
  crops centre well enough to read; state the gate in those terms, with the tight number kept only where
  a measurement (terrain coverage) depends on it.
- 2026-09-16 Ben settles the precision question, and it governs the gate proposal above: STRUCTURE, NOT
  PIXELS. The old process never matched the print pixel for pixel and that was never a problem -- PGG's
  rendered rivers are much thinner than the printed ones, deliberately, and Ben's instruction is to leave
  them alone: "do not try to fix this non-problem". The sheet records which hexes and hexsides carry
  which feature, not the print's appearance. So drop any effort aimed at precise matching: no warping or
  retouching of source images, and a geometric fit is good enough once every printed hex is unambiguously
  identified. Now principle 10 of the image2sheet README. This retires the pixel-accuracy ambition behind
  calibrate.py's 0.1 hex gate; when the fit model changes after M6i, the gate is restated as identity,
  and the one place a tight number is still earned is a measurement that feeds a decision (terrain
  coverage shares), not alignment for its own sake.
- M6e (tasks/12-pgg-map.md) was at status: review before the follow-up: calibrate 15 control
  points, worst 0.008 hex, grid 59 x 31; 120/120 tiles; merge 102 resolutions, 0 open; verify 0 differences;
  terrain audit of 282 near-threshold hexes, 0 corrections; Ben's known errors fixed with before/after crops
  (work/pgg/report/); network_check 0 on TRC, PGG, DS; W5 ctest 224/224; six PGG goldens re-recorded (full-turn
  script re-routed on the printed terrain); TRC goldens unchanged; README 346 lines with per-stage timings.
  Coordinator re-checked network_check, XML, banners, TRC goldens and two crops (Orsha, columns 57-59);
  staged. OPEN FOR BEN (tasks/12 "Open questions"): (1) VP hex codes in the rules XML name old/label hexes
  (Smolensk 2216+2217, Vyazma 4015, Kaluga 5921, Gzhatsk 4607, Vitebsk 0412+0513, Mogilev 0424+0525,
  Roslavl 2626) -- change codes and PggFacts; (2) entrance box "W,1" (Russian) vs "W, 2" (English); (3)
  terrain rule (b) for one-hex woods symbols (24 hexes) -- agree or keep the half rule alone; (4) the
  Russian map draws no road across any river, so German road supply cannot reach 0120 -- W5 recommends
  an engine rule joining a road that ends at a river to the road across; (5) re-place river-name labels
  (e.g. "Вопь" where no river runs) or leave them as decoration. Plus Ben's own hex-by-hex check today.
- M6e history: tasks/12-pgg-map.md (W5, Opus by Ben's choice), the pilot
  of the image2sheet process; W5 keeps "Process notes for Sonnet" in the task file and folds them into
  map_graphics/xml/tools/image2sheet/README.md so later maps (TRC, DS, DDaT, out-of-sample) can run on
  Sonnet from the README alone. 2026-09-14 18:xx the laptop's phone hotspot dropped: W5 and its
  row-by-row reader helpers failed (API unreachable) after stages 1-4 and 8 of 120 tile readings.
  19:30 resumed in its own conversation; told to re-read partial tiles and to run fewer helpers at a time
  so that a drop costs only a few tiles. Then stopped again on Ben's session usage limit (reset 19:40)
  with 8 of 120 tiles read; resumed 19:49, at most two reader helpers at once.
- M7b: M7b (tasks/11-pgg-engine.md, W4, 850k+769k tokens) is at
  status: review: ctest full 224/224, -LE long 221/221, 0 warnings; TRC goldens byte-identical; games/pgg
  (engine library, 8 test suites, scenario/pgg-1941.xml, six goldens); ledger 0 OutOfScope, 3 optional not
  implemented. Two small engine changes: E1 a combat result can hand the engine a game obligation
  (OweEffect), E2 a GameChoice names the side that answers. Rules XML gained 34 steps, three spaces and a
  turn-1 "set-up" phase. For Ben: open questions 1 (E1/E2), 2 (markers kept in PggState, not on the map,
  because the engine treats any enemy counter as a unit), 3 ("set-up" phase vs an engine option to run
  the starting phase's enter steps), 6-8 (simplified owner's choices, one target per battle, one move
  per unit per phase). Map data gaps W4 found go to M6e.
  Ben's answers (2026-09-14): the turn-1 "set-up" phase is OK; the simplified owner's choices, one
  target per battle and one move per unit per phase are OK. Still open: E1/E2 (coordinator to explain)
  and markers (Ben: markers are very common; the rules should let non-combat, information-only icons
  be placed on maps; coordinator to recommend). Map gaps are errors in the PGG sheet, not in the
  rules: Orsha should be a junction of four railways and three rivers (Cyrillic PNG) but the sheet has
  a short rail stub and one river; column 59 is missing because the sheet's grid is too narrow; the
  south-edge rail and the entrance areas are the same kind of error. All go to M6e, which waits on the
  coordinator's advice on building accurate sheets from images.
  Later the same day Ben ACCEPTED E1 (OweEffect: a combat result the game settles, pushed on the
  resolution stack) and E2 (GameChoice names the side that answers), and chose markers option B (M6h).
- 2026-09-14 Building sheets from images (Ben accepted the coordinator's advice): the old method (colour
  sampling, reading shrunken crops, plausibility checks, tidy tools that re-author features) matched
  the print badly. New method, written as a repeatable process for PGG first, then DDaT, TRC, DS and
  out-of-sample maps: verify the grid geometry against printed hex numbers across the whole map; tile
  a grid-and-id overlay at full resolution; catalogue each tile into JSON with a fixed vocabulary (hex
  terrain and places, hexside lines, per-hex rail/road exits), recording only what is printed; cross-
  check tile overlaps and neighbour exits; assemble XML with a plain script (no re-authoring); verify
  render tile beside scan tile until no differences remain; image processing only proposes
  candidates. Process: map_graphics/xml/tools/image2sheet/README.md; first run: tasks/12-pgg-map.md. M6d (tasks/09-ds-map.md, W5, 731k tokens) is at status:
  review: network_check 1989 -> 0 broken on dai-senso.xml (TRC, PGG still 0); ctest 185/185; files
  staged. Coordinator's visual check against pic4573603.png: India rail connected, Nepal border and
  Himalaya ridge right, but rivers are short loops round single hexes instead of the Indus and Ganges
  courses; central Pacific rings closed, but the Gilbert ring has an extra southward leg, and islands
  are drawn as whole land hexes (115 sea hexes made clear) where the print shows sea hexes with island
  marks. Eight open questions in tasks/09. Waiting on Ben's look before any fix round.
- worker lesson (2026-09-14, twice: W5 on M6d, W4 on M7b): a background worker that starts its build or
  ctest with run_in_background and then ends its turn never sees the result and stops without
  reporting. Every worker brief must say: run builds and ctest in the FOREGROUND with a long timeout;
  before building, wait until no ninja, cl, link, ctest or cmake process is running, because workers
  share the cmake-build-debug tree.
  Coordinator: games/trc/tools/trc_control.py (M6c item 4) and the hexview contracts (M10 prep).
- M6c: tasks/08-map-networks.md (W5, opus): connected rail, road and river networks on
  the TRC and PGG sheets, SVG/PNG regenerated, TRC scenario rail and goldens re-recorded to match.
  2026-09-14: networks done (network_check 0 broken on both sheets; ctest 185/185; goldens supply,
  rail-move, full-turn re-recorded, two golden scripts moved off removed rail; 5 tests updated, 3 of them
  outside games/trc). Borders done too: TRC border 153 hexsides in 3 pieces, matching the TRC v5
  deluxe map (which also shows the German frontier, a Kaunas-Baltic line and Bulgaria's edges; kept);
  goldens unchanged by the borders. Coordinator re-checked: network_check 0 broken on both sheets,
  only supply/rail-move/full-turn goldens changed, XML valid, banners 0. status: review, W5 432k+374k
  tokens. Waiting on Ben: local ctest (185 full), the seven open questions in tasks/08, then commit.
  After M6c: smoothed roads, railways and rivers in hexsheet2svg.py (Ben approved; TRC and PGG
  re-rendered). Next in flight: tasks/09-ds-map.md (M6d, W5 fresh, opus). Engine tasks renumber:
  Dai Senso engine (M8) and PGG engine (M7b) become tasks/10 and tasks/11, written after M6b/M6c commit.
  M6b is staged (105 files) for Ben's commit; W5 makes no git writes, so the index stays M6b only.
- M6b: tasks/07-engine-api.md (W4, opus, 575k tokens) was at status: review
  (2026-09-14): W4 full ctest 180/180 with one skip (PendingSaveTest, waits for the hexsave proposal);
  goldens byte-identical (0 golden files changed); style clean; 11 XML valid; banners 0 failures.
  Coordinator review done; Ben answered 2026-09-14 (decision log "M6b review"). W4 applied them
  (review round 1, +719k tokens): ctest full 184/184, -LE long 182/182, no skips; only
  trc-test.golden.xml re-recorded (one line: the weather-drm flag; cite "M6b review: no side flags
  without a game module"); hexsave.xsd <resolution> applied; 11 XML valid; banners 0 failures.
  Open for Ben: on engine defaults, steps whose @commands verb the default grammar lacks (TRC's
  rail-move, 5 steps) are withheld and listed rather than refused. Then Ben builds, commit M6b,
  write tasks/08 (DS) and 09 (PGG).
- in review: tasks/05-trc-engine.md (W4, opus) -- status: review 2026-09-14, W4 ctest 165/165
  (683k tokens). Coordinator review: scope and style clean (no unordered_/assert/default:/mutable
  statics in new code); golden diff consistent with a re-record (rules 13.3). Engine API grew more
  than "minimal": GameAdjudicator hook, CombatPlan in Position, string-keyed per-side flags in
  Position -- for Ben. Ben's local ctest -LE long 163/163 (2026-09-14). All work staged (117 files);
  next: commit, split into coordinator changes, M6 (TRC) and M7a (PGG, after Ben's PGG decisions).
- in review: tasks/06-pgg-digest-rules.md (W5, sonnet) -- status: review 2026-09-14; outputs
  untracked, not staged. Coordinator review fixed two transcription errors in the PGG rules XML (CRT
  rows 3-6 against crt.txt; Rzhev 5 VP, not 10). Awaiting Ben on the PGG open questions below.
  W5 died three times reading the 6.6 MB scanned rulebook PDF directly; the relaunch worked from the
  coordinator's pdftotext extraction (scratchpad, not kept) -- for DS/DDaT digests, extract first.
- next coordinator action (2026-09-14): M6 and M7a are done and staged (Ben committing). Ben chose
  the post-M6 engine API (decision log: steps in the rules XML, game-owned typed state, resolution
  stack); tasks/07-engine-api.md (M6b, W4, opus) is written, NOT launched. Ben reviewed and approved
  the hexrules.xsd step and concealment elements (2026-09-14); ready to launch on Ben's word. Then tasks/08 (M8 DS, W4, 2014 living rules) and tasks/09 (M7b
  PGG engine, W5) on the new API.
- worker spend so far: W1 217k + 475k, W3 286k, W2 618k, W5 ~290k (+ three failed starts) (~1.9M)
- 2026-09-14 coordinator changes (staged, not yet committed): hexcoord offset="even" keeps the odd
  grid's ABC origin (GridTest 11/11); TRC rules XML hostile-to written out; three class .puml files
  fixed (one-line bodies); BoardBuilder parses orientation/offset exhaustively (throws)
- blockers: none (Ben reviewing hexsave.xsd and hexpackage.xsd; parked XSD proposals unchanged)
- last green: ctest --preset win-msvc-debug -LE long 163/163 (Ben, 2026-09-14, all staged work);
  full ctest 165/165 (W4)   last commit: 0eccf0b
- crash protocol: read this block, then every `tasks/*.md` with status assigned|in-progress|blocked,
  then `git status`; continue from the task files' `resume:` lines. Nothing lives only in chat.

## Milestones

- [x] M0  PLAN.md, .gitignore, CLAUDE.md, BUGS.txt, CMake skeleton, presets, gtest + TinyXML2 fetch,
          banner check, XSD ctest, .clang-format, tasks/ protocol, smoke test
- [x] M1  Contracts (F): interface headers (hexcoord, hexmodel, hexsearch, hexrules, hexengine, hexrecord),
          PlantUML `[PROPOSED]` (3 class, 4 sequence), test lists, hexsave.xsd, hexpackage.xsd,
          TRC test scenario + trc.package.xml, five forward design docs (01 02 04 05 08)
- [x] M2  hexcoord (W1, 2026-09-13): ABC strong types, Direction, Grid/HexIdFormat, pixel mapping,
          testtri port, four-sheet pixel test — 27 tests, ctest 35/35
- [x] M3  hexxml + hexmodel + hexrules loaders (W2, 2026-09-13): TinyXML2 facade, five document
          models, Quantities/Board/Roster/Position, Board/Roster/Position builders, RuleSetBuilder,
          Ledger, PackageLoader; TRC package loads and checks clean; ctest 90/90
- [x] M4  hexsearch + hexengine (W1, 2026-09-13): scratch/Field/algorithms, Session, PhaseCursor,
          PRNG streams, events, default policies, adjudicators, determinism + 16-thread parallel
          rollout test, hexrecord M4 glue, hexgames_cli, first TRC golden (with a pending-decision
          round trip); side binding wired; ctest 131/131
- [x] M5  hexrecord (W3, 2026-09-13): SaveModel document model, hexsave reader with document-level
          checks, canonical writer (validates), LCS diff and golden report; 15 tests. Session glue
          (readRecord/writeRecord/replay/sessionFor/compareWithGolden) compiles with `// M4:` markers;
          replay tests deferred to M4
- [x] M6  TRC engine module (W4, 2026-09-14): CombatPlan + GameAdjudicator in the engine, 27-file
          trc_game, 43/54 rules implemented, 1941 scenario (derived start line), six goldens; ctest
          165/165. Provisional data marked TODO(decide); see tasks/05 open questions
- [ ] M6b Engine API after M6 (W4): phase steps declared in the rules XML, game-owned typed state,
          one resolution stack; TRC goldens byte-identical (tasks/07-engine-api.md)
- [ ] M6c Map networks (W5): connected road, rail and river networks on the TRC and PGG sheets; network_check
          in ctest; TRC scenario rail and goldens re-recorded to match (tasks/08-map-networks.md)
- [ ] M6d Dai Senso map cleanup (W5): continuous roads per landmass and across the grid seam, border and
          zone lines, mountain ranges, places checked against the DS images (tasks/09-ds-map.md)
- [ ] M6e PGG map accuracy (task written, not launched: tasks/12-pgg-map.md, first run of the image2sheet
          process in map_graphics/xml/tools/image2sheet/README.md): carefully compare the generated PGG sheet (XML, SVG, PNG)
          with the PNG/JPG references in C:\Library\War-Games\Panzergruppe Guderian and repair it feature
          by feature, as M6d does for Dai Senso. Known example (Ben, 2026-09-14): in the Russian redesign
          map ("Panzergruppe Guderian map Russian redesign.jpg", same image as "PGG map, Russian.png")
          rivers run down around columns 48/49 and 57/58, with twists and angled stretches, which the
          sheet does not show. Do after M7b (the PGG goldens it records depend on the sheet); re-record
          them afterwards, citing "maps: PGG accuracy". Data gaps the M7b engine found (PggFacts::dataGaps,
          asserted by pgg_package_test, so fixing them will fail that test until it is updated): no road
          reaches the German supply hex 0120 (German supply works only through the 20-point trace);
          Victory Point hexes 5907 and 5915 lie beyond the sheet's 56 columns; no rail reaches the south
          edge; the entrance areas A, C-H, V, W, X, Z and 1-6 are provisional hexes (only B is in the text).
- [ ] M6g Dai Senso map, second round (pending, not started): fix what the M6d review found, and more.
          Known items (coordinator's check against pic4573603.png, 2026-09-14): rivers traced along their
          printed courses (India's Indus and Ganges came out as short loops round single hexes); the
          Gilbert Islands border ring trimmed to its printed outline (an extra southward leg); how islands
          are shown (115 sea hexes were made land; the print shows sea hexes with island marks); ports
          hidden under counters placed (Saipan, Palau, Truk, Kwajalein, Okinawa ...); place-name labels a
          hex or more off moved (VLADIVOSTOK beside Mukden); plus tasks/09's eight open questions and a
          fresh region-by-region comparison. PART OF THIS TASK, before any fixing: the coordinator advises
          Ben on a more effective way to build XML map sheets from PNG/JPG/PDF maps (the M6c/M6d method --
          colour sampling, per-region reading of small crops, hand-authored guide data, network tidying --
          was slow and still inaccurate); compare two or three approaches and recommend one.
- [ ] M6h Map markers, option B (pending, not started; Ben chose it 2026-09-14): markers become their own
          kind of object, never units -- placed on a hex, a hexside, a unit, or a space or track, with a
          type, an optional owning side, an optional value (track markers) and an optional counter for
          the artwork; one ordered list in Position (digest, fork); rules XML unit-type kind="marker"
          gains where a marker may be placed and how many exist; hexsave gains <markers><marker .../>
          and unit/@status tokens move there (both XSD changes proposed for review first); PGG's
          air-interdiction, Soviet-interdiction, disruption and rail-cut markers move out of PggState;
          TRC's unit tokens migrate; hexview's Markers layer draws them. Before more game modules
          depend on the workaround.
- [ ] M6f TRC map accuracy (pending, not started): carefully compare the generated TRC sheet with the PNG
          references in C:\Library\War-Games\The Russian Campaign (the three TRC v5 deluxe maps, TRC map v1
          adjusted.png) and repair it feature by feature: rivers, rail, borders, terrain, cities. Fold in
          M6c follow-ups 1-3 (out-of-grid ids, land cities, guide data file). Re-record TRC goldens after,
          citing "maps: TRC accuracy".
- [ ] M6j Structure / style / layout split in the map and counter languages (tasks/14-sheet-layers.md,
          pending, not started; Ben 2026-09-16). One document mixes three kinds of semantic content, so an
          image-driven workflow leaked rendering data into structure: `grid/@size @ox @oy` are required and
          come from the image calibration, so every sheet embeds one scan's pixel frame, and `label/@x @y`
          are written straight from source-image pixels. Ben's acceptance test: changing a river's width
          touches ONLY a style file, not layout and never structure. Recommended mechanism: three bound
          documents (map, style, layout), with the engine reading structure alone. Includes the symbol
          shape/meaning split, and the counters language's `Tile` fill that encodes a year. First step is
          an audit of what SheetDoc and BoardBuilder actually consume (not checked). XSD proposals go to
          Ben first: review gate. After M6i. MECHANISM A CHOSEN BY BEN 2026-09-16: three bound documents.
          It beat one-document-three-sections on sharing (TRC and PGG can share one operational style),
          substitution (screen against print, Qt6 against SVG, with no edit to the map), enforcement (the
          map schema has no concept of a stroke width, so no tool can write one into a map file), and a
          map git log that carries structural change only. Fixtures may keep an inline style.
- [ ] M7  PGG digest + rules XML (review gate) + package + scenario; PGG engine module. M7a (digest, rules XML,
          package, test scenario) done 2026-09-14; M7b engine module (W4, not W5 as first planned) in flight,
          tasks/11-pgg-engine.md
- [ ] M8  DS engine module (W4)
- [ ] M9  DDaT engine module (W5)
- [ ] M10 hexview (W6): geometry, scenes, faces, SVG goldens, InteractionMachine, replay. Must reproduce
          the reference renderers' look (smoothed links, rounded rivers, straight boundaries, casings,
          dashes, ticks, terrain colours) for both Qt6 and HTML; see decision log "Visual parity"
- [ ] M11 hexqt (W6): Viewport, painters, MapView, panels, GameSession, AiTurnRunner, viewer
- [ ] M12 trc_gui, pgg_gui, ds_gui, ddat_gui + GUI tests + script/snapshot playback (W4/W5)
- [ ] M13 hexgames_bench + sanitizers, diagrams `[AS BUILT]`, docs snapshot 2, Debian build (F)
- [ ] M14 two out-of-sample games
- [ ] M15 Card-driven "AI" opponents (pending, design not started): the kind of card- or chart-driven bot common
          as the opponent in solitaire wargames (a deck or table of prioritised instructions per phase,
          with conditions, target selection and tie-breaks, as DDaT's Japanese action deck already is).
          Two uses, one design: (a) an opponent a human plays against in the GUIs, and (b) a player for
          running hundreds of computer-vs-computer games headless (hexgames_bench, M13) to debug the game
          engines -- rule coverage, crashes, stuck phases, determinism, statistical sanity of outcomes.
          First step is a design for Ben's review: how bot instructions are expressed (rules XML or a new
          document, vs. code), how they plug into Player/PendingDecision without blocking, how randomness
          uses the named PRNG streams, and how batch runs report problems.

## Decision log

- 2026-09-12 Graph library: none (hand-rolled over the ABC grid). OGDF rejected (GPL); CXXGraph would be
  the fallback (MPL-2.0).
- 2026-09-12 ABC re-implemented in `hexcoord`; `panj\hexmap` untouched. One algebra for both orientations;
  orientation lives in the pixel mapping and offset constructors.
- 2026-09-12 Pilot order TRC, PGG, DS, DDaT.
- 2026-09-12 XML parser TinyXML2 via FetchContent, confined to `hexxml`; XSD validation by Python/lxml
  under ctest (`tools/validate-xml.py`).
- 2026-09-12 Human player = prompt/submit; PendingDecision, never blocking.
- 2026-09-12 Region membership goes into the sheet XMLs' existing `<region>` element (instance change,
  flagged for review); the package manifest binds names only.
- 2026-09-12 No XSD change needed for ABC. New schemas: hexsave.xsd, hexpackage.xsd. Every XSD edit is a
  review gate for Ben (XML Copy Editor) before it lands.
- 2026-09-12 Style: irrgo layout (`#pragma once`, PascalCase files) with visolver's `.clang-format`
  (2-space). Banners enforced by `tools/banner-check.py`.

- 2026-09-13 hexcoord (W1 review): a second grid on a sheet is placed by `Grid(GridSpec, const Grid&)`
  reading the offset from the two pixel origins, tolerance `kLatticeTolerance = 0.05 * size` (the
  1e-6 in the plan was unrealistic: scan-fitted origins put Dai Senso's east grid 0.0012 * size off).
  Offset frame exposed as free functions `abcOfIndex`/`indexOfAbc` (the only place offset coordinates
  exist). Pointy offsets derived from the renderer: row 2m -> (col, -3m, -col), row 2m+1 ->
  (col+1, -(3m+1), -col); pointy basis = flat basis turned 30 degrees clockwise on screen.

- 2026-09-13 M3 review: SheetDoc keeps eight same-kind vectors (hexes, hex, edge, path, link, region,
  label, panel) in document order each, not one interleaved list -- accepted, nothing needs the
  cross-kind order. BoardBuilder/RosterBuilder/PositionBuilder are declared in HexModel (friends) but
  compiled into the hexrules target because they need RuleSet -- accepted for now; TODO(decide) move
  them to hexrules with `friend class HexRules::X` forward declarations. trc.package.xml unit
  bindings now accept the `-N` suffix of repeated printings (26 counters).

- 2026-09-13 M4 review (engine defaults a game module overrides; obligations for M6):
  (a) an empty hostility matrix (no @hostile-to anywhere, as in TRC) means every other side is an
  enemy -- engine convention, no XML change; (b) combat decisions are ONE round trip: Position has a
  single PendingDecision slot, so the defender's loss/retreat choices are settled by a fixed rule
  (fewest steps, then lowest counter id) -- TRC 13.3 gives the defender the choice and the attacker the
  routing, so M6 must add a CombatPlan to Position (or a queue of pending decisions) and chain them as
  seq-combat-with-decision.puml draws; (c) the default supply check only marks isolatedP -- @fatal
  elimination is a game rule; (d) BoundedTrace's default source set ("a controlled hex with a drawn
  feature") is a placeholder -- M6 defines TRC's sources (friendly cities; rail chains to cities or the
  owner's edge); (e) DefaultPhaseGate derives caps from phase names -- M6 supplies a real gate;
  (f) defaultValueLine reads "8-7" as combat/combat/allowance -- correct for TRC; (g) cmake label
  fix: multi-label ctest registration works now (`ctest -L trc` finds tests).

- 2026-09-14 ABC axes (Ben): A is due east, B north-west, C south-west, as drawn in
  `doc/hex-ABC-coord-example.svg`. This is the algebra `hexcoord` already follows; the "NorthEast" /
  "SouthEast" labels in `panj\tempest\doc\coords.txt` are wrong.
- 2026-09-14 TRC hostility (Ben): written out explicitly in the-russian-campaign.xml
  (`axis hostile-to="russian"`, `russian hostile-to="axis"`). The engine's empty-matrix convention
  stays for documents that omit it.
- 2026-09-14 offset="even" (Ben): "offset" means shifted down (flat) or right (pointy). An even grid
  keeps the odd grid's lattice and ABC origin (`doc/hex-ABC-offset-odd-down.svg`,
  `hex-ABC-offset-even-down.svg`); the origin need not be a printed hex. (ox, oy) keeps
  hexsheet2svg.py's meaning, so on an even grid the ABC origin is half a hex before it along the
  shifted axis. `hexcoord::Grid` now matches the renderer in absolute pixels for both parities.
- 2026-09-14 PGG (Ben): Yelnya is hex 3122 (original 1976 map; absent from the Cyrillic map the sheet
  was traced from), 10 VP in the rules XML occupation list. PGG-amendments.pdf (the "Panzergruppe
  Guderian II" fan variant) is used as base rules, as W5 wrote it.
- 2026-09-14 Counter graphics (Ben): icons follow the clearest counter sheet actually played with; the
  aim is to show every TYPE of information can be represented, not to reproduce a game exactly.
  build_pgg.py: the 18 German 9-7/4-7 division counters are infantry (ids s2-<n>-inf-9-7, were
  -mot-9-7); "mot" is motorized on the German sheet and mechanized on the Soviet one.
- 2026-09-14 Hidden units (Ben): hiding and revealing are common, so the rules language handles them.
  hexrules.xsd unit-type gains @hidden-from (enemy|all), @conceals (values|identity), @reveal (list of
  attacked|attacking|combat|adjacent|rule|owner) and @rehide (never|rule); RuleSetBuilder requires all
  four on a hidden type and none on a visible one (HexRules::Concealment). PGG and Tarawa carry them.
  XSD change applied at Ben's request; review it in XML Copy Editor.
- 2026-09-14 Banners (Ben): tools/banner-check.py also checks game_rules, map_graphics and
  unit_graphics; the 29 older files there carry banners now; exempt: military-unit-icons-handoff.md
  and game_rules/xml/validate.py; a Python "#!" line may precede the banner.
- 2026-09-14 Engine API after M6 (Ben): (2) the game's sequence of play is declared as steps in the
  rules XML phase tree -- "the whole point of the rule language is to guide the engine" -- and the
  engine runs, in document order, the functions a game registers under those step ids (replaces the
  single GameAdjudicator hook; needs an XSD change). (3) CombatPlan generalises to one resolution
  stack in Position whose step kinds games extend (combat losses and retreats first; DS card effects,
  Tarawa reveal-and-consult-again later); PendingDecision comes from its top. (1) per-side flags:
  a game-owned typed state struct held by Position (option C); strings only in the hexsave codec.
  Implementation: tasks/07-engine-api.md (M6b, W4), before M8 and M7b.
- 2026-09-14 M6b review (Ben, answers to tasks/07 open questions): (1) the explicit, listed withhold
  of a game's step behaviours on engine defaults is acceptable. (5) Session build reports every
  @commands verb the grammar does not know, and goes no further unless there are none. (6) No
  bending: side flags are refused when no game module is loaded (VerbatimFlagCodec and
  UninterpretedFlags go); the engine smoke golden is re-recorded accordingly. hexsave.xsd
  <resolution> proposal APPROVED as written in tasks/07 "XSD proposals".
- 2026-09-14 M6b review round 2 (Ben): ctest -LE long 182/182. On engine defaults, a step whose
  @commands verb only a game's grammar knows (TRC's rail-move) is withheld and listed; strict in
  Required mode -- accepted. M6b staged for Ben's commit.
- 2026-09-14 Maps (Ben): the TRC and PGG sheets carry the right element kinds but have broken
  networks: road gaps, short disconnected river and rail pieces. Fix them so the road network is
  connected with no gaps and no short isolated river or rail pieces remain; physically plausible and
  pleasing, not a faithful copy of either game. Validate against hexsheet.xsd; regenerate SVG and PNG.
- 2026-09-14 TRC country borders (Ben; he first called them fortification lines): the sheet's "border"
  line (rules country-border) must be connected and match the borders drawn in the three PNG maps of
  C:\Library\War-Games\The Russian Campaign\TRC v5 deluxe: one surrounds Warsaw and divides Poland into
  two sectors, one surrounds Hungary along its mountains, one covers two edges of Rumania. Added to M6c
  (W5). Deriving country <region> membership from them is NOT in scope (it would activate four inert
  TRC rules and change play); a separate decision for Ben.
- 2026-09-14 Smoothed roads and railways (Ben): hexsheet2svg.py draws <link> networks as panj/tempest
  (src/hxsvg.cpp) does: through a hex, hexside midpoint to midpoint; at ends and junctions, midpoint to
  centre; joined into polylines between ends/junctions. Rendering only, the XML is unchanged; the C++
  renderer (M10) reproduces it. Also noted: Ben likes tempest's terrain colours (paleBeige 255,255,227;
  paleGreen 198,255,198; paleBlue 128,198,255; paleGray 227,227,227; paleBrown 178,161,144) and will
  adapt tempest's terrain synthesis later to generate new terrain with road, river and rail networks.
- 2026-09-14 Smoothed rivers (Ben: tried, "looks great", now the default; political boundaries and all
  other hexside lines stay jagged, exactly along the hex edges): hexsheet2svg.py joins each
  river's hexsides into corner chains and rounds every corner (quadratic curve from hexside midpoint
  to midpoint); ends and junctions stay on their corners; other hexside lines stay straight. ROLLBACK:
  re-render with `--straight-rivers` (Renderer(..., smooth_lines=())); the XML is unchanged either way.
- 2026-09-14 M6c open questions (Ben accepted the coordinator's recommendations): (1) remove the
  out-of-grid ids from the TRC and PGG <hexes> lists; (2) reclassify Riga F17, Helsinki C14 and Sevastopol
  KK23 as land; (3) move the river guides and border lists out of tidy_networks.py into a data file
  beside the sheets; (4) keep the TRC scenario control generator in the tree (games/trc/tools/);
  (5) keep full-turn's attack at W15; (6) the old scenario rail control list is replaced wholesale;
  (7) keep the extra borders the TRC map shows. Items 1-4 are follow-up work, done AFTER M6d, because
  M6d's worker is editing the same map tools now; item 2 may change TRC goldens (re-record, cite
  "maps: land cities").
- 2026-09-14 Dai Senso map (Ben): add the rail network (white line, grey casing) to the sheet, not only
  repair roads. Ben checks the result against pic4573603.png in two places: India's transportation
  network (the white lines), and the dashed boundaries (red/yellow country/dependent borders and
  white/grey naval zone borders) around the Eastern Carolines, Marshall and Gilbert Islands out to
  Johnston Island. Boundaries at sea are printed and correct; sent to M6d's worker. Those two regions
  are examples only: the whole map is to be fixed to the same standard (every rail and road line, every
  international and naval zone border, region borders, mountains, places), against pic4573603.png.
- 2026-09-14 Urban buildings (Ben: now the DEFAULT, "the cities in that PGG map are fine";
  `--urban-symbols` draws the old city symbol; W5 told to treat city drawing style as deliberate in
  verify; TRC render refreshed; hexview must reproduce it). EXCEPTION (Ben, same evening): Dai Senso
  keeps plain city symbols (circles, as pic4573603.png draws them) because the map is already dense;
  the sheet now says so itself (urban="symbol", XSD entry "sheet/@urban"). hexsheet2svg.py tints each hex carrying a
  city-major, city or capital glyph pale grey (tempest's paleGray #e3e3e3, in the terrain layer) and
  draws the glyph as 4-5 scattered black rectangles after panj/tempest's drawBuildings (sizes 0.2-0.44
  of the hex radius, scattered over +/-0.55 by +/-0.48 of it; a small overlap may join two into an L,
  but no rectangle more than 15% covered and centres at least 0.30 apart, so they do not lump; seeded
  by sheet id and hex id so every render is the same). Spread widened after Ben found 35% overlap made lumps. OFF by default so the M6e map pilot's renders and verify pass are unchanged (default
  render checked byte-identical). Trial render: scratchpad/urban/pgg-blocks.png. After Ben's look and
  M6e, make it the default (then hexview's SymbolLibrary/MapSceneBuilder must reproduce it too).
- 2026-09-14 Visual parity (Ben, for the future): the C++ visualization, both the Qt6 GUI (hexqt) and
  the HTML/browser client, must reproduce the graphics now being settled in the Python reference
  renderers' SVG/PNG: smoothed roads and railways (hexside midpoint to midpoint through a hex, midpoint to
  centre at ends and junctions, joined into polylines), rivers with rounded corners, political and
  other boundary lines straight along the hexsides, line casings, dashes and rail ticks, and the terrain
  colours and patterns. hexview's Scene builder (M10) owns this geometry, so both front ends draw the
  same primitives; its SVG goldens are compared against hexsheet2svg.py / counters2svg.py output.
- 2026-09-14 Parallel work while M6d runs (Ben): W4 starts the PGG engine module (M7b, tasks/11); the
  coordinator moves the TRC scenario control generator into games/trc/tools/ (M6c item 4, made
  self-contained on hexsheet2svg.Grid so it does not depend on the network_check.py M6d is rewriting)
  and writes the hexview contracts (M10 preparation: headers, test list, UML) for Ben's review.
  Deferred: the DDaT package, scenario and steps (M9 preparation) to a free worker slot.
  Done by the coordinator the same day: games/trc/tools/trc_control.py (reproduces the scenario's 224
  rail control entries exactly); hexview contracts [PROPOSED] for Ben's review -- hexview/Style.h,
  Scene.h, MapFrame.h, LineGeometry.h, SymbolLibrary.h, MapSceneBuilder.h, FaceModel.h, ViewState.h,
  StateSceneBuilder.h, Interaction.h (intents, EngineFacade, InteractionMachine), Replay.h
  (ReplayController, AnimationPlan), SceneWriters.h; doc/2026-09-12-design/test-lists/hexview-tests.md;
  uml/hexview-classes.puml. Deliberately NO hexview/CMakeLists.txt yet: the root CMakeLists adds
  hexview as soon as one exists, and uncompiled contracts must not break W4's and W5's builds; after
  Ben's review, add it with a header-compile check and build.
- 2026-09-14 (Ben) hexview/CMakeLists.txt added so the contracts can be built: target hexview_contracts,
  EXCLUDE_FROM_ALL (ordinary and worker builds skip it), built on request with
  `cmake --build --preset win-msvc-debug --target hexview_contracts`. Ben built it 2026-09-14 evening:
  no errors, and a full Debug build also clean. Also recorded as pending, not
  started: M6e (PGG map accuracy against the references) and M6f (TRC map accuracy), both after M7b. Dai Senso rules source: "Dai Senso  Living_Rules_February_2014.pdf" (67 pp, text layer).

## XSD proposals awaiting review

- APPROVED 2026-09-13 by Ben, applied (hexpackage.xsd `<side>`, trc.package.xml, kMaxSides=16); to
  be reviewed by Ben in XML Copy Editor. Original proposal: a counter's side. Unit types shared across sides (infantry, armour,
  hq...) carry no side, and hexpackage's <unit> binding has type/counters/match only, so
  RosterBuilder cannot assign UnitSpec::side generically (it currently falls back to the unit type's
  side mask, then a region's side, then a placeholder). Proposal: add to hexpackage.xsd
    <side rules="axis" styles="german ss luftwaffe rumanian finnish hungarian italian marker"/>
    <side rules="russian" styles="russian guards worker"/>
  binding the counters document's style ids (the printed ground colour, which IS the side on every
  sheet) to rules side ids; the loader then requires every unit/support/leader counter's style to be
  bound. No change to hexrules or hexcounters. Awaiting Ben.
- APPROVED 2026-09-14 (Ben reviewed): hexrules.xsd unit-type
  @hidden-from @conceals @reveal @rehide (replaces the `unit-type/@reveal` candidate).
- APPROVED 2026-09-14 (Ben chose option 2C and reviewed the schema):
  hexrules.xsd phase gains step* (@id @does @at=enter|before-command|after-command|end @commands
  @rules @turns). Optional element, so every existing rules document still validates.
- APPROVED AND APPLIED 2026-09-14 (Ben: "the XML has to explicitly choose"): hexsheet.xsd `sheet/@urban` =
  `buildings | symbol`, REQUIRED, no default. Every renderer follows the document (hexsheet2svg.py now;
  hexview for Qt6 and HTML later); the --urban-symbols flag is gone. TRC and PGG say buildings; Dai
  Senso and DDaT say symbol; the four C++ test fixture sheets say symbol. HexXml::SheetDoc reads and
  checks it. W5 told to keep it when assembling the rebuilt PGG sheet.
- EXPECTED from M6b: a hexsave.xsd proposal to store the resolution stack, so a save taken while a
  decision is pending reloads (hexsave has no element for it today). W4 proposes; not applied.
- PROPOSED 2026-09-16 (Ben's design, arising from SMW's bridges; NOT applied, awaiting his review):
  separate SHAPE from MEANING in hexsheet.xsd, so the vocabulary is the product of two small sets
  instead of one big set. Today `Symbol` is a single flat enumeration of about 30 values that mixes
  primitives (`star`, `dot`, `arrow`, `text`) with meanings (`city-major`, `capital`, `port`, `oil`,
  `tank`, `artillery`, `pier-head`, `lvt-wreck`, `fire-intense`, `ice`, `strait`). Every new printed
  thing forces a new named value: N meanings x M shapes. SMW's bridge, "a yellow rectangle exactly
  across the hexside", has no value and would otherwise add one.
  Proposal, in the language's existing declare-then-reference style (as `<palette>`, `<lines>` and
  `<terrains>` already work):
    <legend>
      <mark id="bridge"           shape="rect"    color="bridge-yellow" across="edge" size="0.5 0.12"/>
      <mark id="mobilize-soviet"  shape="star"    color="red"/>
      <mark id="resource"         shape="triangle" color="ink"/>
    </legend>
  and at the point of use `<edge at="2036-2037" mark="bridge"/>`, `<hex id="2036"><glyph mark=
  "mobilize-soviet"/></hex>`. `mark` is an IDREF into the sheet's own legend; `shape` is a small closed
  set of primitives (rect -- hence square, ellipse -- hence circle, star, triangle, diamond, bar, cross,
  arrow, text); colour, size, orientation and slot stay attributes, as now.
  One honest limit: some existing symbols are pictures, not primitives (`port`'s anchor, `tank`,
  `artillery`, `lvt-wreck`, `pier-head`, `ice`). They cannot be expressed as a shape plus a colour, so
  the proposal keeps a `pictogram` shape whose `mark` id selects the drawing; the split then covers the
  geometric marks (the growing set) and leaves the drawn ones as a small fixed library.
  Migration, if approved: hexsheet.xsd; hexsheet2svg.py's SYMBOLS table becomes primitives plus a
  per-sheet legend; the four sheets and four C++ fixture sheets convert mechanically; HexXml::SheetDoc
  and the hexview FaceModel contract follow; every SVG and PNG regenerates. No engine goldens carry
  symbols, so none should change -- to be confirmed. Recommend doing it AFTER the SMW process test
  (M6i), not during it. Interim, agreed with Ben: SMW writes mobilization hexes as `star` and resource
  hexes as `oil` (both already in the enumeration and already drawn); bridges stay in the catalogue and
  out of the sheet until this lands.
- Candidates noted in the plan: `phase/@repeat-per-side`, `phase/@caps`, `panel/@space`.

## Open questions

- PGG (W5 review, 2026-09-14):
  (a), (b) settled 2026-09-14 (decision log). Still open: whether the sheet XML should gain a town
  glyph for Yelnya at 3122.
  (c), (e) and W5's first schema proposal (hidden units) settled 2026-09-14 (decision log). Parked:
  W5's proposals 2 (a conditional side effect on a CRT result) and 3 (a per-unit segment length),
  tasks/06 "## Notes".

---

# Approved plan (verbatim, 2026-09-12)

## 1. Context

The three XML languages already in `hexgames` — `hexrules.xsd` (rules), `hexsheet.xsd` (map sheets),
`hexcounters.xsd` (counters) — were written to guide and constrain a C++20 implementation. This plan
designs that implementation: a headless, reusable wargame engine driven by those documents; a common Qt6
GUI over it, designed so a browser client can be added later without touching the engine; per-game
modules and GUIs for TRC, PGG, Dai Senso (DS) and D-Day at Tarawa (DDaT), each with a golden-record
system test; and a fourth language, `hexsave.xsd`, for saving, reloading and scripting games. Two
out-of-sample games follow once the four work.

Findings from exploration that shape the plan:
- `panj\hexmap` today is only `tricoord.h/.cpp` (CoordXYZ/CoordABC/CoordQRS algebra, `reduce()`,
  `hvCode()`, offset constructors), flat-top only, with an empty `hexgraph` stub, a dead namespace
  `operator+` that returns the origin, a dangling `doc/coords.txt` reference (the file lives in
  `panj\tempest\doc`), hard abzar/TinyXML2 CMake dependencies, and assertion-only tests (`testtri.cpp`).
- `irrgo` (same repo) is the closest precedent for build and style: C++20, CMake ≥ 3.20, Ninja + MSVC on
  Windows and must build on Debian, Qt 6.8.3 at `C:/Qt/6.8.3/msvc2022_64`, Qt-free core targets,
  `#pragma once`, PascalCase files, braces everywhere, no silent defaults, `TODO(decide)` markers. Its
  `AbsGame::Game` (two-player, zero-sum, enumerable integer moves) does NOT fit a wargame; its
  `gui_common` (GameMainWindow hooks, PlaybackBar, MoveListWidget, SearchController token guard) does.
- `visolver` supplies the Google Test recipe (FetchContent v1.16.0, `gtest_force_shared_crt`,
  `*_add_gtest` helper, `gtest_discover_tests`), the QPainter-widget pattern (`flowplanview`), the
  QtConcurrent + QFutureWatcher + value-capture + staleness-token pattern, and PRNG discipline
  (`std::mt19937_64(seed ^ streamTag)`, generators passed by reference, literal seeds in tests).
  Neither repo deploys Qt DLLs from CMake; both document the manual recipe.
- `HexKrieg` (Java) supplies the patterns to port: frozen immutable board shared across parallel
  rollouts; adjudicators as pure functions returning fresh state; one bounded multi-source Dijkstra with
  a generation-stamped scratch buffer; `HKMove` (parameterised move) separated from `Player` (chooser);
  named PRNG streams; insertion-ordered collections wherever a PRNG is reached.
- No PGG rules XML exists (map and counters do); the PGG rulebook, summary, CRT and errata PDFs are in
  `C:\Library\War-Games\Panzergruppe Guderian\`. No sheet XML carries `<region>` membership. No language
  holds initial set-up positions.

## 2. Decisions and recommendations

| Topic | Decision |
|---|---|
| Graph library | Neither. Hand-rolled A*/Dijkstra/BFS/components over the ABC hex grid and link networks. |
| ABC coordinates | Re-implemented in `hexcoord` as C++20 strong types, reproducing `tricoord` semantics exactly; `testtri.cpp` assertions become Google Tests; `panj\hexmap` untouched. |
| Orientation | One ABC algebra; flat-top vs pointy-top is a property of the pixel mapping and of the offset constructors (taken from the sheet `grid`). |
| Pilot order | TRC → PGG → DS → DDaT. |
| XML parser | TinyXML2 (zlib) via FetchContent, confined to `hexxml`; XSD validation stays Python/lxml, run by ctest. |
| Human player shape | Prompt/submit: `prompt()` and `apply(Command)`; mid-resolution choices surface as `PendingDecision`. |
| Region membership | In the sheet XML's existing `<region layer hexes>` element; the package manifest binds layer names. |
| XSD changes | None required for ABC: hexes stay printed IDs; the engine maps ID → (col,row) → ABC through the sheet `grid`. New schemas: `hexsave.xsd`, `hexpackage.xsd`. Every XSD edit is a review gate. |

Graph library — why neither, and which if forced. OGDF: GPL-2/3 with narrow linking exceptions —
linking it makes the engine GPL, incompatible with "All Rights Reserved"; compiled, heavy, superb
shortest-path/flow/components, `NodeArray<T>` payloads, thread-safe pool allocator by default (must be
verified in the build), active (Foxglove 2025.10); no A*. CXXGraph: MPL-2.0, header-only, MSVC/C++17,
Dijkstra/BFS/DFS/components/topological/Ford-Fulkerson; `shared_ptr` + string-keyed nodes
(allocation-heavy, wrong for thousands of parallel rollouts), no thread-safety contract, stale (4.1.0,
June 2024), no A*. Every search the four rulebooks need runs on a static integer-indexed hex grid where
the neighbour function is ABC arithmetic; both libraries would be an algorithm cookbook behind an
adapter. If a library were ever required, CXXGraph for its licence; OGDF only if the engine may be GPL.

Agent workflow (Fable coordinates, ≤ 5 Opus/Sonnet workers):

| | A: contract-first layer workers | B: one worker per milestone, sequential | C: game-parallel workers |
|---|---|---|---|
| Parallelism | 3 in the core phase | 1 | up to 4 |
| Token cost | medium; workers read only their contract | lowest per step, longest wall-clock | high; four workers each learn the core |
| Rework risk | low if contracts are compiling headers + test lists | lowest | high before the core is stable |
| Crash resume | good: independent per-worker task files | best | good, four in-flight states |
| Review burden | 3 small reviews per phase | continuous | 4 large diffs at once |

Recommended: A, then C. Phase 1 (core): Fable writes the contracts alone, then three workers in parallel
(W1 Opus: hexcoord+hexsearch+hexengine; W2 Sonnet: hexxml+hexmodel+package loader; W3 Sonnet: hexrecord,
stubbing the engine through the contract). Phase 2 (games): two Opus workers at a time (W4: TRC then DS;
W5: PGG digest → PGG then DDaT). Phase 3 (GUI): one Sonnet worker (W6) on hexview/hexqt, then game-GUI
tasks in freed slots. Never more than 5 alive; usually 2–3. Fable works alone on anything touching a
contract, XSD, manifest, PLAN.md or docs, on reviews, on fixes under ~150 lines, and on anything spanning
two workers. Fable delegates when a task is bounded by a header and a test list, when the diff will
exceed ~300 lines, or when it is transcription (PGG digest, scenarios, golden scripts).

## 3. Repository layout and build

```
hexgames/
  CMakeLists.txt  CMakePresets.json  cmake/{HexGamesGtest,HexGamesQtDeploy}.cmake  .gitignore  .clang-format
  CLAUDE.md  PLAN.md  BUGS.txt  tools/  tasks/  uml/  doc/  tests/
  hexcoord/   ABC algebra, Direction, Grid, pixel mapping                 (no deps)
  hexxml/     TinyXML2 façade + value models SheetDoc, CounterSetDoc, RulesDoc, SaveDoc, PackageDoc
  hexmodel/   ids, quantities, Board, Position, Roster                    (hexcoord)
  hexrules/   RuleSet (typed rules), Binding/PackageLoader, Ledger        (hexmodel, hexxml)
  hexsearch/  scratch, Field, Dijkstra/A*/flood/components/monotone walk  (hexmodel)
  hexengine/  Session, Command, Player, PhaseCursor, policies, adjudicators, events, PRNG streams
  hexrecord/  hexsave read/write, canonical writer, replay, golden compare (hexengine)
  hexview/    presentation model: Scene, MapFrame, faces, ViewState, InteractionMachine (NO Qt)
  hexqt/      Qt widgets: Viewport, ScenePainter, CounterPainter, MapView, GameSession, panels
  games/{trc,pgg,ds,ddat}/  engine/  gui/  test/  golden/  scenario/
  game_rules/xml, map_graphics/xml, unit_graphics/xml (existing) + game_records/xml (hexsave) + packages/xml
```
Dependency order: hexcoord → hexxml → hexmodel → hexrules → hexsearch → hexengine → hexrecord →
games/*/engine → hexview → hexqt → games/*/gui → tools. Every library is `STATIC`; `hexqt` and the `*_gui`
executables are the only targets that may link Qt; `option(HEXGAMES_BUILD_GUI ON)` and the `headless`
preset that never calls `find_package(Qt6)` are the compile-time gate.

Build conventions: C++20, `CMAKE_CXX_EXTENSIONS OFF`, IPO in Release, MSVC `/W4` else `-Wall -Wextra`,
`Qt6_DIR` Windows default guarded by `NOT DEFINED`, GoogleTest v1.16.0 via FetchContent with
`gtest_force_shared_crt ON`, `hexgames_add_gtest(name SOURCES ... LIBS ... LABELS ...)` wrapping
`gtest_discover_tests`, ctest labels `xsd hygiene core package records golden long gui trc pgg ds ddat`.
Presets: `win-msvc-debug`, `win-msvc-release` (Ninja, cl, `cmake-build-*`), `headless`, `linux-debug`,
`linux-release`; test presets set `QT_QPA_PLATFORM=offscreen`. `hexgames_deploy_qt(<target>)`: POST_BUILD
`windeployqt` with a copy fallback (Core/Gui/Widgets/Concurrent[d].dll + platforms\qwindows[d].dll).

Style: `#pragma once`; PascalCase `.h/.cpp`; `namespace HexCoord`, `HexModel`, …, `Trc`, `Pgg`; braces on
every `if`; `throw` never `assert`; exhaustive `switch` without `default`; Yoda literals; trailing-`P`
predicates; no `unordered_*` in engine state; small functions and files; three-line copyright banner top
and bottom of every `.h/.cpp`; `Copyright Ben Paul Wise. All Rights Reserved.` first and last line of
every `.md/.txt`; `#` form in CMake, Python and PowerShell — enforced by `tools/banner-check.py`.

## 4. Architecture

Invariants live in types; the boundary (XML load, user input) is the only place that validates.

### 4.1 hexcoord — the ABC system
With the flat `(row, clm)` constructor, +Q = C−B is one row down and R−S = 3A is two columns right, so in
pixels A = (s,0), B = (−s/2, −√3s/2), C = (−s/2, +√3s/2); pointy-top is the same basis rotated 30°. The
clockwise cycle of centre-to-centre steps is (−Q, +R, −S, +Q, −R, +S) in both orientations; only compass
names differ (flat: n ne se s sw nw; pointy: ne e se sw w nw — matching `hexsheet2svg.py`).
- `Abc{a,b,c}` stored reduced (tricoord's `reduce()`), `height()`, `hvCode()` = floor-mod(2a−(b+c), 6);
  `Qrs{q,r,s}` with `toAbc()` = (r−s, s−q, q−r); `+ − *`, `hexDist`, `edgeDist`; constants
  `AVec BVec CVec QVec RVec SVec`.
- `HexCentre` (hvCode ∈ {0,3}; `+Qrs`, `−HexCentre → Qrs`, `neighbour`, `edge`, `vertices()` total),
  `HexVertex` (hvCode ∈ {1,2,4,5}; `hexes()`, `neighbours()`), `HexEdge` (two adjacent vertices, canonical
  order; `between(HexCentre, Direction)`, `hexes()`, `vertices()`). Only the explicit constructors test
  `hvCode`.
- `Direction` (six, cyclic; `rotate`, `opposite`, `step → Qrs`, `compassName(d, Orientation)`,
  `fromCompass`), `Orientation {Flat, Pointy}`, `Parity {Odd, Even}`.
- `Grid` from the sheet `grid` spec: generates every printed id through `HexIdFormat` and interns them,
  `centreOf(col,row)` via the two offset formulas, inverse `indexOf`, `pixelOf`, `hexAt(Pixel)` by inverse
  basis + cube rounding. DS's two grids share ONE lattice (origins lattice-integral, else throw).
- Neighbourhood: `neighbours`, `ring`, `disc`, `ray`, `spineWalk`.
- Tests port every `testtri.cpp` assertion plus `PixelMatchesRendererFourSheets`.

### 4.2 hexxml
`XmlDocument::load`, `XmlNode::required`, `optionalAs<T>`, `children`, `line()`; every failure throws
with `file:line`. Value models `RulesDoc`, `SheetDoc`, `CounterSetDoc`, `SaveDoc`, `PackageDoc` mirror
the XSDs one-to-one. Nothing outside `hexxml` includes `<tinyxml2.h>`.

### 4.3 hexmodel
Strong ids `Id<Tag>`; `SideMask`; quantities `Strength`, `MovementPoints` (halves), `HexCount`,
`Actions`, `Budget` variant, `Steps{n,max}`, `Odds::of(att, def, Rounding)`, `MoveCost`, `Extent`,
`TurnSelector`. `Board` immutable after `BoardBuilder::build` (hex table, edge terrain sets, 6×N
neighbours, `LinkNetwork`s, `RegionLayer`s, `Space`s, grids, generated row/col layers). `Roster` of
`UnitSpec` via a game `ValueLineReader`. `Position` mutable, value-semantic (units, stacks in insertion
order, last-toucher control, network/region state, tracks, weather, `TurnClock`, `PendingDecision`,
`digest()`).

### 4.4 hexrules
`RuleSet` (immutable, all IDREFS resolved, symmetric `HostilityMatrix`, `PhaseTree`, typed `Table`s,
`Annex` of prose rules with scope). `hexpackage.xsd`: paths + bindings "identity by default, exceptions
listed" (terrain, hexside, network, space, layer, counter → unit-type); `<g>_package_test`. Rule coverage
ledger: every `RuleId` → `Implemented | Common | OutOfScope | OptionalNotImplemented`; policies expose
`claims()`; `<g>_ledger_test`. Plug-in interfaces: `ZocPolicy`, `MovementPolicy`, `SupplyTrace`,
`CombatResolver`, `RetreatPolicy`, `StackingPolicy`, `VictoryCheck`, `PhaseGate`, `Randomizers{Die, Deck,
Hand}`, with defaults in hexengine.

### 4.5 hexsearch
`SearchScratch` (generation-stamped) + `Field`; `dijkstraBounded` as the one primitive, `aStar`,
`bfsFlood`, `components`, `reachCount(threshold)`, `monotoneWalk`, `regionFlood`; adaptors
`HexAdjacencyGraph`, `NetworkGraph`, `LayeredGraph`; concepts `SearchGraph`, `CostLike`.

### 4.6 hexengine
`Session{GameDefinition const, Position, PrngStreams, SearchScratch, EventLog}` with `prompt`,
`legalCommands`, `moves(unit)`, `apply(Command)`, `fork`, `attach(EventSink&)`. `Command` variant +
game `CommandGrammar`; `Player` (Scripted, Random, AI slot); `PositionView(side)`. Pure adjudicators;
`PendingDecision`; `PhaseCursor` with `PhaseCaps`; `PrngStreams` per `StreamTag` (`mixSeed`); `Event`
variant, `EventLog`, `TextEventEncoder`. No globals, no caches → N sessions on N threads.

### 4.7 hexrecord — hexsave.xsd
`save @format @kind=scenario|save|script|golden @game @package @scenario @seed @engine @created` with
`package/file*`, `cursor`, `sides/side/register* flag*`, `units/unit*`, `control/hex* link*`,
`regions/region*`, `piles/pile*`, `streams/stream*`, `log/move*(arg*, result?, draw*, event*)`, `notes`.
Canonical writer; strict replay reporting the first divergent `@n`.

### 4.8 hexview (no Qt; the HTML seam)
`HexGeometry`/`MapFrame`, `Scene` of `Primitive`s with `HitTag`s, `MapSceneBuilder` (ten static layers as
`hexsheet2svg.py`), `SymbolLibrary`, `FaceModel/FaceResolver/FaceLayout` (as `counters2svg.py`),
`StateSceneBuilder`, `ViewState`, `Lod`, `EngineFacade`, `Intent`, `InteractionMachine` (emits only
commands the facade listed as legal), `ReplayController`, `AnimationPlan`, `SvgWriter`, `JsonWriter`.

### 4.9 hexqt
`Viewport`, `ScenePainter`, `CounterPainter`+`FaceCache`, `MapView`, `StackInspector`, `PhaseBar`,
`OrderOfBattle`, `LogView`, `SpacesDock`, `PlaybackBar`/`EventListWidget`, `GameSession`,
`AiTurnRunner`, `GameMainWindowBase`, `hexqt_viewer`; `--script/--autoplay/--snapshot` on every `*_gui`.

### 4.10 Game modules
`Binding`, `ValueLineReader`, policy overrides only where prose changes a default, `CommandGrammar`
verbs, `Ledger.cpp`, scenarios, scripts + goldens, `<G>MainWindow` + panels.

## 5. XML additions and review gates
hexsave.xsd + validator + one scenario per game; hexpackage.xsd + manifests + `tools/bind-counters.py`;
PGG digest then rules XML (review); region membership in the four sheet XMLs (review); later XSD
proposals (review first): `unit-type/@reveal`, `panel/@space`, `phase/@repeat-per-side`, `phase/@caps`.

## 6. Documentation
PlantUML in `uml/` (`[PROPOSED]` until built): hexcoord/hexmodel/hexrules-package/hexengine-policies/
hexengine-session/hexrecord/hexview/hexqt class diagrams, `hexweb-future`; sequences seq-load-package,
seq-move-command, seq-combat-with-decision, seq-turn-advance, seq-parallel-rollouts, seq-replay-golden,
seq-save-reload, seq-human-move, seq-ai-turn, seq-load-replay. `tools/render-uml.ps1`. Ten sections in
`doc/2026-09-12-design/` (01, 02, 04, 05, 08 now; 03, 06, 07, 09, 10 at the first code snapshot);
`tools/build-summary.ps1` → `YYYY-MM-DD-system-summary.tex`.

## 7. Testing and golden records
Google Test per module; loader tests on the four real sets; package and ledger tests; search tests;
`determinism_test`; `parallel_rollout_test` (sanitizers); hexview headless tests; hexqt offscreen tests;
SVG goldens against the committed Python renders. Golden records `games/<g>/golden/<slug>.script.xml` +
`.golden.xml`; `<g>_golden_test` byte-compares and reports the first divergent move; `tools/bless-goldens.ps1`;
re-bless commits cite `refactor` / `bugfix` / `rules <id>`. `hexgames_cli`, `hexgames_bench`.

## 8. Verification
`cmake --preset win-msvc-debug && cmake --build --preset win-msvc-debug && ctest --preset win-msvc-debug -L core`;
`-L xsd`, `-L package`, `-L records`, `-L golden`, `-L gui`; the `headless` preset builds every non-Qt
target with Qt absent; `hexgames_cli --replay` reproduces goldens byte-for-byte; `hexgames_bench
--sessions 64 --threads 16` matches a serial run; `trc_gui --script ... --snapshot`; Debian
`linux-release` + `ctest -LE gui`.

Copyright Ben Paul Wise. All Rights Reserved.
