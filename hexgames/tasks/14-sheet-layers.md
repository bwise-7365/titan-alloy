Copyright Ben Paul Wise. All Rights Reserved.

# Task 14: separate structure, style and layout in the map and counter languages (milestone M6j)

status: pending
worker: unassigned          started: -
resume: not started, and NOT to be started until Ben approves the XSD proposals this task must write first.
inputs:
  map_graphics/xml/hexsheet.xsd, unit_graphics/xml/hexcounters.xsd
  map_graphics/xml/hexsheet2svg.py, unit_graphics/xml/counters2svg.py   (the reference renderers)
  hexxml/SheetDoc.{h,cpp}, hexmodel/BoardBuilder.cpp                    (what the engine actually reads)
  packages/xml/hexpackage.xsd                                           (how documents are bound today)
  the eight sheets: map_graphics/xml/*.xml and the four C++ fixture sheets
  hexgames/PLAN.md decision log 2026-09-16 ("structure, not pixels"; the symbol shape/meaning proposal)
outputs:
  XSD proposals in PLAN.md "XSD proposals awaiting review" (REVIEW GATE: Ben decides before any code)
  then, on approval: the schemas, the reference renderers, the sheets, SheetDoc, the hexview contracts
acceptance:
  Ben's test: CHANGING A RIVER'S WIDTH TOUCHES ONLY A STYLE FILE -- not layout, and never structure.
  The engine loads a document that carries structure alone, with no style and no layout present.
  All eight sheets migrate; SVG and PNG regenerate; ctest green; no engine golden changes.

## Why (Ben, 2026-09-16)

The map language mixes three kinds of semantic content in one document, with no boundary between them,
so an image-driven workflow can push rendering data into what should be structure. It already has:
- `grid/@size @ox @oy` are REQUIRED and `assemble.py` fills them from the IMAGE CALIBRATION, so every
  sheet embeds the pixel frame of one particular scan. Rescan at another resolution and the "correct"
  values change, though nothing about the map has.
- `label/@x @y` are absolute pixels, and the marker pass writes them straight from source-image
  coordinates: literal pixel-copying, where a label almost always has a real anchor (a hex, a path).
The counters language is cleaner -- normalized 100 x 100 face units, named slots, and a back defined by
RULE (`derived="reduced|concealed|same"`) rather than as a second picture -- and is the precedent to
follow, not to rewrite.

## The three layers

STRUCTURE -- the ideal design; the only layer the engine reads.
  Map: the lattice (orientation, offset, cols, rows, id-format, col/row-start and step, clip, id-side,
  default terrain); terrain per hex; hex names; which hexsides carry which kind of line; which hex chains
  carry which link kind; regions as hex sets with names; labels anchored to a hex slot or a path; the
  MEANING of each mark ("this hexside is a bridge", "this hex is a Soviet mobilization hex"); panel
  CONTENT (a track's ordered cells, a table's rows).
  Counters: family, icon, modifiers, echelon, values, steps, emblem meaning, the back rule, sheet order.
STYLE -- how a kind is drawn, and nothing about where anything is.
  Palette; terrain fills and patterns; line width, dash, casing, opacity, ticks; the SHAPE of each mark
  (the shape/meaning split already proposed); the urban policy; fonts; label size, weight, halo,
  spacing; counter style colours; corner radius; the scale knobs.
LAYOUT -- where things sit on a rendered or printed page.
  Sheet width and height; the grid's rendering unit and origin (today's size, ox, oy); panel, box, track
  and table geometry (x, y, w, h, rotate, cell sizes); free x/y placements; counter-sheet cols, rows,
  gutter, margin, crop marks, mirror.

## Mechanism: A, CHOSEN BY BEN 2026-09-16. B and C are kept only as the record of what was weighed.

A. Three documents -- map (structure), style, layout -- bound by hexpackage.xsd or by `@style`/`@layout`
   on the sheet root. Meets Ben's test directly: a river-width change is one edit in one style file, and
   several maps can share a style. Why it beat B: several sheets SHARE one style (TRC and PGG are both
   sparse operational maps, so one edit restyles both); the same map RENDERS through a different style
   (screen against print, high-contrast, Qt6 against SVG) with no edit to the map; the map schema has no
   concept of a stroke width, so no tool can write one into a map file even by accident -- it would fail
   validation; the map's git log carries structural change only; and "the engine reads structure alone"
   becomes true by construction rather than by the loader's good behaviour.
   Costs accepted with it: a binding mechanism and resolution rules (a missing style id, two styles
   defining the same id); more files per sheet; assemble.py, validate-xml.py, both renderers, SheetDoc
   and the package language all learn about it; and the fixtures need a minimal default style so a bare
   map still renders. On fixtures, consider the HTML compromise: a document MAY carry an inline style,
   external preferred, inline reserved for the four C++ fixture sheets and one-offs.
B. One document, three strict top-level sections, engine reads only the first. Cheaper migration, weaker
   separation: nothing stops a later tool writing pixels into structure.
C. Demotion only: make the layout attributes optional with renderer defaults and forbid appearance on
   structural elements. The cheapest step, and a sound first move toward A even if A is chosen.

## Specific defects this task must fix

1. `grid/@size @ox @oy` -- out of structure; the renderer picks a unit (a nominal circumradius) and an
   origin. A sheet must never inherit a scan's pixel frame.
2. `sheet/@width @height` -- layout, derived from the grid and the margins when absent.
3. `label/@x @y` -- structure labels anchor at a hex slot or along a path; free coordinates belong to
   panel content only. `assemble.py` and the marker pass change with it.
4. `path/@offset` ("shifts the stroke into the named hex so two chains can share a hexside") -- a
   rendering workaround; it belongs in style.
5. Panels: separate the content (cells, rows, text) from the geometry.
6. The `Symbol` enumeration conflating shape with meaning -- the proposal already written up; this task
   should land it, or follow it.
7. Counters: `Tile`'s fill encodes the year on TRC backs, so meaning is hidden in a colour and a
   recolour would lose it -- make the meaning explicit. Also review `symbol/@partial`, the `scale`
   knobs and `silhouette/@href`.

## First step, before any proposal

Audit what the C++ side actually consumes: if `HexXml::SheetDoc` and `HexModel::BoardBuilder` read the
pixel geometry, the leak reaches the model and the split has to reach it too; if they ignore it, the
damage is confined to the documents and the renderers. This was NOT checked on 2026-09-16.

## Open questions for Ben

1. Mechanism A, B or C.
2. Is a style document per sheet, or shared by every sheet of a game (or across games)?
3. Do the counters get the same split now, or later, given that language is already close?
4. Does hexpackage.xsd bind the layers, or does the sheet root name its style and layout?

## Sequencing

After M6i (the SMW process test) and after the shape/meaning proposal is decided. Not during a map run:
changing the geometry layer under a running worker would invalidate its tiles and candidates.

log:
- 2026-09-16 written by the coordinator at Ben's request; not started

Copyright Ben Paul Wise. All Rights Reserved.
