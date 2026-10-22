Copyright Ben Paul Wise. All Rights Reserved.

# Task 17: hexview, the map half (M10a): Style, Scene, MapFrame, LineGeometry, SymbolLibrary, MapSceneBuilder, writeSvg

status: review
worker: W6 (opus)            started: 2026-09-23
resume: DONE (pass 2, legend marks), awaiting review. 38 hexview tests green (32 core + 6 golden, all six sheets EQUAL incl. VL's 30 marks); full ctest 293/293. Last green: tools\build-dev.cmd win-msvc-debug
inputs:
  hexgames/hexview/*.h                                   (the [PROPOSED] contracts; you implement seven of them)
  hexgames/hexview/CMakeLists.txt                        (replace: a STATIC hexview library; keep hexview_contracts)
  hexgames/map_graphics/xml/hexsheet2svg.py              (THE REFERENCE: transcribe, do not reinterpret)
  hexgames/map_graphics/xml/hexsheet.xsd, README.md
  hexgames/map_graphics/xml/{the-russian-campaign,panzergruppe-guderian,d-day-at-tarawa,dai-senso,stalin-moves-west,velikiye-luki}.xml
  hexgames/hexcoord/Grid.h, Direction.h, Abc.h; hexcoord/test/reference-centres.py and hexcoord/test/CMakeLists.txt
                                                         (the pattern for running the Python reference under ctest)
  hexgames/hexxml/SheetDoc.h                             (the parsed sheet: junctions, marks, legend, ticks are all there)
  hexgames/hexmaped/SheetFrame.h, SheetFrame.cpp         (an existing printed-id -> pixel frame; MapFrame is its twin)
  hexgames/hexmaped/app/MapView.cpp                      (drawLinks/joinSegments: a C++ port of link_strokes/side_chains
                                                          to compare with, incl. explicit junctions; presentation only)
  hexgames/doc/2026-09-12-design/test-lists/hexview-tests.md (the test list; the map-half sections apply)
  hexgames/doc/2026-09-23-network-junctions-proposal.md  (section 5: drawing explicit junctions)
  hexgames/cmake/HexGamesGtest.cmake, CLAUDE.md, .clang-format, tasks/README.md
outputs:
  hexgames/hexview/CMakeLists.txt: add_library(hexview STATIC ...) linking hexxml hexcoord hexmodel; hexview_contracts
    target kept for the headers not yet implemented (FaceModel, ViewState, StateSceneBuilder, Interaction, Replay)
  hexgames/hexview/Style.cpp, Scene.cpp, MapFrame.cpp, LineGeometry.cpp, SymbolLibrary.cpp, MapSceneBuilder.cpp,
    SceneWriters.cpp (writeSvg complete; writeJson may be a faithful minimal form, say so in the log)
  hexgames/hexview/test/: StyleTest, SceneTest, MapFrameTest, LineGeometryTest, SymbolLibraryTest,
    MapSceneBuilderTest, MapSvgGoldenTest (label golden) + CMakeLists.txt + a Python step that renders the six
    reference SVGs into the build tree with hexsheet2svg.py at configure or test time
  small header changes ONLY where the contract cannot be met (list every one under "API changes" below)
acceptance:
  tools\build-dev.cmd win-msvc-debug -> ctest 100%; labels core and golden green; nothing else changes
  MapSvgGoldenTest: for each of the six sheets, writeSvg(MapSceneBuilder scene) equals the Python SVG after both
    are normalised (XML parsed; attributes sorted; numbers to 2 decimals; whitespace collapsed), element for
    element, layer for layer -- EXCEPT the buildings groups of city hexes on urban="buildings" sheets, where the
    C++ generator is the reference (Ben, 2026-09-23): compare their count (4 or 5 rects each) and bounds only
  SymbolLibraryTest.HasEveryReferenceSymbol: names == hexsheet2svg.py SYMBOLS keys (read the .py in the test, or
    a generated list -- never a hand copy that can drift) and the nine MARK_SHAPES
  hygiene_banner_check green; no file touched outside hexview/ (+ this task file)
log:
- 2026-09-23 created by the coordinator; Ben chose "option 2": pull the map half of M10 forward so HexMapEd
  (and later hexqt and the HTML client) paint one shared Scene instead of hand-drawn stand-ins
- 2026-09-23 W6: header changes first (see API changes); Style, Scene, Shapes, MapFrame, LineGeometry, SymbolLibrary,
  SceneWriters; golden harness svg-golden.py (reference / centres / compare) + MapSvgGoldenTest with an EMPTY builder:
  VL reported background 1, terrain 154, grid 106, edges 5, links 3, hexglyphs 43, labels 123 elements missing.
  MapFrameTest green on all six sheets (every centre and corner within 0.01 px of the reference Grid, VL's three
  grids and DS's two included).
- 2026-09-23 W6: MapSceneBuilder split by layer (MapAreaLayers, MapLineLayers, MapGlyphLayers, MapTextLayers,
  MapPanelLayer, Buildings); first full build: all six goldens EQUAL, every layer, element for element. Checked the
  comparer catches a changed colour, a 0.03 px move, a changed text, a changed linecap, a changed symbol arc flag and
  a dropped buildings path.

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W6. Read this task file first and stay within its inputs. Implement the map half of hexview:
the seven contract headers named in outputs, exactly as declared, so that a sheet document becomes a Scene of
primitives and writeSvg turns that Scene into the same SVG hexsheet2svg.py writes. The Python renderer is
the specification: SYMBOLS (29 fragments in the unit frame, currentColor), MARK_SHAPES (nine legend-mark
shapes with their size formulas), buildings() (4-5 scattered rectangles, seeded by sheet id and hex id --
in C++ seed a std::mt19937_64 from a SplitMix64 mix of the two strings' FNV-1a hashes and document it;
the C++ layout is the reference from now on), the urban pale-grey tint, the four terrain patterns, the
layer order, rail ticks as a second dashed stroke, casings, dashes, opacity, id labels and id-side, the
smoothed links (link_strokes: midpoint to midpoint through a hex, midpoint to centre at ends and junctions,
polylines between; on a junctions="explicit" sheet each link is its own chain and only junction members
meet -- see the proposal section 5 and MapView.cpp::drawLinks), the rounded rivers (side_chains +
rounded_path: quadratic corners; every line id containing "river" when MapStyle names "river"), straight
boundaries, labels with anchor/angle/halo/spacing, panels (rect, text, boxes, tracks, tables as the Python
draws them), regions (tint, outline, label). Where the Python's output has an order, keep it, so the
normalised SVGs compare element for element. Build the golden test first with an empty builder so you
watch it fill. House style and build/test as in CLAUDE.md (2-space, Allman, banners top and bottom, Yoda,
trailing-P, throw not assert, exhaustive switch, no silent defaults, small files: split MapSceneBuilder by
layer). Update the resume line after every green build. Finish with status: review, the "API changes"
list, and the golden's remaining differences if any (say which sheet, which layer, which element).

- 2026-09-23 W6: LineGeometryTest (9), MapSceneBuilderTest (8: layer order, patterns, hit index, two refusals,
  MapStyle, buildings seed, SVG well-formed + JSON), hexview_contracts target still compiles; clang-format run on
  the new files (see open question 7); full ctest 292/292 incl. hygiene_banner_check. status: review.

- 2026-09-24 W6, pass 2 (coordinator: SheetDoc now carries <legend>; VL's legend restored, so the VL golden had
  been passing vacuously): legend marks drawn. MapMarks.cpp transcribes mark_svg -- shape marks are the MARK_SHAPES
  body placed through the reference's printed transform (translate %.2f, rotate %.1f, scale %.2f; body %.3f),
  strokes and dashes scaled, currentColor = glyph/edge colour else mark colour else #333 (glyph) / #fff (edge);
  "pictogram" draws the named symbol (default dot), "text" the mark's name; glyph rot as for symbols, edge rot =
  edge angle when across="edge" else edge angle - 90; an unknown mark id throws naming it. The glyph with a mark
  draws no glyph text (as the reference). svg-golden.py: the sym-mark-* leniency removed -- MINE may define no symbol
  the reference lacks; the library's mark shapes are now SymbolFrame::Mark and never written to <defs>. New test
  MapSceneBuilderTest.RefusesAMarkMissingFromTheLegendNamingIt. VL golden: hexglyphs ref 85 / mine 78 (the 7
  buildings leaves), sideglyphs 3/3, EQUAL; all six EQUAL; tools\build-dev.cmd: 293/293.
  Build note: the earlier "Permission denied" configure failures were CLion (open on this tree) auto-reloading
  CMake whenever a CMakeLists.txt changed; waiting for its cmake to finish fixed it.

Files (all under hexview/): CMakeLists.txt (STATIC hexview + hexview_contracts kept); Style.cpp, Scene.cpp,
MapFrame.cpp, LineGeometry.cpp, SymbolLibrary.cpp, MapSceneBuilder.cpp, SceneWriters.cpp (the seven), plus
internal helpers Shapes.h/.cpp (basic shapes as paths, SVG path-data reader, similarity transforms), MarkShapes.h/.cpp
(the MARK_SHAPES formulas), Buildings.h/.cpp (the scattered-buildings generator, seed documented in the header),
MapLayers.h + MapAreaLayers.cpp, MapMarks.cpp, MapLineLayers.cpp, MapGlyphLayers.cpp, MapTextLayers.cpp, MapPanelLayer.cpp
(MapSceneBuilder by layer), WriterText.h/.cpp, SceneJson.cpp (writeJson: a faithful minimal form -- layers,
primitives, hit tags, used symbols; no pattern tiles yet). test/: CMakeLists.txt, TestSupport.h, svg-golden.py
(renders the reference SVG and grid geometry into cmake-build-*/hexview/test/out AT TEST TIME, and normalises +
compares; its docstring states the normalisation), StyleTest, SceneTest, MapFrameTest, LineGeometryTest,
SymbolLibraryTest, MapSceneBuilderTest, MapSvgGoldenTest (label golden; the others core).

Golden, remaining differences from the reference: NONE on any of the six sheets, any layer, beyond the two agreed
ones: (a) buildings groups of city hexes on urban="buildings" sheets (TRC 21, PGG 12, SMW 20, VL 2) are the C++
generator's, matched by count (4-5) and bounds; hexview draws each as ONE path of 4-5 rectangles where the reference
writes a <g> of <rect>s; (b) the writer's SVG differs in form only (one flat <path>/<text>/<use> per primitive,
groups flattened, transforms applied, class/data-* attributes absent, font-family on each text) -- the normaliser
removes exactly those differences. Legend marks (VL's 27 mark glyphs, 3 bridge bars) now match element for element.

## API changes (worker: every contract header touched, and why)
1. Scene.h: PathCommand gains ArcTo{rx, ry, rotationDegrees, largeArcP, sweepP, to}. The reference's symbols use
   circles, rounded rects and SVG arcs (fire-intense, port's anchor), and panels are rounded rects (rx=2); none can
   be drawn exactly with Move/Line/Quad.
2. Scene.h: TextShape gains `TextBaseline baseline` (new enum in Style.h: Alphabetic, Central, Middle) -- ids, glyph
   text and labels are dominant-baseline="central", panel and edge text alphabetic; the contract had no way to say.
3. Scene.h / Style.h: TextShape gains `std::optional<Stroke> halo` and Font loses `bool haloP`. The reference has two
   different outlines (labels: white, size*0.18, opacity 0.85; edge labels: black, 0.3); a flag cannot carry them.
4. MapSceneBuilder.h: the constructor drops `const HexModel::Board&` (and the hexmodel/Board.h include). A Board
   needs a RuleSet and a PackageDoc, which the editor (the motivating client) and two of the six golden sheets
   (SMW, VL) do not have; the Board was used only for hex indices, which the frame can mint.
5. MapFrame.h: adds `gridOf(HexId)` and `index(HexId) -> HexModel::HexIndex` (grid by grid, each grid's ids() in
   order -- exactly BoardBuilder.cpp's numbering, read there to confirm), and documents the corners() order (the
   reference's: clockwise from 0 deg flat / 30 deg pointy), which the golden needs.
6. LineGeometry.h: adds `struct LinkNode {HexId hex; std::string key;}` and a smoothedLinks overload over LinkNode
   chains, for junctions="explicit" sheets (VL): two chains crossing a hex without a junction must not join, which
   the HexId-only signature cannot express. The HexId overload is kept (key = hex id).

7. SymbolLibrary.h (pass 2): SymbolFrame gains Mark, for the library's legend-mark shapes ("mark-<shape>"), so the SVG
   writer can leave them out of <defs> as the reference does (it draws marks inline) while the library still lists
   every reference shape.

## Open questions for review
1. RESOLVED 2026-09-24: SheetDoc now carries the legend (coordinator); marks are drawn and the VL golden covers
   them. Nothing missing from SheetMarkDoc for drawing.
2. Bulk <hexes> ids that no grid prints (TRC lists A0, AA0 ... row 0) are skipped, as the reference does (it warns);
   everything else unresolved throws. Keep, or make the sheet clean and throw?
3. MARK_SHAPES has EIGHT keys (rect bar ellipse diamond triangle cross arrow star), not nine as the task says; the
   test reads the keys from the .py, so it follows whatever the reference has. They are in the library as
   "mark-<shape>" (at the default 0.3 x 0.3), since "arrow" and "star" collide with SYMBOLS names.
4. counters2svg.py's face symbols are not in SymbolLibrary::reference() yet (not in this task's inputs); the M10
   test list's HasEveryReferenceSymbol wants the union -- the face half should add them.
5. SheetPanelDoc keeps texts/boxes/tracks/tables in separate vectors, so a panel that interleaves kinds would draw in
   a different order from the reference (none of the six does; checked). A child-order field in SheetDoc would fix it.
6. <label path="..."> (text along a path) throws "not drawn by hexview yet": TextShape has no path form and no sheet
   in view uses it.
7. clang-format 22 (VS 18) reads the repo's AlwaysBreakAfterReturnType: TopLevelDefinitions as NOT breaking
   functions inside a namespace, unlike the house code; the new .cpp files were formatted with
   BreakAfterReturnType: AllDefinitions (return type on its own line everywhere, as in hexmaped/SheetFrame.cpp).
   .clang-format itself was not touched (outside hexview/).
8. Scene::hitAt skips primitives tagged NoHit (grid lines, labels, glyphs), so they never hide the hex under them.

Copyright Ben Paul Wise. All Rights Reserved.
