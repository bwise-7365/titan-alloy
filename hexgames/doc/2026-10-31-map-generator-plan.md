Copyright Ben Paul Wise. All Rights Reserved.

# hexgen: a C++ hex map generator for hexgames, headless and in HexMapEd

Approved by Ben 2026-10-31. Ben then asked for iteration 1 only for now; iteration 2 is recorded here
as the direction, not as work in progress.

## Context

HexKrieg has a procedural map generator in Java: fractal or Perlin heights, terrain by percentage,
cities, roads by minimum spanning tree plus a tour, and roads and cities on each map edge. hexgames
(C++20, titan-alloy) has none. Its maps are traced from scans or, for Circling Dragons (CD), built
in Python from real geography. Ben wants a C++ generator in two iterations:

1. **Iteration 1** does what HexKrieg's generator does: fractal and Perlin heights, terrain
   fractions the user sets, and the number of roads and cities on each map edge.
2. **Iteration 2** adds the detail that makes a map approach Dai Senso (DS) or Circling Dragons:
   more terrain types, rivers, a hierarchy of places, several networks, countries and labels.

The generator is headless (a library and a command), and has a Qt GUI.

Decisions (Ben, 2026-10-31):
- **Home.** A new module `hexgen` in titan-alloy/hexgames (namespace `HexGen`).
- **Fidelity to Java.** Same algorithms, parameters and defaults as HexKrieg, with hexgames' own
  deterministic random numbers, tested by properties. Maps need not be bit-identical to Java's.
- **GUI.** A Generate dock in HexMapEd, not a separate application.
- **Settings.** An XML document in a new language, `hexgen.xsd`, in HexRules. Ben reviews the
  schema.

### What exists and is reused

| Need | Existing code |
|---|---|
| Hex geometry in ABC/TUV | HexRules `cpp/hexcoord`: `Grid` (`centreOf`, `indexOf`, `idOf`, `pixelOf`), `HexCentre`, `HexVertex`, `HexEdge`, `neighbours`, `ring`, `disc`, `Direction`, `compassName` |
| Sheet model and writer | `HexXml::SheetDoc` structs (HexRules `cpp/hexxml/SheetDoc.h`); `HexMapEd::writeSheet` (hexgames `hexmaped/SheetWriter.h:20-22`), which already writes `seed`, `move-cost`, `alt` and edge `cost`; `Schema::require*` |
| Shortest paths | `HexSearch::Algorithms` over any `SearchGraph` (`hexsearch/Search.h:27-131`): multi-source `dijkstraBounded` with `Field::distance`, `origin` and `pathTo`; `components` |
| Seed mixing | `HexEngine::mixSeed` (SplitMix64, `hexengine/PrngStreams.h:22-29`) |
| SVG preview without Qt | `MapFrame::of`, `MapSceneBuilder`, `writeSvg` (hexview) |
| Qt view, document, undo, save | `MapView`, `paintScene`, `Document`, `MainWindow::buildDocks`, `applyEdit` (hexmaped/app) |
| Validation | HexRules `python/validate.py` (dispatches by root element) |
| Tests | `hexgames_add_gtest(... [GUI])` (`cmake/HexGamesGtest.cmake`) |

**What is missing:**
- 2-D noise;
- terrain assignment by quota;
- a minimum spanning tree and the tour;
- a `SearchGraph` over a bare `Grid` (the existing adaptors need a `Board`);
- edge-set-to-chain decomposition for the C++ side;
- seed mixing usable outside hexengine;
- a settings reader.

## Architecture

**The generator is a pure function.** `SheetDoc HexGen::generate(const Settings&)` holds no state
between calls and uses no globals. The CLI and the GUI both call it, and the GUI can run it on a
worker thread. It works on one plain model and passes that model through an ordered list of stages:

```
Settings ──► GenMap (Grid + per-hex arrays + edge/link sets)
               │  stage: heights      (Raster: fractal | perlin)
               │  stage: terrain      (quota roles)
               │  stage: altitude     (optional calibration)
               │  stage: network      (cities → MST + tour → paths)
               │  stage: edge links   (per side: spacing, edge cities, join)
               │  ── iteration 2 adds stages here, each optional ──
               ▼
           toSheet(GenMap) ──► SheetDoc ──► writeSheet / MapView / SVG
```

**The model.** `GenMap` holds:
- the `HexCoord::Grid`;
- a `std::vector<double> height`, a `std::vector<double> cover`, a `std::vector<int> terrain`
  (indexed by `Grid::indexOf`) and an optional `alt`;
- a set of `HexEdge` per line kind;
- a set of hex-to-hex steps per link kind;
- the place glyphs.

Each stage is a free function `void stageX(GenMap&, const Settings&, std::uint64_t seed)`.

**Randomness.** Each stage draws only from its own `std::mt19937_64` seeded by
`mixSeed(seed, StreamTag::X)`. It takes raw draws and never a `<random>` distribution, which is the
rule at `hexview/Buildings.h:9-13`. As a result, adding or changing an iteration-2 stage never
changes the output of an earlier stage under the same seed.

**Geometry is ABC throughout.** Every field is sampled at the hex centre's true position
(`Grid::pixelOf`), not at `(row, col)`. Hexsides are `HexEdge`s and river corners are `HexVertex`es.
Offset `(col, row)` appears only in the printed id.

**Errors.** Settings are validated once, at the reader, which throws `std::invalid_argument` naming
the file, line and element. The stages trust validated settings and contain no `assert`.

## Step 0: record the plan

1. Create a titan-alloy branch `10-map-generator` from `9-hexrules`, because this work needs the
   HexRules layout.
2. Commit this plan alone as `hexgames/doc/2026-10-31-map-generator-plan.md`.
3. Add a RESUME HERE line to hexgames `PLAN.md`.
4. HexRules: create a branch `2-hexgen` from `1-bootstrap` and add the schema proposal (step 1) to
   `PLAN.md`.

## Iteration 1: HexKrieg's generator in C++

### Step 1: settings language (HexRules)

`xsd/hexgen.xsd`, root `generator`. Approving this plan approves this sketch; the finished schema
comes back for review before anything reads it.

```xml
<generator seed="60834130">
  <grid orientation="flat" offset="odd" cols="30" rows="18" size="30"/>
  <height method="perlin" roughness="0.5"/>          <!-- fractal | perlin -->
  <terrain id="water"    role="low"   percent="10" fill="#7fb2e5" move-cost="100"/>
  <terrain id="mountain" role="high"  percent="20" fill="#a08060" move-cost="4"  road-cost="2"/>
  <terrain id="forest"   role="cover" percent="25" fill="#5f9f5f" move-cost="2"  road-cost="1.25"/>
  <terrain id="urban"    role="city"  percent="3"  fill="#c0c0c0" move-cost="1"  road-cost="1"/>
  <terrain id="open"     role="rest"               fill="#e8e0b0" move-cost="1"  road-cost="1"/>
  <roads line="road">
    <side at="left" roads="2" cities="2"/> <side at="right" roads="2" cities="2"/>
  </roads>
  <altitude max-land="2000" max-depth="200"/>        <!-- optional; absent: no alt written -->
</generator>
```

- **Roles.** Each of `low`, `high`, `cover`, `city` and `rest` appears once.
- **Percentages.** The percentages must sum to at most 100. `rest` takes the remainder.
- **Road costs.** A terrain without `road-cost` cannot carry a road (water).
- **Edges.** For each side, `cities` must be no more than `roads`.
- **Defaults.** The defaults are HexKrieg's, so a generated sheet uses HexKrieg's terrain ids and
  HexKrieg can read it.
- **Other changes.** `validate.py` gains the `generator` root. `corpus/valid/generator-hexkrieg.xml`
  and `corpus/invalid/generator-*.xml` are added (role missing, percentages over 100, cities above
  roads).

### Step 2: shared seed mixing (hexgames)

- **New header.** Move the SplitMix64 arithmetic of `HexEngine::mixSeed` into a header-only
  `hexmodel/Seed.h`: `mixSeed(std::uint64_t seed, std::uint64_t stream)`.
- **Wrapper.** `HexEngine::mixSeed(seed, tag)` becomes a one-line wrapper around it.
- **Check.** Every golden must be unchanged, which proves the arithmetic is the same.
- **hexgen tags.** hexgen declares its own `StreamTag`: `Height`, `Cover`, `City`, `EdgeTop`,
  `EdgeBottom`, `EdgeLeft`, `EdgeRight`, then the iteration-2 tags.

### Step 3: the `hexgen` library (`hexgames/hexgen/`)

It links `hexcoord`, `hexxml`, `hexmaped` and `hexsearch`, and builds under the `headless` preset.

| File | Contents (Java source it ports) |
|---|---|
| `Settings.h/.cpp` | records for the language; `Settings::parse(XmlDocument)`, throwing with file and line |
| `Raster.h/.cpp` | a regular sample grid covering the map's pixel box, with bilinear `at(x, y)`; one sample per hex spacing in iteration 1 |
| `Noise.h/.cpp` | `fillFractal(Raster&, roughness, gen)`, midpoint displacement with corners in [1, 100] and perturbation scaled as `FractalHeight`; `fillPerlin(Raster&, persistence, gen)`, 6 octaves, 4 gradients, quintic fade and a shuffled 256-entry table (`PerlinTerrainBuilder`) |
| `GenMap.h` | the model above |
| `Terrain.h/.cpp` | quota assignment (`TerrainAssignment.assign`). Water is the lowest `n`, then mountain the highest `n`. Forest is the highest cover among the rest. Cities are a shuffle of land; their count is `cityCount = round(softMin(pct·N/100, 200))`. Sorts are `std::stable_sort` with ties in index order. Altitude is calibrated about the waterline. |
| `CostGraph.h` | a `SearchGraph` over a `Grid`: arcs to the in-grid neighbours. An arc's cost is the sum of its endpoints' road costs in hundredths, an integer in `CostHalves`, so ties are exact. A terrain with no road cost has no arcs. |
| `Network.h/.cpp` | `buildNetwork(GenMap&, kind, nodes, costs)`: one `dijkstraBounded` per node gives a distance matrix; Prim's algorithm on that dense matrix gives the minimum spanning tree (no union-find needed); a furthest-insertion closed tour (`TerrainBuilder` lines 254-334); every tree edge and tour pair is laid with `Field::pathTo` |
| `EdgeLinks.h/.cpp` | `addEdgeLinks(GenMap&, side, roads, cities, gen)`: shuffled non-water candidates excluding the corners, kept only if `2·roads·gap ≥ length`; the first `cities` become cities; each end joins the nearest hex already on the network (`TerrainBuilder.addEdgeRoads`) |
| `Chains.h/.cpp` | a set of steps to chains, cut wherever the degree is not 2, with leftover loops as closed chains (`HexSheet` lines 180-210); iteration 2 reuses it for rails and for river paths |
| `ToSheet.h/.cpp` | `GenMap` to `SheetDoc`: grid, palette, terrains with `move-cost`, `hexes` lists, `alt`, `link kind="road"`, `seed`, `urban="symbol"`, `junctions="implicit"`, city glyphs. It follows the corpus sheet `hexkrieg-save-sheet.xml`. |
| `Generate.h/.cpp` | `SheetDoc generate(const Settings&)`: runs the stages in order |

**Deliberate departures from Java,** each recorded in the header comment:
- **Water.** A pair of cities with no land path between them is not joined. Java gives water a cost
  of 10000 and so builds roads across it.
- **Edge ends.** An edge end may be any hex a road can enter (Java: any hex but water); each is
  joined to the nearest hex already on a road, as in Java.
- **Ids.** Without `id-format`, the ids widen to three digits at 100 columns or rows, as in Java.
- **Altitude.** Java's `buildBoard` never writes altitude. Here altitude is written only when
  `<altitude>` is present.

### Step 4: the command (`hexgames/hexgen/app/HexGenCli.cpp`, target `hexgen_cli`)

`hexgen_cli <settings.xml> <out-sheet.xml> [--seed N] [--svg out.svg]`

- `--seed` overrides the file's seed.
- `--svg` draws the result through `MapFrame`, `MapSceneBuilder` and `writeSvg` (hexview, headless).
- The exit codes are those of `hexgames_cli`: 2 for usage, 3 for an exception.

It is a separate executable, because `hexgames_cli` links both games.

### Step 5: the GUI (`hexmaped/app`)

**`GenerateDock`** (namespace `HexQt`) is a form bound to `Settings`:
- rows, columns and orientation;
- the seed, with a random button that draws from `std::random_device` in the GUI only;
- the method (Fractal or Perlin) and the roughness;
- one percentage spinner per role, with `rest` shown derived, as HexKrieg's Main window does;
- a 4×2 grid of roads and cities per side, where the maximum for cities follows roads;
- Load settings, Save settings, and Generate.

**Generate:**
- It runs `generate` through `QtConcurrent::run` with a `QFutureWatcher`, so the window stays live
  on large maps.
- It opens the result as a new `Document`, through a new `MainWindow::adoptSheet(SheetDoc)` that
  shares `open()`'s path. If the current document has unsaved changes, it asks first.
- Regenerating replaces the document rather than adding an undo entry, so `Document` needs no new
  edit.

**Menu.** "File > New from generator…" shows the dock.

### Step 6: tests (gtest, label `hexgen`)

- **Noise:**
  - the same seed gives the same raster;
  - fractal corners and roughness 0 stay in [1, 100];
  - Perlin output lies in [-1, 1].
- **Terrain:**
  - the count of each terrain equals its computed quota exactly;
  - every water hex is no higher than every land hex;
  - with `<altitude>`, the minimum is −200 m, the maximum is 2000 m, and water lies below 0
    (as `AltitudeCalibrationTest` checks).
- **Network:**
  - the cities on one land mass form one road component (`Algorithms::components` over road steps);
  - no road enters a hex that cannot carry a road;
  - 0 cities gives no roads.
- **Edge links:**
  - every end lies on its side, is not a corner and is not water;
  - the spacing rule holds and the number of cities is at most `roads`;
  - each end reaches a city by road;
  - the same seed gives the same ends.
- **Whole sheet:**
  - `generate` twice gives the same `writeSheet` string, for both orientations;
  - the output validates (`validate.py`, in the `xsd` ctest loop) and round-trips through
    `SheetDoc::parse`;
  - one small golden sheet is committed under `hexgen/test/golden`.
- **GUI** (`GUI`, offscreen): the dock generates a 12×20 map, and the document has 240 hexes.
- **Cross-language:** a generated sheet is committed as HexRules
  `corpus/valid/hexgen-sheet.xml`, and HexKrieg's `HexSheetTest` reads it into a `HexGraph`.

### Step 7: documentation

Update the following:
- hexgames `CLAUDE.md`: the `HexGen` namespace, and the module in the dependency list;
- hexgames `PLAN.md`;
- hexgames `README` (the command);
- the module figure and table of `doc/hexgames-architecture.tex`;
- HexRules `CLAUDE.md` and `README.md` (the new language).

## Iteration 2: detail and scale like Dai Senso and Circling Dragons

**Targets** (counted from the sheets):

| | DS | CD |
|---|---|---|
| Hexes | 2968 (pointy) | 2162 (flat) |
| Terrains | 6: sea, clear, hills, desert, swamp, forest | 7: sea, broken, clear, mountain, steppe, desert, marsh |
| Hexside lines | rivers 171, borders 361, region 129, mountain 113, lake 18 | none besides the river paths |
| Rivers | 171 hexside edges | 21 river paths (358 hexsides) with corner junctions and `ends` |
| Links | rail 72, road 8 | rail 39, road 14, track 4, courier 1, all named, with `ends` and 62 junctions |
| Places | capital 27, city 63, town 60, ports 99 | capital 4, major city 7, city 33, town 65, ports 23 |
| Other | faction rings | airfields and similar marks |
| Areas | — | 9 regions |
| Labels | 62 | 175 |

**Scale is not the problem.** Iteration 1 already reaches 300×300. The work is the content.

Each item below is an optional stage with its own settings element, its own stream tag, and its own
tests. Each schema addition goes to Ben for review when written. Panels, which hold the game's
charts, stay out of scope.

1. **Finer fields and climate.**
   - **Resolution.** The raster resolution becomes a setting, for example 4 samples per hex.
   - **Statistics.** Each hex gets the 19-point sample statistics that CD's stage 2 takes (the
     centre, 6 points at 0.45 R and 12 at 0.8 R): mean, median, spread and range, in metres after
     calibration.
   - **Moisture.** A second field, `moisture`, combines noise, distance from the sea (a multi-source
     `dijkstraBounded` from the sea hexes) and a gradient chosen by the user.
2. **Terrain by profile.**
   - **Rules.** An ordered list of rules over the statistics and moisture, where the first match
     wins. Each rule is a `<class terrain="mountain"><when stat="median" ge="1800"/>…`.
   - **Profiles.** Two shipped profiles:
     - `cd`: CD stage 4's thresholds (`stage4_terrain.py:93-112`), with moisture standing in for
       the steppe, desert and floodplain polygons;
     - `ds`: DS's six terrains.
   - **Coexistence.** The quota roles of iteration 1 remain, and a document uses one method or the
     other.
3. **Coasts.**
   - Sea is the low water connected to the map edge. Enclosed low water becomes lake terrain or
     marsh.
   - A land mass or lake smaller than a set size is removed.
4. **Rivers.**
   - **Drainage.** Heights are taken at the hex corners (`HexVertex`, lattice points in ABC).
     Depressions are filled by priority flood. Each corner drains to the lowest of its 3 neighbouring
     corners, and the flow is accumulated.
   - **Rivers.** A hexside chain whose flow exceeds a threshold is a river. A second threshold makes
     it a major river (`path kind="river"` with lines `river` and `minor-river`).
   - **Clean-up,** reusing CD's rules (`build_cd.py:552-766`): drop hexsides on the coast, prune
     spurs of 1 or 2 hexsides, extend short ends to the sea, the map edge or the parent river, place
     junctions where chains meet, and label each end source, sea, junction or edge.
   - **Lakes.** Lakes become `lake` edge lines, as in DS.
5. **Places.**
   - **Classes.** Settlements come in classes (capital, major city, city, town), each with a count
     and a minimum spacing.
   - **Placement.** Places go in descending class order by a seeded score that favours open terrain,
     rivers and coasts, keeping to the spacing (the hexsearch disc).
   - **Ports.** A coastal place gets a port glyph in the slot facing the sea (`build_cd.py:432-435`).
   - **Names.** Names come from a seeded syllable table in the settings, written as `hex/@name` and
     as labels tied to the hex.
6. **Networks.**
   - `buildNetwork` from iteration 1 is called once per `<network kind line connects=... >`. For
     example, rail joins capitals and cities on the rail cost table. A network may cross a river
     only at a cost set in the settings.
   - Roads join towns to the nearest network. Tracks are permitted in desert only.
   - Edge exits per side, from iteration 1, apply to each network.
   - Links get names and `ends` (place, junction or edge), and an explicit `junction` wherever two
     or more links meet.
7. **Countries and regions.**
   - Countries are grown from the capitals by one multi-source `dijkstraBounded` over terrain cost;
     `Field::origin` gives each hex its country.
   - A `region` tint is written per country, and border `edge` lines are drawn where the owner
     changes, skipping the sea.
   - Escarpment (`mountain`) hexside lines are drawn where the altitude changes by more than a
     threshold across a hexside.
8. **Labels.**
   - Country names go at each region's interior point, as `region/@label-at`.
   - Sea names go at the centres of the large sea components. River names follow the river.

**Acceptance for iteration 2:**
- Two settings files, `ds-like.xml` and `cd-like.xml`, generate sheets of DS and CD size, with each
  feature count within a factor of two of the table above (a test asserts these ranges).
- Each sheet renders with no warnings.
- Changing an iteration-2 element leaves the iteration-1 terrain unchanged under the same seed.

**Not in either iteration:** hierarchical refinement through `HexStack.topDown` (HexKrieg's 7-to-1
stack). That would be a possible iteration 3, to add detail by refining a coarse generated map.

## Order, commits and review gates

- **Branches.** Each step is its own commit on `10-map-generator` (titan-alloy) and `2-hexgen`
  (HexRules), staging explicit paths only.
- **Review gates.**
  - Step 1's finished schema and each iteration-2 schema addition wait for Ben.
  - Steps 2-7 proceed once step 1 is approved.
- **Merging.** Nothing is pushed or merged without Ben.
- **Excluded from git.** No generated PDF goes into git. Generated test sheets go to the test
  output directory; only the named golden sheets and corpus sheets are committed.

## Verification

- **hexgames:**
  - `cmake --preset win-msvc-debug`, then build and `ctest --preset win-msvc-debug`: everything
    green, including the labels `hexgen`, `xsd` and `golden`, with all existing goldens unchanged
    after step 2;
  - the `headless` preset builds `hexgen` and `hexgen_cli`.
- **Command:** run `hexgen_cli` on the corpus settings with `--svg`, then validate the sheet with
  `python ..\HexRules\python\validate.py` and look at the SVG.
- **GUI:** Ben runs HexMapEd and uses File > New from generator… (not launched automatically).
- **HexRules:** `python python/run-tests.py`, and the banner check.
- **HexKrieg:** `HexSheetTest` reads `corpus/valid/hexgen-sheet.xml`; the full JUnit list in
  CLAUDE.md still passes.

Copyright Ben Paul Wise. All Rights Reserved.
