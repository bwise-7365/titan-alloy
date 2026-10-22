Copyright Ben Paul Wise. All Rights Reserved.

# hexmaped test list (task 16, HexMapEd) [AS BUILT 2026-09-21 for step 1 and save]

Headless Google Tests, label `core`, no Qt: the editor's model is a Qt-free library and the widgets
only turn clicks into its edits. Fixtures are the committed sheets under `map_graphics/xml/`, reached
through `HEXGAMES_SOURCE_DIR`. The widget suite (label `gui`, `hexgames_add_gtest(... GUI)`) comes with
the later steps.

## Schema
- `SchemaTest.SymbolsEqualTheXsd` / `SlotsEqualTheXsd` / `DirsEqualTheXsd` / `PatternsEqualTheXsd` /
  `EndReasonsEqualTheXsd` (the lists are read from hexsheet.xsd itself, so the editor cannot drift
  from the schema).
- `SchemaTest.RefusesValuesOutsideTheVocabulary` (symbol, slot, HexRef, RGB; message names the value).

## SheetFrame
- `SheetFrameTest.EveryHexsideFacesItsDirection` (SMW pointy and PGG flat: the midpoint of the side
  named by a Direction lies on the renderer's edge angle, within a degree).
- `SheetFrameTest.NeighboursShareTheHexside` (the same midpoint from both hexes; one canonical name).
- `SheetFrameTest.TokensRoundTripAndBadOnesThrow` (`1028:ne`; no `n` on a pointy sheet; unknown id).
- `SheetFrameTest.HexAtInvertsCentreAndHexsideAtFindsTheNearSide`.

## SheetWriter
- `SheetWriterTest.RoundTripsEverySheetInTheTree` (load, write, load: grids, palette, terrains, lines,
  bulk terrain, edges, links, paths, hexes with glyphs, regions, labels, panels come back equal).
- `SheetWriterTest.WritesTheSchemaOrderAndEscapesText` (grid, palette, terrains, lines, content; `&`,
  `<`, `>`, `"`; LF line ends).
- `SheetWriterTest.RefusesAnUnwritablePath`.

## Document
- `DocumentTest.LoadsAndAnswersTerrain` (bulk assignment or the grid default).
- `DocumentTest.SetTerrainMovesTheHexAndUndoRestoresIt` (one bulk list holds the hex; the default
  terrain means no list; undo, undo, redo).
- `DocumentTest.RefusesWhatTheSheetDoesNotDeclare` (undeclared terrain, line, colour; unknown hex;
  symbol and slot outside the schema; a link step between non-neighbours; nothing changes and the
  document stays clean).
- `DocumentTest.ToggleEdgeAddsThenRemovesOnEitherSpelling` (HEX:DIR from either hex is one hexside).
- `DocumentTest.LinkStepsExtendSplitAndDropShortChains` (a step extends the chain it touches; removing a
  step splits it; a one-hex remainder is dropped, as assemble.py's chains() would).
- `DocumentTest.GlyphsNamesRingsAndClip` (add and remove a glyph; name; ring; clip a hex out of the
  frame and back).
- `DocumentTest.AddHexAtRestoresAClippedCellAndGrowsTheGrid` (a clipped cell returns by its pixel; a
  click one column outside grows the grid by one column, every old hex keeps its id and centre, the
  other new cells stay clipped; undo shrinks it back).
- `DocumentTest.SaveWritesWhatLoadsBack`.

## Gate (manual until the gui suite exists)
- Open each of the six committed sheets in HexMapEd, save without edits, run tools/validate-xml.py on
  the folder: all valid; hexsheet2svg.py renders the saved file.

Copyright Ben Paul Wise. All Rights Reserved.
