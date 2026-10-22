Copyright Ben Paul Wise. All Rights Reserved.

# HexMapEd -- using the map sheet editor

HexMapEd opens a hexsheet XML document (`map_graphics/xml/hexsheet.xsd`), draws it, lets you change the
structure of the map by clicking, and saves XML that validates against the schema. It only offers what
the schema defines: the terrains, lines and colours the sheet declares, and the schema's own symbols,
slots and directions. Anything else is refused with a message in the status bar, and nothing changes.

## Quick start

1. File > Open sheet, and pick a sheet (`map_graphics/xml/*.xml`).
2. Mouse wheel zooms; in Select mode, drag to pan; F fits the whole sheet.
3. Pick a Mode on the toolbar, then a Kind, and click on the map. Each click is one edit; Ctrl Z undoes.
4. File > Save writes the XML and checks it against the schema; the status bar says whether it is valid.
5. File > Export SVG writes a picture of the map as you see it.

## Build and run

```
cmake --preset win-msvc-debug
cmake --build --preset win-msvc-debug --target HexMapEd
cmake-build-debug\hexmaped\app\HexMapEd.exe [path\to\sheet.xml]
```

The Qt6 debug DLLs and `platforms\qwindowsd.dll` are placed beside the executable by the build, so it
runs from there. A sheet path on the command line opens it at once; otherwise use File > Open sheet.
The reader's sheets are at `map_graphics/xml/tools/reader/work/<map>/chain/sheet.xml`, the committed
sheets at `map_graphics/xml/*.xml`.

## The window

- **Map view** (centre): the sheet drawn in the renderer's layer order: terrain fills, region tints,
  hexside lines, links, rings, glyphs, names, labels, and every printed hex id.
- **Toolbar**: Mode, Kind, Slot, Link kind, Waviness. Waviness (0-10) draws roads, railways and rivers
  hand-scratched, as irrgo draws its Latrunculi boards (perpendicular noise, relaxed; hexview/Scratch.h,
  HexView::MapStyle::scratch); 0 is straight. Presentation only: the document is not changed.
- **What you see** is hexview's Scene (the same builder writes the SVG and will draw the game GUIs), painted
  by app/ScenePainter.cpp: every symbol, legend mark, city, rail tick and pattern is the reference's. Kind lists what the current mode paints; it is filled from
  the sheet (terrains, lines) or the schema (symbols).
- **Doubts** dock (right): the reader's doubts, one per line as `kind  at  why`, loaded from a
  `doubts.json` beside the sheet if there is one, or from File > Open doubts. Clicking a doubt centres
  the view on its hex, outlines it in magenta and fills the Hex dock.
- **Hex** dock (right): the hex under the last click or doubt: its terrain, name, ring, glyphs, the
  lines on its hexsides, and the links that pass through it.
- **Status bar**: the hex under the mouse with its terrain and the sheet pixel, or the last refusal.

## Moving around

| Action | How |
|---|---|
| Zoom | mouse wheel (around the cursor), View > Zoom in / out, Ctrl + / Ctrl - |
| Fit the whole sheet | View > Fit, or F |
| Pan | in Select mode, drag with the left button; in every mode the scroll bars work |
| Hide or show hex ids | View > Show hex ids |

## Editing

Pick a mode in the toolbar. Every click is one edit, and Edit > Undo (Ctrl Z) and Redo (Ctrl Y) step
through them exactly.

| Mode | Kind | Click | Shift-click |
|---|---|---|---|
| Select (pan) | - | drag pans; a click fills the Hex dock | |
| Terrain | a terrain of the sheet | the hex takes that terrain | |
| Hexside line | a line of the sheet (river, border, rail ...) | the nearest hexside of the clicked hex gets that line, or loses it if it already has it | |
| Link | a line of the sheet; Link kind names the link (rail, road ...) | first click picks the start hex; each further click on a neighbour adds a step and the chain continues from there; a click on the current hex ends the chain | removes the step between the current hex and the clicked neighbour, splitting the chain; on the current hex itself, deletes the one-hex links there (refused for a link a junction names) |
| Junction | - (uses Link kind) | joins every link of the Link kind that passes through the hex, so they connect there; on a hex that already has such a junction, separates them again. Link ends at the hex follow. Only on sheets with `junctions="explicit"` | |
| Clip | - | a printed hex leaves the grid; an empty cell inside the grid's rectangle joins it; a cell just outside the rectangle grows the grid by one column or row and joins it | |
| Glyph | a symbol of the schema; Slot places it (c, n, ne, e, se, s, sw, w, nw) | adds the glyph to the hex | removes the hex's last glyph |
| Name | - | a dialog sets or clears the hex's name | |

Notes on the modes:

- A hexside is shared by two hexes. The editor stores it once, under the lower-indexed hex, and finds
  it from either side, so clicking the same side from the neighbour removes it.
- A link chain that loses a step becomes two chains. A piece left with one hex is kept (a railway leaving
  the map is recorded as a one-hex link, so the editor never throws one away); the second piece takes the id `<id>-2` and the junctions on it, and both
  new ends read `unexplained`. To delete a whole link: shift-click away each of its steps, then click
  each remaining hex and shift-click it again, which deletes its one-hex link.
  Chains are stored as the schema's `link` elements with the hexes in order.
- Adding a hex outside the grid moves the grid's origin and numbering start with the new column or
  row, so every existing hex keeps its id and its position; the other cells of the new column or row
  stay clipped until you click them.
- Terrain the grid declares as its default (usually clear) is stored by omission: painting a hex clear
  removes it from every terrain list.
- Rings and side glyphs are drawn and preserved but not yet editable; regions, labels, styles and
  panels likewise (see "Not yet").

## Saving

File > Save (Ctrl S) writes the sheet in the schema's element order: grid, palette, terrains, lines,
then terrain lists, hexside lines, paths, links, regions, hexes with glyphs, labels, panels. The
document carries both the structure and the style declarations the renderers use, so `hexsheet2svg.py`
draws the saved file as it stands. After writing, the editor runs `tools/validate-xml.py` on the
folder the file is in and shows the result in the status bar, or a warning box with the validator's
message if anything fails. File > Save as writes to a new path; `xsi:noNamespaceSchemaLocation` is
written relative to wherever the file lands, so a sheet saved under `tools/reader/work/...` still
names the schema. Closing with unsaved edits asks first.

File > Export SVG writes the map exactly as the window draws it (the same hexview Scene, the current
Waviness and the View > Show hex ids choice included) to an SVG file; the sheet itself is not changed.

Help > About HexMapEd (or the About button on the toolbar) shows this guide inside the program.

To render a saved sheet:

```
python map_graphics\xml\hexsheet2svg.py path\to\sheet.xml path\to\sheet.svg --png
```

## A typical session: correcting a sheet

1. Open the sheet and put the printed map (or its scan) beside the window.
2. Work down the map a region at a time, at a zoom where the hex ids are readable.
3. Terrain mode: click any hex whose fill is wrong, with the right terrain chosen as Kind.
4. Hexside line mode: click a hexside to add the chosen line (river, border ...) or to remove it.
5. Link mode, with Link kind set (rail, road, river ...): click the start hex, then each next hex along
   the line; click the last hex again to end the chain. To remove a step, click one end of it, then
   shift-click the other end. To remove a leftover one-hex piece, click that hex, then shift-click it.
6. Junction mode, with Link kind set: click a hex where two lines of that kind should connect (a railway
   branching, a river flowing into another). Lines that only cross there stay separate until you do.
7. Glyph and Name modes for cities and other places.
8. Save, and read the status bar: "saved and valid". Export SVG to share the result.

## Not yet

Lattice pinning against the scan, the scan as an underlay, regions and labels, editing rings and side
glyphs, style declarations (new colours, terrains, lines), panels, more than one grid (Dai Senso), and
the automated GUI test suite. These are the remaining steps of `tasks/16-sheet-editor.md`.

Copyright Ben Paul Wise. All Rights Reserved.
