Copyright Ben Paul Wise. All Rights Reserved.

# hexgames — plan of record (live tracking copy)

Approved 2026-09-12 from `2026-09-12-2002-plan-request.txt`. Dated snapshots: `doc/2026-09-12-plan.md`.
The coordinator (Fable) rewrites RESUME HERE before every delegation and after every hand-off, ticks
milestone boxes, and appends to the decision log. Workers write only their own `tasks/NN-slug.md`.

## RESUME HERE

- phase: 1 (core contracts)          milestone: M0 done, M1 in progress
- in-flight tasks: none (Fable alone)
- next coordinator action: M1 contracts — interface headers for hexcoord/hexmodel/hexengine,
  `[PROPOSED]` PlantUML, test lists, `hexsave.xsd`, `hexpackage.xsd`, TRC scenario + package manifest
- blockers: none
- last green: `cmake --preset win-msvc-debug && cmake --build --preset win-msvc-debug && ctest --preset win-msvc-debug`
  (smoke + xsd + hygiene labels)   commit: (pending)
- crash protocol: read this block, then every `tasks/*.md` with status assigned|in-progress|blocked,
  then `git status`; continue from the task files' `resume:` lines. Nothing lives only in chat.

## Milestones

- [x] M0  PLAN.md, .gitignore, CLAUDE.md, BUGS.txt, CMake skeleton, presets, gtest + TinyXML2 fetch,
          banner check, XSD ctest, .clang-format, tasks/ protocol, smoke test
- [ ] M1  Contracts (F): interface headers, PlantUML `[PROPOSED]`, test lists, hexsave.xsd, hexpackage.xsd,
          TRC scenario + trc.package.xml, five forward design docs
- [ ] M2  hexcoord (W1): ABC strong types, Direction, Grid/HexIdFormat, pixel mapping, testtri port,
          four-sheet pixel test
- [ ] M3  hexxml + hexmodel (W2): document models, Board/Position/Roster, BoardBuilder, package loader
          and package tests on the four sets
- [ ] M4  hexsearch + hexengine (W1): scratch/Field/algorithms, Session, PhaseCursor, PRNG streams,
          events, default policies, adjudicators, determinism + parallel tests, hexgames_cli
- [ ] M5  hexrecord (W3): hexsave reader/writer, canonical writer, replay, golden harness, validate
- [ ] M6  TRC engine module (W4): bindings, policies, ledger, scripts + goldens
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

## XSD proposals awaiting review

- (none yet) Candidates noted in the plan: `unit-type/@reveal` (PGG untried), `phase/@repeat-per-side`,
  `phase/@caps`, `panel/@space`.

## Open questions

- `coords.txt` prose labels B "NorthEast"/C "SouthEast" disagree with the algebra in `tricoord.cpp`
  (B is the north-west corner vector, C the south-west). The code follows the algebra.

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
