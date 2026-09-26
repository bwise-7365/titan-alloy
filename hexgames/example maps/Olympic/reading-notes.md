Copyright Ben Paul Wise. All Rights Reserved.

# Operation Olympic map: reading notes, 2026-09-25

Result: `map_graphics/xml/operation-olympic.xml` (validates; renders to `operation-olympic.{svg,png}`).
Method: `doc/map-reading-by-eye.md`, leaning on the invariants because the sheet is folded and wrinkled.

## Sources
- `Olympic map wrinkled.png` (2396 x 1568): the sheet's pixel space and the source of every measurement.
- `Olympic map wrinkled and marked.png`: the three ovals (red 1106, green 1613, yellow 3211).
- `olympic full map, tilted.jpg`: flat but in perspective; registered to the wrinkled photo (homography
  plus about 700 patch correlations, median residual 17 px) and used to check places and the south coast.
- `olympic detail.jpg`: the south-west, flat and sharp; used by eye.

## Lattice (the invariants did the work)
- Flat-topped, ids `{col:02}{row:02}`, columns 01-61, rows 00-34. The EVEN columns sit half a hex lower
  (measured from the cell positions; this matters for every neighbour and hexside).
- Cells come from the earlier lattice run (perfect lattice, grown locally over the wrinkles), numbered by
  counting from the ovals. All three ovals land on their ids. Each cell was then snapped to the printed
  grid lines by sliding a perfect hexagon of the known size over the photo (median shift 2 px).
- No-holes: five cells the lattice run missed (1217, 1417, 3226, 4705, 5532) were added at their
  neighbours' centroid.
- The sheet XML uses one ideal grid (size 26.08, fitted to all cells; rms 8 px against the photo, the
  wrinkles). Cells under the printed panels (tracks, TEC, CRT) are clipped: they are not in play.

## Layers
- Terrain: colour fractions per cell. Sea when 85% or more water; otherwise Ben's rule below (rough over
  25%, mountain at 25%).
- Assault hexes (TEC: pale blue sea hexes): a sea cell markedly paler than the sea around it on the same
  fold panel (the photo's lighting differs panel to panel). 49 found; the title art over the north-west sea
  is excluded.
- Rivers: thin blue water along each hexside (the river mask minus large water). 392 hexsides.
- All-mountain hexsides (TEC: entry prohibited): the darkest brown continuous across the whole side, on both
  sides of the line: 73. All-rough hexsides (TEC: +2 MP): tan continuous across the side: 657.
- Places: 45 hexes read by eye on labelled tiles (towns as dots, cities as blocks); the Japanese deployment
  letters (C, L, 2L, CL, 36C, 36L, R) as text glyphs, 78 hexes.

## Not read yet
- Roads (TEC: road hexside negates rough or river hexside cost). The automatic crossing test was noise; they
  need tracing by eye, like the TRC rails.
- The American Objective Line and the North-South Boundary Line (dashed, running north-south).

## Ben's rulings (2026-09-25), applied
1. First-pass thresholds are acceptable; the aim is to represent the map, not replicate it exactly.
2. Terrain: more than 25% rough makes a hex rough, unless at least 25% is mountain, which makes it
   mountain (fractions of the land in the cell). Now clear 455, rough 511, mountain 55. May be revisited.
3. A pale hex holding a red letter is a Japanese Deployment hex, not an allied assault hex: 0906, 1705, 5311,
   5411, 5412 removed from the assault hexes (44 remain).
4. There are no north-west islands: the "islands" were the blurry photo under the title. Those cells are sea;
   gaps the lattice left there are filled as sea (no-holes: any group of missing cells that reaches neither
   the sheet edge nor a panel).

5. The 657 measured all-rough hexsides were removed (Ben: no such features on the map; they drew as pale
   grid edges). If any turn up on the physical map, Ben will name them.

OPEN ISSUE (BUGS.txt): the 73 measured all-mountain hexsides are unverified. They are drawn in magenta until
Ben checks the physical map (Monday); the TEC does define the type and a few may be real.
ROADS (2026-09-26, DRAFT): traced by eye on "olympic full map, tilted.jpg" (registered to the wrinkled photo),
25 tiles, every hex a road enters: 94 roads + 14 one-hex links closing seams between tiles (marked "junction
junction"); 5 pieces remain. 39 of 45 places reached (missed: Kitano 0715, Tokashi 0918, Tsuno 2325, Sonogi 4705,
Fukuoka 5115, Yawata 5621). Quality: the pattern is right (roads radiate from the right towns), but individual
hexes are often a column off, some straight runs are inferred, and counters hide the coastal roads in the tilted
photo. A first attempt from the wrinkled photo alone was discarded. Road ends not at a place read "unexplained".
Still open: finishing roads against the physical map; the two dashed boundary lines.

Copyright Ben Paul Wise. All Rights Reserved.
