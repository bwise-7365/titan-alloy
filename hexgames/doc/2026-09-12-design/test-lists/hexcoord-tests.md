Copyright Ben Paul Wise. All Rights Reserved.

# hexcoord — test list (contract for W1)

Files: `hexcoord/test/AbcTest.cpp`, `DirectionTest.cpp`, `HexAddressTest.cpp`, `GridTest.cpp`,
`HexIdFormatTest.cpp`, registered by `hexgames_add_gtest(... LABELS core)`. Random cases use
`std::mt19937_64 rng(20260912u)` and `std::uniform_int_distribution<int>(-10, 20)`, 400 iterations,
mirroring `panj/hexmap/src/testtri.cpp`.

Ported from `testtri.cpp` (each line is one TEST):
- `AbcTest.ReduceIsIdempotentAndCanonical` — constructing from (a,b,c) and from (a+d,b+d,c+d) yields
  equal Abc; height() is minimal over the three zeroings; ties zero a, then b, then c.
- `AbcTest.DiagonalShiftIsSameClass` — `testCoordConv`: equality and equal hvCode across +(d,d,d).
- `AbcTest.OffsetHeightEqualsEdgeDist` — `verifyOffSet`: height of a difference == edgeDist.
- `HexAddressTest.VertexClass1And4HaveCentresAtPlusABC` — code 1/4: v+A, v+B, v+C are centres, each at
  edgeDist 1; `(v+AVec).hvCode()` matches tricoord's case table.
- `HexAddressTest.VertexClass2And5HaveCentresAtMinusABC` — code 2/5: v-A, v-B, v-C are centres.
- `HexAddressTest.EveryQrsIsACentre` — `CoordABC(CoordQRS)` code ∈ {0,3} for random Qrs.
- `GridTest.RowClmRoundTripFlat` — `getRC` round trip, reduced and unreduced inputs.
- `AbcTest.StraightLineEdgeDistIsTwiceHexDist` — along pure Q, R, S multiples.
- `AbcTest.EdgeDistBounds` — 3·hexDist ≤ 2·edgeDist ≤ 4·hexDist for random centre pairs.
- `GridTest.FlatDecompositionEven` / `FlatDecompositionOdd` — even clm=2n → (3n, −r, r); odd 2n+1 →
  (3n+1, −(r+1), r); tricoord's `CoordABC(row, clm)`.
- `GridTest.RowClmQrsRowClm` — `testHexRowClm`: (row,clm) → Qrs → (row,clm).

New:
- `DirectionTest.RotateIsCyclicAndOppositeIsThree` — rotate(d,6)==d, opposite==rotate(d,3),
  step(opposite(d)) == step(d)*(-1).
- `DirectionTest.CompassNamesPerOrientation` — flat n ne se s sw nw; pointy ne e se sw w nw; round
  trip through fromCompass; a pointy name in flat throws.
- `DirectionTest.DirectionOfUnitStepsOnly` — directionOf(step(d)) == d; directionOf(QVec*2) throws.
- `HexAddressTest.EdgeBetweenAndHexesRoundTrip` — `HexEdge::between(h, d).hexes()` is {h, h.neighbour(d)};
  vertices are at edgeDist 1; `of(a, b)` with non-adjacent vertices throws.
- `HexAddressTest.VerticesAreClockwiseAndShared` — each hex has 6 vertices; adjacent hexes share 2.
- `HexAddressTest.RingAndDiscCounts` — ring(r) has 6r hexes at distance r; disc(r) has 3r²+3r+1.
- `HexIdFormatTest.RendersAllFourGames` — `{rowletter}{col}` with colStart 33, colStep −1 → "KK19"
  for the Kerch hex; `{row:02}{col:02}` rowStart 25 rowStep −1 colStart 2 → "2336"; `w{row:02}{col:02}`
  rowStart 61 rowStep −1 → "w5227"; `{col:02}{row:02}` → "0109"; letters(27)=="AA", letters(0) throws.
- `GridTest.PointyRoundTrip` — centreOf/indexOf for every cell of a 7×7 pointy grid, both parities.
- `GridTest.PixelMatchesRendererFourSheets` — for each sheet in `map_graphics/xml/*.xml`, the grid
  built from its `<grid>` element gives the same centres (±0.01 px) as hexsheet2svg.py for a fixed
  sample of ids: TRC A33, KK19, KK20 (adjacent), QQ1; DDaT 2336, 2502, 0143; DS w5227, w6101, e3411,
  and a w/e seam pair; PGG 0101, 0109, 5631. Reference values are computed once by a small Python
  script and embedded as constants, with the script's name in a comment.
- `GridTest.HexAtInvertsPixelOf` — hexAt(pixelOf(h) + jitter up to 0.4·size) == h for all cells;
  a pixel outside returns nullopt.
- `GridTest.ClipRemovesIds` — clipped ids are absent from ids() and find().
- `GridTest.TwoGridsShareOneLattice` — a second grid whose origin is off-lattice throws; DS's real
  east grid is on-lattice with the west grid and their seam hexes are neighbours.

Copyright Ben Paul Wise. All Rights Reserved.
