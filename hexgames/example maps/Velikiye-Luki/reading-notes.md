Copyright Ben Paul Wise. All Rights Reserved.

# Velikiye Luki map: reading notes (by eye, one-off, no reader tools), 2026-09-22

Result: `map_graphics/xml/velikiye-luki.xml` (validates; 118 hexsides, 24 chains, 27 places).
Method: `doc/map-reading-by-eye.md`. These notes record what was settled, how, and what is still
for Ben with the paper map. The working notes written during the first pass are superseded by the XML.

## Sources
- `Velikiye Luki map complete.png` 5182 x 8021: the whole sheet, a photograph. All XML coordinates are
  in its pixels. It is not affine: the row pitch grows from 448 px at the top to 467 at the bottom and
  the column pitch (390) is 1.7% short of the row pitch's hexagon (397). The XML uses the mean
  (size 262.3, ox 1013, oy 3389), so a render overlays the photo to within a tenth of a hex.
- `... map north scanned.jpg`, `... map south scanned.jpg` 5100 x 6600, rotated 90 degrees: sharper;
  used only to settle doubts (Novoskolniki stream, Vlase, 0107/0206/0306 hills, 0302/0402/0502 woods).
- `... TEC scanned.jpg`: terrain key with the eleven terrain classes and the objective marks.

## Lattice
- Flat-topped. Nine numbered columns 01..09 (index 0..8), ids `{col:02}{row:02}`; rows 00..09 in the
  odd columns, 00..08 in the even ones, which sit half a hex lower (offset="odd" on the zero-based
  index). Ids sit inside the hex at the bottom (id-side s).
- Ten grey German entry hexes west of column 01 and ten pink Russian entry hexes east of column 09,
  both shifted like the even columns: separate grids `G00..G09`, `R00..R09`.
- Measured: horizontal grid lines along the column-03 centre line at y = 3140, 3588, 4033, 4482,
  4934, 5396, 5858, 6322, 6789, 7255, 7722 (row pitch mean 458.2); diagonal-edge crossings at row-4
  height put column centres at 1817 (03), 3377 (07), 4158 (09) (column pitch 390.2).

## Neighbours (flat-top, even columns shifted down)
- odd column c, row r: n (c,r-1) s (c,r+1) ne (c+1,r-1) se (c+1,r) nw (c-1,r-1) sw (c-1,r)
- even column: ne (c+1,r) se (c+1,r+1) nw (c-1,r) sw (c-1,r+1). The G and R strips count as even.
- A road or rail leaving a hex exactly at a corner is ambiguous between two hexsides; the XML records
  the reading and the alternative (Novoskolniki rail to G05 not G04; Poreche's two roads both leave
  the map through 0509).

## Settled by the scans
- 0105:s carries the stream from mid-edge to the south-west corner (kept as a minor-river hexside).
- No stream on 0106:ne or 0306:nw (dropped). The Cveretitsa rises inside 0306.
- Hills: 0103, 0107, 0206, 0306, 0403, 0606, 0909 (0206 about half; 0107 about two thirds).
- The Lovat leaves the map along 0408:s and 0309:se (the sheet's bottom edge); enters along 0700:nw.
- Prohibited hexsides: 0708:nw, 0708:sw, 0708:s (TEC 11; the key's sample hex is 0708 itself).
- Bridges (road or rail crossing the Lovat): 0604:nw, 0604:sw, 0508:sw.

## Ben's rulings from the paper map (2026-09-23), applied
1. 0302 is wooded, minor river on SW, S and SE; Tulubevo village kept. 0103 is both wooded and hill:
   the sheet gives it (and 0107, 0206, 0306, 0606, 0909) the compound terrain `hill-wooded`; 0403 is `hill` over open.
2. The Gubani road runs 0702-0602-0703: Kartsevo (0602) has road access.
3. Dokukino: 0805-0906 eastward link and 0805-0905 link to Kunya. The four roads into the Russian strip are
   0902-R01, 0905-R04, 0906-R05 and 0909-R09; the rail enters at 0905-R04 too (R00 at the north end).
4. 0902:s and 0902:se are both minor-river hexsides.
5. 0606 N NE SE; 0707 NW SW NE SE; 0708 lake-edges NW SW S, river NE SE; 0607 river NE S SW, lake-edge SE,
   Lovat NW: added 0607:s and 0607:sw; the rest were already present.
6. The Novoskolniki rail leaves 0105 into the fifth entry hex from the north (G05 in this file's numbering).
7. Shubino 0203: five minor-river sides; NE dropped.
Numbering of the entry strips: this file counts G00/R00 at the NORTH end, like the printed rows 00; Ben
counted from the south when answering, so his G04/R05/R04 are this file's G05/R04/R05.

## Still by eye (no ruling asked)
- 0206 hill (about half), 0502 open (wooded about 55%).

## Hill over wooded (Ben's question 1)
The TEC gives Hill "other terrain in hex" for movement and -1L for combat, and Wooded 1.5/3 MP and -1L;
nothing in the TEC, the rules summary or the designer's notes (VLDNotes.pdf) says how the two combine,
so a hill-and-wooded hex costs wooded movement and presumably one -1L, not two (the CRT shift table
never stacks two terrain shifts, but the rules do not say). Recorded as a rules question, not a map one.

## Network geometry (Ben's note 5)
A `link` is a chain of hexes, so a shared hex is a shared node. Not good enough: 0705 has two roads that
do not meet, 0805 has three separate junctions, 0403 two arcs that never meet, and The Russian Campaign
has two hexes beside Leningrad with parallel roads. Ben's node-and-arc scheme (each hex holding an
unordered set of the intersections in it) goes to PLAN.md "XSD proposals awaiting review".

## Junctions (Ben, paper map, 2026-09-23; sheet is junctions="explicit")
Connected intersections drawn inside one hex are one junction (briefing slide 14). Roads: 0604 six
(two to 0605), 0605 three links (five arcs), 0105 three, 0805 four links (five arcs: 0705 twice, 0906,
0706, 0905; there is NO 0804-0805 road), 0705 one junction of the roads to 0604, 0804 and 0805 while the
0605-0705-0805 road crosses without meeting, 0203, 0303, 0504, 0508, 0706, 0802, 0800, 0701 two each.
0403: two roads cross, no junction. Rail: 0105 three lines, 0604 four, 0505 two; the two eastern lines
cross 0804 (north 0704-0804-0904, south 0705-0804-0905) without meeting.

## Counts
Open 40, wooded 39, hill 1, hill over wooded 6 (86 numbered hexes: 5 x 10 + 4 x 9), plus 20 entry hexes.
Lovat 22 hexsides; minor rivers 94; prohibited 3; rail 6 chains; road 17 chains; 17 junctions (14 road, 3 rail).

Copyright Ben Paul Wise. All Rights Reserved.
