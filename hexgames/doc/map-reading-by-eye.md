Copyright Ben Paul Wise. All Rights Reserved.

# Reading a hex map by eye into hexsheet XML: method notes

Written while reading the Velikiye Luki sheet (2026-09-22) as a one-off, with no detector code:
the model looks at crops of the scan and writes the XML by hand. The notes are for a model (Sonnet,
Opus) that must do the same for a large sheet such as Dai Senso. `hexsheet.xsd` is the target and
`tools/validate-xml.py` the only gate; `hexsheet2svg.py` renders the result for comparison.

## 1. Invariants to lean on (they remove most of the guesswork)

1. Every hex is a perfect regular hexagon and every hex grid is a perfect lattice. Nothing on the
   sheet is "slightly rotated" or "a bit bigger here": if it looks so, the scan is warped or the
   crop is misread. A photographed sheet drifts (Velikiye Luki: row pitch 448 px at the top, 467 at
   the bottom); a flatbed scan does not.
2. A grid is flat-top or pointy-top, never anything else. Decide it once from any hex: flat-top
   hexes have a horizontal top edge and columns run straight up and down; pointy-top hexes have a
   vertex at the top and rows run straight left to right.
3. Numbering is regular. Read a handful of clear labels, then COUNT: the other ids follow from the
   lattice. Column index is (x - ox) / (1.5 size) for flat, row index is (y - oy) / (1.5 size) for
   pointy; the other axis has pitch sqrt(3) size with a half-pitch shift on alternate columns (flat)
   or rows (pointy). Decide which parity is shifted from one pair of adjacent columns/rows.
4. The outline may be irregular (short columns, entry strips, cut corners) but there is never a
   hole: any cell surrounded by cells is a cell. Unnumbered cells (entry hexes, holding boxes drawn
   as hexes) are still cells of the lattice; give them their own grid with its own id-format.
5. Every hexside belongs to exactly two hexes; every feature drawn along a hexside is one `edge` no
   matter which of the two hexes you saw it from. Name it from ONE of them consistently (I use the
   hex whose id is lower in reading order) so the list can be checked for duplicates.
6. Line features are chains: a river follows hexsides end to end; a road or rail passes through hex
   centres (hexside midpoint to midpoint). A road that "bends inside a hex" is still the chain of
   hexes it visits. A river that "cuts through a hex" (meanders across the interior) is a drawing
   liberty: it must be assigned to the hexsides the rules would use, and that assignment is a doubt
   to record, not to hide.

## 2. Procedure that worked

1. Overview first (whole image scaled to ~1400 px): identify the sheet's kind, orientation, the
   numbering scheme, the terrain key, and where the map area ends and furniture begins. Write it down.
2. Fix the lattice numerically, not by eye: sample one vertical line through a column of centres and
   one horizontal line through a row of centres, list the dark (grid-line) pixel runs, and read the
   pitch from the run positions. Ten hexes give the pitch to a quarter percent. Derive size =
   pitch / sqrt(3) (the pitch across the hexes' flat sides) and ox, oy from the position of one
   hex whose id is legible. Check with a second hex far away.
3. Cut 1:1 crops of about two columns by two rows (1000 x 1250 px is a good size for the viewer),
   with overlap, named by column band and row band, and read them in a fixed order (west to east,
   north to south). Big crops lose detail; small ones lose context. Six to nine crops per request is
   the practical limit of the viewer; more are silently dropped.
4. For each hex, in the order of the crops, record: terrain (one value; the dominant fill), place
   (village, town) and its objective marks, then each hexside feature and each through-feature with
   the neighbour it goes to. Write notes as you go, in a file, not in your head: a session can end.
5. Where the reading is ambiguous, make a 2x zoom crop with a pixel ruler drawn on its margins
   (every 50 px, labelled every 100) and look again. The ruler turns "the river is near the corner"
   into "the river crosses x = 2312", which the lattice converts to a hexside.
6. Only then write the XML: grid, palette, terrains, lines, legend; `<hexes>` by terrain; the
   `<edge>` lists per line kind, sorted; `<link>` chains per network with `ends`; places and labels.
   Validate. Render. Compare the render with the scan at the same scale, band by band.
7. Keep a doubts list in the notes and in XML comments. The reviewer with the paper map resolves
   them; the model must not resolve them by guessing.

## 3. Mistakes I made, so the next reader need not

- I wrote notes for crops the viewer had dropped ("media removed"), from memory of the overview.
  They were wrong in detail. Rule: never write a reading for an image you did not see in THIS
  request; if the result says the media was removed, ask again with fewer images.
- I derived neighbours with the wrong parity rule several times (treating an odd column as shifted).
  Rule: write the two neighbour tables (shifted, unshifted) at the top of the notes and copy from
  them; then run a script that checks every `link` for adjacency and every `edge` for duplicates
  before rendering. It caught a 0706-0807 step that is not adjacent.
- I placed hex centres from a formula with the wrong row base for one column (0706 at row 7's y).
  Rule: compute centres in a script, never in prose; print a table of centres once and read from it.
- The first render looked as if it drew lines across the whole sheet. It did not: at quarter scale
  thin dotted rails and straight diagonal roads looked like one line. Rule: judge a render at half
  scale in quadrants, never at a scale where a hex is 60 px.
- Two of my "streams along a hexside" were on hexsides I had named twice from the two hexes that
  share them (0503:nw and 0402:se). The duplicate check finds these.
- A stream that meets a river along a hexside the river already occupies (0407:se on the Lovat) is
  not a second edge.

## 4. Pitfalls met on this sheet

- The viewer drops images when a request carries too many pixels: ask for at most six images per
  request, JPEG, ~1000 px on the long side. A dropped image is reported as "media removed".
- Photographed sheets are not affine: the same lattice cannot fit the top and the bottom to the
  pixel. Take the mean pitch for the XML (the renderer's output is the reference, not the photo) and
  expect the overlay to wander by up to a tenth of a hex in the middle.
- Rivers are drawn twice: as a thick line along hexsides (the rule-bearing feature) and as thin
  meanders across hex interiors (decoration). Only the hexside line goes into `<edge>`; a stream that
  runs through an interior and reaches a hexside further on is a doubt.
- Woods and hill fills straddle hexsides freely. The rule is "dominant fill in the hex"; a hex whose
  fill is half and half is a doubt to list for the reviewer, with the fraction seen.
- Lakes drawn as blobs are terrain of the hex (the TEC says "other terrain in hex"): record them as
  a glyph or a terrain flag according to the game's rules, not as edges.
- A "prohibited hexside" or "bridge hexside" is an edge with a symbol/mark; put the mark in the
  legend block once and reference it.
- The complete image and the two half scans disagree in detail: the halves are sharper, the
  complete image is the one whose pixels the XML is measured in. Use the halves only to settle a
  doubt, then convert the answer to the complete image's hexes, never its pixels.

Copyright Ben Paul Wise. All Rights Reserved.
