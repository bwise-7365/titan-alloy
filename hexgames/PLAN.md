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
- in-flight tasks: tasks/08-map-networks.md (M6c, W5, opus): connected rail, road and river networks on
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
- [ ] M7  PGG digest + rules XML (review gate) + package + scenario; PGG engine module (W5)
- [ ] M8  DS engine module (W4)
- [ ] M9  DDaT engine module (W5)
- [ ] M10 hexview (W6): geometry, scenes, faces, SVG goldens, InteractionMachine, replay
- [ ] M11 hexqt (W6): Viewport, painters, MapView, panels, GameSession, AiTurnRunner, viewer
- [ ] M12 trc_gui, pgg_gui, ds_gui, ddat_gui + GUI tests + script/snapshot playback (W4/W5)
- [ ] M13 hexgames_bench + sanitizers, diagrams `[AS BUILT]`, docs snapshot 2, Debian build (F)
- [ ] M14 two out-of-sample games

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
  international and naval zone border, region borders, mountains, places), against pic4573603.png. Dai Senso rules source: "Dai Senso  Living_Rules_February_2014.pdf" (67 pp, text layer).

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
- EXPECTED from M6b: a hexsave.xsd proposal to store the resolution stack, so a save taken while a
  decision is pending reloads (hexsave has no element for it today). W4 proposes; not applied.
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
