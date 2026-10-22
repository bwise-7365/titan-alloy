Copyright Ben Paul Wise. All Rights Reserved.

# Task 01: hexcoord — implement the ABC coordinate module (milestone M2)

status: done   (reviewed and accepted by the coordinator 2026-09-13: API notes 1-5 adopted; qrs() comment fixed)
worker: W1 (opus)            started: 2026-09-13
resume: DONE, awaiting review. All four contract headers are implemented and all 27 tests of the
  test list (plus five extra) pass; `tools\build-dev.cmd win-msvc-debug` is green at 35/35 and
  `banner-check.py` reports 0 failures. Next: the coordinator reviews the API notes below (the
  lattice tolerance and the Parity::Even origin) and commits.
inputs:
  hexgames/hexcoord/Abc.h, Direction.h, HexAddress.h, Grid.h, CMakeLists.txt   (the contract)
  hexgames/doc/2026-09-12-design/test-lists/hexcoord-tests.md                   (the tests to write)
  hexgames/uml/hexcoord-classes.puml
  hexgames/CLAUDE.md, hexgames/.clang-format, hexgames/cmake/HexGamesGtest.cmake
  C:/repos/ghub-per/panj/hexmap/libsrc/tricoord.h, tricoord.cpp                  (semantics to reproduce)
  C:/repos/ghub-per/panj/hexmap/src/testtri.cpp                                  (assertions to port)
  hexgames/map_graphics/xml/hexsheet2svg.py  class Grid (make_id, centre, neighbour, letters, polygon)
  hexgames/map_graphics/xml/the-russian-campaign.xml, d-day-at-tarawa.xml, dai-senso.xml,
    panzergruppe-guderian.xml  -- only their <grid> elements, for the four-sheet pixel test
outputs:
  hexgames/hexcoord/Abc.cpp, Direction.cpp, HexAddress.cpp, Grid.cpp
  hexgames/hexcoord/CMakeLists.txt  (STATIC, sources listed, test/ subdirectory)
  hexgames/hexcoord/test/CMakeLists.txt, AbcTest.cpp, DirectionTest.cpp, HexAddressTest.cpp,
    HexIdFormatTest.cpp, GridTest.cpp, reference-centres.py
  inline definitions added to Direction.h and HexAddress.h where a function is declared constexpr
acceptance:
  tools\build-dev.cmd win-msvc-debug  ->  ctest 100% (label core includes the new suites)   MET (35/35)
  hygiene_banner_check green; no file outside hexgames/hexcoord touched                     MET
  every public declaration in the four contract headers is defined; any API change is listed here  MET

## What the module now says

- **Pointy-topped offsets.** The flat formula is tricoord's `CoordABC(row, clm)` unchanged. Its
  pointy counterpart was derived (not guessed) from the requirement that the pixel mapping reproduce
  `hexsheet2svg.py`'s `Grid.centre()`: row 2m -> `(col, -3m, -col)`, row 2m+1 -> `(col+1, -(3m+1),
  -col)`. Inverting it needs a different discriminant from the flat one: the row index is `q - s`
  where the flat column index is `r - s`.
- **The pointy pixel basis** is the flat one turned 30 degrees *clockwise on screen* (y down). That
  sign is the one that makes D0..D5 read `ne e se sw w nw`, matching the renderer's `edges` table;
  `DirectionTest.EdgeAnglesAreTheRenderersEdgeOrder` pins it.
- **Vertices and hexsides are orientation-free.** The six vertices of a centre, clockwise from the
  leading corner of D0, are `+B, -C, +A, -B, +C, -A` for both orientations, and vertices k and k+1
  bound the hexside in direction Dk -- because every step vector is the sum of two vertex offsets
  (D0 = -Q = B - C, D1 = +R = A - C, and so on).
- **`hexAt`** inverts the basis in the sum-zero ABC frame, which *is* the cube frame of a hex grid,
  and cube-rounds there; a centre always has a sum-zero representative because its hvCode forces
  a + b + c to be a multiple of three.

## API changes and additions (for review)

1. `Grid.h` gained the offset frame as two free functions, `abcOfIndex(GridIndex, Orientation,
   Parity)` and `indexOfAbc(HexCentre, Orientation, Parity)`. The ported tricoord cases run over
   negative rows and columns, which no bounded `Grid` can express, and the sheet boundary is the
   only place offset coordinates are allowed to exist, so they live beside `Grid`.
2. `Grid.h` gained `Grid(GridSpec, const Grid& sheetLattice)`, `Abc latticeOffset() const` and
   `inline constexpr double kLatticeTolerance = 0.05`. **The brief's 1e-6 * size will not do**: Dai
   Senso's real east grid sits 0.048 px -- 0.0012 * size -- off the west grid's lattice, because the
   sheet XML origins are fitted to a scan by hand. A lone `Abc latticeOffset` argument carries no
   pixel information, so the pixel check cannot live in the one-grid constructor: that one throws
   unless the offset is a hex centre, and the new two-grid constructor reads the offset off the two
   origins and throws when they disagree by more than the tolerance (or when orientation or size
   differ). `GridTest.TwoGridsShareOneLattice` covers both.
3. `Grid` private additions: `nearestCentre`, `offsetFor`, `renderedId`, `locate`, `clippedIds`, and
   the members `format_`, `origin_` (the pixel of lattice point `Abc{}`) and `byIndex_`.
4. `Direction.h` gained `#include <stdexcept>`; the constexpr functions end in a throw after their
   exhaustive switch, since a switch with no default has no other way to return.
5. `HexCentre::qrs()`: the header's comment gives `q = (c - b) / 3`, which holds only for the
   sum-zero representative and is not integral in general (Abc{1,0,-1} is R, but (c-b)/3 = -1/3).
   The implementation shifts the three differences by the one residue that makes all three divisible
   by three, and is exact for every representative; `HexAddressTest.EveryQrsIsACentre` round-trips
   400 random Qrs through it. The comment in the header is now the only inaccurate line left.

## Open question for Ben or the coordinator

`offset="even"` places grid cell (0, 0) at pixel `(ox, oy)`, as the brief directs ("subtracts the
base so that grid cell (0,0) maps to Abc{}"). `hexsheet2svg.py` would draw that same cell half a hex
along the shifted axis, since it adds the jog to cell (0, 0) itself. Every relative distance agrees
with the renderer -- `GridTest.PixelLayoutMatchesRendererForBothParities` checks all four
orientation-and-parity combinations cell by cell -- so only the meaning of `ox, oy` on an even-offset
grid is at stake, and no sheet uses `offset="even"` today.

## Tests

27 TESTs in five suites -- every one the test list names, plus three: `AbcTest` 6, `DirectionTest` 4,
`HexAddressTest` 6, `HexIdFormatTest` 1, `GridTest` 10. The three extra are
`AbcTest.PrintsLikeTricoord`, `DirectionTest.EdgeAnglesAreTheRenderersEdgeOrder` and
`GridTest.PixelLayoutMatchesRendererForBothParities`, the last being the only cover for
`Parity::Even` geometry. The polygon and inset cases ride along in
`GridTest.PixelMatchesRendererFourSheets`. Random cases use `std::mt19937_64 rng(20260912u)` over
-10..20, 400 iterations, as testtri does.

The reference pixels of `GridTest.PixelMatchesRendererFourSheets` were printed by
`hexcoord/test/reference-centres.py`, which drives `hexsheet2svg.py`'s own `Grid` class over the four
sheets; the script is named in a comment at the head of `GridTest.cpp` and is not part of the build.

log:
- 2026-09-13 created by the coordinator
- 2026-09-13 W1: baseline green (8 tests) before any edit.
- 2026-09-13 W1: Abc.cpp, Direction.{h,cpp}, HexAddress.{h,cpp}, Grid.{h,cpp} written; CMakeLists
  INTERFACE -> STATIC; library and smoke test green (8 tests), no warnings under /W4.
- 2026-09-13 W1: five test suites and test/CMakeLists.txt added; `tools\build-dev.cmd win-msvc-debug`
  green at 35/35, zero compiler warnings, `banner-check.py` 71 files 0 failures. status -> review.

Copyright Ben Paul Wise. All Rights Reserved.
