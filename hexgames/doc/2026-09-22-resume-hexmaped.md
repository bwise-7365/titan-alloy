Copyright Ben Paul Wise. All Rights Reserved.

# Resume note, 2026-09-22: HexMapEd and the reader track

Written before a reboot. Everything below is on disk; nothing depends on a live session.

## State of the working tree (UNCOMMITTED, staged with git add)

Ben committed the reader track through the three-map run on 2026-09-21. Since then, staged and not
committed:

- `hexmaped/` new module: `Schema.{h,cpp}`, `SheetFrame.{h,cpp}`, `SheetWriter.{h,cpp}`,
  `Document.{h,cpp}`, `CMakeLists.txt`, `README.md` (the user guide), `test/` (four gtest suites,
  21 tests, all green), `app/` (`Main.cpp`, `MainWindow.{h,cpp}`, `MapView.{h,cpp}`, `CMakeLists.txt`).
- Root `CMakeLists.txt`: `add_subdirectory(hexmaped)` before the GUI block.
- `cmake/HexGamesQtDeploy.cmake`: `$<IF:$<CONFIG:Debug>,--debug,--release>` (the two conditional
  expressions handed windeployqt an empty "" argument and the link edge failed).
- `uml/hexmaped-classes.puml`, `doc/2026-09-12-design/test-lists/hexmaped-tests.md`,
  `tasks/16-sheet-editor.md` (status in progress, resume line current), `PLAN.md` (RESUME HERE entry
  "2026-09-21 night", decision-log entries of 2026-09-21).
- Reader files from the same day, also staged: `tools/reader/{lattice,batch,structure,sheet}.py`,
  `anchors.json`, `README.md`, `map_graphics/xml/hexsheet.xsd` (clip documentation: no holes rule).

First thing after the reboot: `git status --short | grep -v "^??"` and commit the pile.

## Building (the compiler is not on the bash PATH)

`cl` and `ninja` are not on PATH in the Git Bash the sessions use. The recipe that works:

```
@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d C:\repos\ghub-per\titan-alloy\hexgames
cmake --preset win-msvc-debug
cmake --build --preset win-msvc-debug --target HexMapEd hexmaped_document_test hexmaped_schema_test hexmaped_sheetframe_test hexmaped_sheetwriter_test
```

Save that as a .bat and run it with `cmd //c path\to\build.bat`. Ninja is CLion's
(`C:/Program Files/JetBrains/CLion 2026.1/bin/ninja/win/x64/ninja.exe`, already in CMakeCache). Qt is
`C:/Qt/6.8.3/msvc2022_64` (the preset sets Qt6_DIR). Once, the configure step failed with
"No SOURCES given to target: gtest" and passed unchanged on the next run; if it recurs, rerun before
suspecting anything.

Run the tests directly: `cmake-build-debug/hexmaped/test/hexmaped_*_test.exe`. Run the app:
`cmake-build-debug/hexmaped/app/HexMapEd.exe map_graphics/xml/tools/reader/work/Target_Leningrad_map/chain/sheet.xml`
(Qt debug DLLs and platforms/ are beside it). Offscreen smoke test:
`QT_QPA_PLATFORM=offscreen timeout 6 ./cmake-build-debug/hexmaped/app/HexMapEd.exe <sheet>` exits 124.

## What HexMapEd does today (task 16 build order)

Done: step 1 (view in renderer layer order, pan, zoom, hover id, doubts dock, inspector), step 3
(terrain paint; clip: remove a hex, restore a clipped cell by clicking it, grow the grid by a column or
row by clicking just outside, every old id and place kept), step 4 (hexside line toggle, one storage
per shared side), step 5 (link chains by clicking neighbours, shift-click removes a step and splits),
step 6 in part (glyph by symbol and slot, name; rings in the model only), step 10 in part (save in
schema order, then `tools/validate-xml.py` on the folder; undo/redo as whole-document snapshots).
Rules from Ben (2026-09-21): only structures the XSD defines; output validates; structure and style
both in the document. `Document` enforces the first at every edit; `hexmaped_schema_test` reads
hexsheet.xsd and fails if the C++ vocabularies drift.

Not done: step 2 (lattice pinning over the scan), scan underlay, step 7 (regions, labels), step 8
(styles: new colours, terrains, lines), step 9 (panels), editing rings and side glyphs, second grid
(Dai Senso), the gui test suite (`hexgames_add_gtest(... GUI)` exists, unused).

## What Ben was doing

Ben started HexMapEd on the Target Leningrad reader sheet and found hex 0801 and others missing at the
grid edge; Clip mode now adds them. His next session: add the missing cells, then the rail network by
hand in Link mode (Kind rail, Link kind rail), save, render with hexsheet2svg.py.

## The reader track (unchanged since the commit, for orientation)

`map_graphics/xml/tools/reader/`: lattice.py (grid fit, numbering from anchors.json, holes filled,
`--printed` by-eye cells), legend.py (`--placed legends.json`), cells.py (colour scoring), structure.py
(hysteresis chains, end reasons, doubts.json), sheet.py (sheet.xml from the lattice numbering),
batch.py (lattice stage only so far). Three maps go scan -> sheet.png: SMW (Ben's bar: editor-
finishable, right structure types), Target Leningrad, Tannenberg (lines over-read). Ben's ruling of
2026-09-21: the goal is the rule language tested by approximations; never parse text boxes; do not
tune detectors. Line kinds draw in conventional colours (rail black, river blue, border red).

## Next, in order

1. Commit the staged pile.
2. Ben's TL session in HexMapEd; whatever it exposes is the next editor work.
3. Then the remaining task-16 steps, lattice pinning first since maps are read one at a time.
4. batch.py should chain lattice -> cells -> structure -> sheet per map.
5. Language items for the XSD proposals list: front line, major river, blocked hexside, double-track
   rail, ring ownership (a colour id today).

Copyright Ben Paul Wise. All Rights Reserved.
