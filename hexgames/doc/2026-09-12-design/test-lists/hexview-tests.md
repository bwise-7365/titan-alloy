Copyright Ben Paul Wise. All Rights Reserved.

# hexview test list (M10) [PROPOSED]

Headless Google Tests, label `core` (goldens: `golden`), no Qt. "Reference" means the Python renderers
(`map_graphics/xml/hexsheet2svg.py`, `unit_graphics/xml/counters2svg.py`) run by a small script under
ctest, as `hexcoord/test/reference-centres.py` does for GridTest. Sheets: TRC and PGG (settled), then
Dai Senso after M6d.

## Style and Scene
- `StyleTest.ParsesPaletteColours` / `RefusesNoneAndMalformed` (message names the palette id).
- `SceneTest.LayersKeepInsertionOrder`; `HitAtPrefersTheTopLayer`; `HitAtOffEverythingIsNoHit`;
  `RefusesNonPositiveSize`.

## MapFrame
- `MapFrameTest.CentresMatchReferenceOnFourSheets` (every printed id, within 0.01 px).
- `CornersAndHexsideEndsMatchReference`; `EdgeRefParsesAndRefusesBadDirection`;
  `HexAtInvertsCentre` (jittered inside each hex); `TwoGridSheetSharesOneLattice` (Dai Senso seam).

## LineGeometry (the visual-parity core)
- `SmoothedLinksStraightThroughPassesTheCentre` (a straight chain is midpoint to midpoint through centres).
- `SmoothedLinksCutTheCornerAtABend`; `EndsAndJunctionsMeetAtCentres`; `PassThroughLoopCloses`.
- `SmoothedLinksMatchReferenceOnTrcAndPgg`: every link path's points equal the reference SVG's to 0.01 px.
- `CornerChainsMatchCornersDespiteFloatNoise` (corners from neighbouring hexes join into one chain).
- `RoundedCornersUseQuadraticsThroughMidpoints`; `RoundedClosedChainRoundsItsStart`.
- `RiversMatchReferenceOnTrcAndPgg` (the rounded river path's commands equal the reference's).
- `BoundariesStayStraight` (border, zone, mountain, blocked, coast lines: straight along hexsides).

## SymbolLibrary and faces
- `SymbolLibraryTest.HasEveryReferenceSymbol` (names equal the union of both renderers' symbol tables).
- `FaceResolverTest.BackInheritsFrontStyle`; `DerivedBacksBecomePlainFaces`; `RefusesUnknownCounter`.
- `FaceLayoutTest.SlotsAndSizesMatchReference`; `CountersSvgMatchesReferenceOnPgg` (golden).

## Builders
- `MapSceneBuilderTest.LayerOrderIsTheReferenceOrder`; `TerrainPatternsAppearWhereDeclared`;
  `RefusesDanglingPaletteColourNamingItsLine`; `MapStyleReferenceRoundsOnlyRivers`.
- `MapSvgGoldenTest.TrcAndPggMatchReference` (writeSvg of the map scene against the reference SVG,
  after both are normalised to the same attribute order; label golden).
- `StateSceneBuilderTest.StacksDrawInPositionOrder`; `ControlLayerFollowsPosition`;
  `ConcealmentHidesFromAllOnPgg` (Untried units show their backs to both players);
  `ConcealmentHidesIdentityFromEnemyOnTarawa` (after M9); `AllSeeingViewShowsFronts`.

## Interaction
- `InteractionMachineTest.EmitsOnlyFacadeLegalCommands` (random intents over a TRC session: every
  emitted command is applied without an exception).
- `PathOutsideReachableEmitsNothing` (refusal names the hex); `AttackOnlyOnListedTargets`;
  `DecisionModeOffersOnlyTheAnswers`; `CancelReturnsToIdle`; `GameOverAcceptsNothing`.

## Replay, animation, JSON
- `ReplayControllerTest.PositionsMatchALiveSession` (every move of trc-test, digest equal);
  `SeekBackwardsUsesTheCache`; `RefusesARecordThatDiverges` (names the move).
- `AnimationPlanTest.MoveBecomesOneTweenAlongThePath`; `RevealIsAFlip`; `NonVisualEventsAddNoBeat`.
- `SceneWritersTest.JsonRoundTripsLayersPrimitivesAndHitTags`; `SvgIsValidXml`.

Copyright Ben Paul Wise. All Rights Reserved.
