Copyright Ben Paul Wise. All Rights Reserved.

# Stalin Moves West map: reading notes (by eye), 2026-09-25

Result: `map_graphics/xml/stalin-moves-west.xml`. It validates and renders to `stalin-moves-west.{svg,png}`.
The file was rewritten from scratch; the earlier tile and chain readings were not reused.
Method: `doc/map-reading-by-eye.md`. Source: `example maps/Stalin Moves West map.jpg`, 1650 x 2550 (a flatbed
scan, so the lattice holds to about 5 px from corner to corner).

## Lattice
- Pointy-top. Ids are `{row:02}{col:02}`: the row number comes first and runs 27 (top) down to 06; the
  columns run 28 to 45 from west to east. The printed id sits on the west side of the hex, rotated.
- Rows 26, 24, ... (index 1, 3, ...) are shifted half a hex east: `offset="odd"` with `row-step="-1"`.
- Measured from the grid-line runs: the column pitch is 92.0 px and the row pitch 80.2 px. The size is
  53.3, the mean of the two estimates (they differ by 0.7%). The centre of 2728 is at ox 19.5, oy 508.5.
- Neighbours. In an unshifted (odd) row: ne (c, r+1), se (c, r-1), nw (c-1, r+1), sw (c-1, r-1). In a
  shifted (even) row: ne (c+1, r+1), se (c+1, r-1), nw (c, r+1), sw (c, r-1).
- Outline: 319 hexes; the `clip` attribute lists what is cut from the 18 x 22 rectangle. The charts
  cover the north-west, and the holding boxes and tracks cover the south-west. Column 45 exists only in
  odd rows. Row 07 starts at 0738 and row 06 at 0638.

## How each layer was read
- Terrain fills are solid hex fills on this map. The fraction of rough, forest, sea and marsh pixels
  was measured per hex, and the classes were then confirmed by eye: 39 rough, 5 forest, 13 marsh and
  6 All-Sea hexes.
- Coastal is terrain-plus: the TEC says OTIH (other terrain in hex) and "see naval movement". So it is
  recorded as a region (`layer="coast"`, 22 hexes) over the land terrain, not as a terrain of its own.
- Rivers: each hexside was scored by how much river-blue ink lies along it, and the candidates were
  checked tile by tile. The eye removed coastline strokes, lagoon shores and the false hits from the
  marsh's blue pattern. The Pripyat through the marsh was traced by eye. Lake Balaton is on 0933:sw
  and 0933:se (line `lake`; the TEC's River/Lake Hexside).
- Borders: red-ink hexsides were split into segments at their junctions. Each segment's kind was then
  set by eye: the thick dashes are the Front Line (USSR border), the dash-dot line is a national border.
- Nations: six regions, filled out from the printed country names with the border and front-line
  hexsides as walls. Every land hex falls in exactly one region (Greater Germany 88, General Government
  21, Slovakia 11, Hungary 36, Romania 28, USSR 129).
- Rails: traced by eye as polylines on labelled 1.65x and 2.2x crops, converted to hex chains by the
  lattice, then checked for adjacency and drawn back over the scan. A link ends at a place, a junction,
  or the map edge. Junctions are explicit: a junction is added wherever two links share a hex.
- Bridges: the yellow bars were found by colour and snapped to the nearest hexside. All 23 fall on an
  accepted river hexside that a rail crosses.
- Places: 24 cities plus Ploesti (a resource, no city blocks). Soviet stars are at Riga, Minsk and
  Odessa. Axis stars are at Berlin, Konigsberg, Vienna, Budapest and Bucharest. Ports are Riga, Danzig,
  Konigsberg, Stettin and Odessa. Resources are Krakow and Ploesti.

## Doubts for Ben (not resolved by guessing)
1. The Smolensk - Kiev rail runs outside the grid for one step: east of 1844, where even rows have no
   column 45. It is recorded as 1945 1844 1745.
2. The Debrecen - north rail runs along the 1337/1338 boundary; 1337 was taken.
3. The Prague - Breslau rail near Breslau: 1433 1534 1633 was taken (the line runs along the 1533/1534
   boundary).
4. The Riga - Kaunus rail loops west close to the coastal hex 2536; it is recorded as 2537 -> 2436.
5. The Pripyat's western end is taken to start on 1840:se; 1840:sw carries faint blue too.
6. The front line runs on top of the Dniester and Prut hexsides (1042/1043, 0943, 0843 ...). Both the
   river and the front line are recorded on those hexsides.
7. 1437:se (between the General Government, Slovakia and the front line at 1338) is recorded as a
   national border.
8. 0745 is recorded as All-Sea although a sliver of land (about 8%) shows. Without that choice the USSR
   region would leak round the end of the front line into Romania.
9. 2739 is forest and coastal. 2336 is coastal (77% water, the Curonian spit).
10. Lagoons (Stettin, Vistula, Curonian) and coastlines are not river hexsides. The river mouths that
    are recorded: 2030:e and 2030:se (Oder), 2234:se (Vistula), 2336:se (Neman).
11. The Riga star sits on the 2638/2639 hexside; it is placed in 2638.
12. The Konigsberg blocks straddle 2235/2236; 2235 was taken because the port and the star are there.
13. The Dvinsk name is half hidden by the rail ("...insk"); the name is assumed.
14. Rails leaving the map from a hex (2345 east, 1745 east, 1145 east) are single-hex links with
    `ends="... edge"`. They validate, but a language question follows: should an exit carry its direction?
15. The printed misspelling "General Goverment" is kept in the label; the region's name is corrected.
16. The two red styles (dash-dot and dashes) map to two lines, `border` and `front`. The rules treat
    them differently (TEC: National Border vs Front Line (USSR border)).
17. Rough-hex rivers along the Danube (1129-1135:se/sw) are hexsides; the Danube is drawn straight
    across the hex row in print.
18. Charts, tracks and holding boxes are not read (per the 2026-09-21 ruling).

Copyright Ben Paul Wise. All Rights Reserved.
