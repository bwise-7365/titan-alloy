Copyright Ben Paul Wise. All Rights Reserved.

# Task 16: read with doubts, then let Ben finish the map by hand (a graphical sheet editor)

status: pending             recorded 2026-09-17 at Ben's request; not started
worker: unassigned          started: -
resume: not started. Step 1 is applied to task 15's running stages from C-river on; steps 2 and 3 wait.
inputs:
  map_graphics/xml/hexsheet.xsd, hexsheet2svg.py (the reference renderer: geometry and layer order)
  hexxml/SheetDoc.{h,cpp} (the loader), hexview/ (the Qt-free geometry and face model), hexqt/ (the Qt seam)
  tasks/15-smw-chain-reading.md (the reading process the editor completes)
  map_graphics/xml/tools/image2sheet/work/smw/chain/doubts.json (the reader's marginal calls, step 1)
outputs:
  a new Qt6 executable (working name sheet_editor_gui; the only new target allowed to link Qt besides
  hexqt and the existing *_gui), its test list in doc/.../test-lists/, uml/*.puml for its model
acceptance:
  load any of the eight sheets; every edit below; save; tools/validate-xml.py passes on the saved file;
  hexsheet2svg.py renders it; a round trip with no edits is byte-stable apart from attribute order.
  First real test: Ben corrects the SMW railway by hand (step 3).

## Why (Ben, 2026-09-17)

The reading process gets most of a map right, and then every attempt to have a model fix the remaining
details tangles them further. It is cheaper for Ben to fix details by hand than to steer an agent to
them. So the division of labour is: the model does the bulk and SAYS WHERE IT WAS UNSURE; Ben finishes
with an editor; the editor saves XML that passes the schema, so the sheet stays a document, not a
picture.

## Step 1 -- the reader records its doubts (applies to task 15 now)

Every reading stage writes `work/MAP/chain/doubts.json`, a list, one entry per marginal call of ANY
kind, not only thresholds:
```
{"at": "1638", "kind": "terrain", "chose": "coastal", "alternatives": ["clear"],
 "why": "sea wedge covers about half the hex", "look": "chain/look/q-1638.jpg", "stage": "T"}
{"at": "1831-1931", "kind": "rail", "chose": "step", "alternatives": ["1830-1931 skipping 1831"],
 "why": "line bends under the bridge symbol", "look": "...", "stage": "Q-rail"}
{"at": "2436", "kind": "end", "chose": "sea", "alternatives": ["edge"], "why": "...", "stage": "C-rail"}
```
Kinds seen so far: terrain near a threshold; which hexes a line passes through near a vertex; a line
along a hexside (which side); an end's reason; a place's hex when city blocks straddle two hexes; a
glyph's kind; a label's anchor; whether a hex is map or furniture. The rule for the reader: if you
would not bet on it, record it, then move on; never spend a second look to remove a doubt. The
coordinator merges the doubts into the sheet as an XML comment block at the end of the document (no
XSD change) so the editor can list them and jump to each.

## Step 2 -- the editor

A small Qt6 application, C++20, in the project's style, reusing the existing pieces rather than
re-implementing: `HexXml::SheetDoc` loads, `hexview` gives the hex geometry and the face model,
hexsheet2svg.py's layer order is the display order. Reference editors to look at for interaction
patterns before designing (Ben: "dozens exist"): Hexographer / Worldographer (terrain paint, feature
stamps), VASSAL's map and grid editors (hexside features, grid numbering), Tiled (layers, brush and
fill), Cyberboard's GameBox designer, HexDraw. Take from them the conventions a wargamer expects,
not their scope.

Scope (Ben, 2026-09-17): EVERY element kind the schema carries, "the works". The editor is the way a
sheet gets finished, so anything the sheet can hold must be editable in it:
  grid        add or delete hexes (the clip), the id format and numbering, more than one grid (Dai Senso)
  hex         terrain; name; ring; glyphs at slots (city, port, star, oil, capital, town, every Symbol);
              side glyphs (Tarawa's fire dots); a hex's glyphs added, removed, re-kinded, moved to a slot
  edge/path   every hexside line kind (river, border, coast, zone, mountain ...), single hexsides and chains
  link        every link kind (rail, road, connectors), chains that split at ends and junctions on save
  region      hex sets with a name, tint and outline, membership by click or drag
  label       text at a hex slot, at a point, or along a path; angle, size, style
  styles      palette colours, terrain fills and patterns, line styles: choose among the declared ones and
              declare new ones (the shape/meaning split of PLAN.md, once it lands, is edited here too)
  panel       furniture: panels, boxes, tracks, tables, moved and resized, their text edited (last)

Build order, each a green build with a test list before the next:
1. View: load a sheet, draw it with the same geometry as the SVG (all layers in the renderer's order),
   pan and zoom, hover shows the printed id, a side panel lists the doubts and clicking one centres the
   view on it. Optional: the scan underneath at the calibration's frame, as a toggled layer, so the fix
   is made against the print.
2. Lattice pinning (Ben, 2026-09-19): in practice one map is read at a time, perhaps one a month, so the
   lattice stage should use each party where it works best. The editor shows the scan with the fitted
   lattice over it and lets Ben drag and zoom the grid, pin a grid hex to the printed hex it belongs to
   (a pin is an anchor: index to position, and to a printed id when he types it), pick among the
   tool's lattice candidates when a texture has won, and mark a fold line. Every pin feeds the refit;
   the tool never overrides a pin. Output: lattice.json with the pins recorded, and the numbering.
3. Terrain paint (pick a terrain, click or drag) and the grid clip (toggle a hex out of or into the map).
4. Hexside lines: pick a line kind, click a hexside to toggle it; chains are kept consistent.
5. Links: pick a link kind, click a sequence of neighbouring hexes to add a chain; click a step to remove
   it; the model keeps chains split at ends and junctions the way assemble.py does.
6. Hex content: glyphs, side glyphs, names, rings; places added, removed, renamed, re-kinded.
7. Regions and labels.
8. Styles: palette, terrains, lines; new declarations validated against the schema's enumerations.
9. Panels and furniture.
10. Save: XML in assemble.py's element order and attribute style, validated against hexsheet.xsd inside
   the application (an error is shown, never written past), the doubts block kept with each doubt marked
   resolved or open. Undo and redo across everything above. Save lands early (after step 3) and every
   later step extends it; it is listed last only because it must cover all of them.
No new XSD element for doubts unless Ben asks; the comment block is enough for the editor.

## Step 3 -- Ben finishes the map

Ben opens the SMW chain sheet, works through the doubts list and the difference image, corrects the
railway by hand as the first test of the editor, saves, and the saved sheet replaces
map_graphics/xml/stalin-moves-west.xml. The chain stages of task 15 (river, border, places) run
before this with step 1 in force, so the editor's first session has the whole map to finish.

log:
- 2026-09-19 Ben: add interactive lattice pinning (drag, zoom, pin grid hexes to image hexes, choose
  candidates, mark folds) as an editor step, since maps are read one at a time and each party should do
  what it does best.
- 2026-09-17 Ben: the editor must cover every element, "the works": terrain, add/delete hexes, roads,
  rivers, cities and all the rest; scope rewritten accordingly.
- 2026-09-17 written by the coordinator at Ben's request. Ben's ruling: leave the railway in its
  imperfect state and move task 15 on to C-river; the railway is the editor's first test.


## Requirement added 2026-09-19 (Ben): supplementary high-detail sections

While numbering the D-Day at Tarawa lattice, the scan's hex ids were about five pixels tall and
unreadable, so Ben photographed one section ("Tarawa detail of Red Beach, the parrot's beak.jpg")
and named a unique symbol in it (the yellow F2 hexagon). From that one section the whole sheet was
numbered: F2 = 1437, A1 = 2035, F10 = 1838, F5 = 1638 read off the photo, each found on the scan by
its symbol, and lattice.py --anchor derived id-format {row:02}{col:02}, rows 31 downward, columns from
1 eastward. Ben: "This is the kind of interaction an editor should support: providing little
high-detail sections to supplement the overall map." So the editor must let a person add a detail
image (photograph or rescan of a small region), place it on the sheet by naming a hex or a unique
symbol, and read from it -- ids, terrain, hexsides -- with the detail standing in for the scan where
it covers. The detail is never warped or merged into the scan; it addresses hexes, like everything
else (PLAN.md 2026-09-16, structure not pixels).

Copyright Ben Paul Wise. All Rights Reserved.
