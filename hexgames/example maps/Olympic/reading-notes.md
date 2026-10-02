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

## 2026-09-29 re-read from the three flat photos (tasks/18-olympic-photos.md)
Sources: `example maps/Olympic left|middle|right.jpg` (EXIF-rotated, read at quarter scale). Each photo was
registered to the sheet's ideal grid by a homography (landmarks, refined on the printed grid lines); the
drawn ids sit on the printed ids. Reading by Sonnet workers, one sub-task each; outputs in
`example maps/Olympic/work/out/`; merged by the coordinator.
- Water (Ben: only two water colours; darker = shadow): 54 pale hexes; the 15 holding a red letter are
  deployment hexes (ruling 3), so 39 assault hexes. Added 0917 1705 5411; removed 0706 3010 4505 5206
  (sea) and 4605 (half land, now clear).
- Roads: three column strips, merged by `work/merge_roads.py` (a step belongs to the strip that owns its
  mean column; ten steps only a neighbouring strip read were confirmed on crops and kept). 746 steps,
  310 links, 3 pieces: the mainland (623 hexes, every place reached), the island at cols 29-34 (24) and the
  ring on the island at 5807/5907/5908 (3). The draft roads were replaced. Unexplained ends: 1510 1918
  2720 3021 3932 4319 5619 and the island ring. Triangles (two readings of one road, or a Y near a vertex):
  0408/0409/0509, 3632/3732/3733, 3805/3905/3906, 3915/3916/4015, 4103/4104/4203. About 60 per-road doubts
  in the three roads-*.txt files.
- Deployment letters: 82 (39 C, 28 L, 6 2L, 5 36L, 3 36C, 1 R); the sheet's text glyphs were rewritten
  from them. 1705 has none. 5624 R is black on the print. Doubt: 1003 L near the 1103/1104 side.
- All-mountain hexsides: 68 of the 73 measured candidates rejected; 5 kept for Ben (still magenta):
  4518:ne, 4518:se (clearest), 4020:se, 4124:n, 4223:ne.
- Places (Ben, 2026-09-29): 4705 Saesbo (was Sonogi), 3532 Saeki added, printed spellings Yokamaechi
  (2214) and Tsyuzaki (5517).
- Ben's rulings (2026-09-29, from the physical map), applied: Tokashi removed, Fukushima town at 1017;
  Kobayashi town at 2016 (three roads meet there; 2116 carries only 2016-2116-2215, so the middle strip's
  2214-2115-2116 was removed in merge_roads.py); Kushkino (printed spelling) at 1706 (three roads); city
  hexes Kumamoto 3616 3717, Yawata 5520 5620 5621, Kokura 5521 5522 5523 5622; both dashed lines run
  along hexsides and extend into the sea (Objective Line 2403-2429, Boundary Line 3112-3134).
- (superseded) Open for Ben: Tokashi 0918 (no dot seen; Fukushima's dot sits on the 1017/1118 side, its L in 1017);
  Kobayashi (dot on the 2016/2116/2117 vertex, L in 2016; sheet 2116); Kushikino (sheet 1706, dot seen in
  1707); Kumamoto buildings also in 3717; Kokura/Yawata city hexes (buildings in 5520, 5523, 5622).
- All-mountain hexsides, second pass (Ben: shading along a hexside counts even when both hexes are mostly
  not mountain): 13 confirmed by Ben (line mountain-side, brown); 65 candidates (line mountain-side-check,
  magenta) from pass E2's yes list, minus 24 edges that only outlined whole dark hexes. Those dark hexes were
  missed ALL-MOUNTAIN HEXES, now terrain mountain: 2318 3123 3124 4625 4822 4823 (the terrain was measured on
  the wrinkled photo; more may be missed). Review images: work/crops/review_mountain_{left,middle,right}.jpg
  (cyan = confirmed, yellow = candidate). Ben's "4118/4219" is not adjacent; E2 says 4119:se matches.
- Ben, 2026-09-29: accept every candidate (he edits the rest in HexMapEd) and add 4811/4911 (4811:ne). The
  sheet now has 79 all-mountain hexsides, one line style (mountain-side), drawn solid magenta so they stand
  out while he edits.
- Dashed boundary lines, as `<path>` hexside chains (kind objective / boundary; no XSD change): American
  Objective Line 45 hexsides, the ne/se sides of column 24, 2405 to 2427 (coast north of Izumi to the Tsuno
  sea); North-South Boundary Line 42 hexsides, the ne/se sides of column 31, 3113 to 3133 (inlet north of
  Yatsushiro to the sea south of Saeki). Both print as straight lines midway between two columns, so the
  zig-zag is the nearest hexside chain. Doubts: the exact north start of each; Yatsushiro's dot is within a
  fifth of a hex west of the red line.

Copyright Ben Paul Wise. All Rights Reserved.
