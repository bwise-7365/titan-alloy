Copyright Ben Paul Wise. All Rights Reserved.

# Reading a hex wargame map: the primitives, how each is printed, measured and checked

A revision of `map_graphics/graphic-element-taxonomy.md` from a survey of the eighteen maps in
`example maps/` (Ben's `notes.txt` beside them), written 2026-09-18 for Ben's review before any tool is
built. The first taxonomy asked what a library must hold to draw these sheets. This one asks how each
printed thing is found, so that reading a map is a fixed list of small measurements at known places,
and a model or a person is consulted only about the residue.

The survey read, for each map, one reduced whole view, the legend panel where one exists, and one dense
corner at print resolution: 55 looks in all, no agents, no code.

## 1. The one fact that makes reading tractable

On every map in the set the playing surface is a regular lattice of identical hexagons. Everything the
rules read is attached to that lattice at one of a small number of address kinds, and the lattice's
position is known to a pixel or two once it is fitted. So the places where a feature can be are
ENUMERATED before the image is opened, and reading is verification of a finite hypothesis list, not
detection in a free picture. Comancheria, a point-to-point map, is the contrast: nothing there can be
enumerated in advance.

The corollary is the working rule for every tool: cut a small patch or strip at a known cell, in a known
orientation, normalise it, score it against a template, and never look at anything larger than a cell
except to find the lattice and read the legend.

## 2. The address kinds a printed feature can occupy

Seven kinds carry everything seen on the eighteen maps. Five are cells of the lattice; two are derived.

| kind | what sits there | seen on |
|---|---|---|
| hex area | terrain, tint, letter code, out-of-play | all |
| hex outline (ring) | fortress, strategic or objective hex, position badge, city outline | Tannenberg, DS, TK, Downfall, DDAT, TRC |
| hex centre and slots | city, town, port, capital, star, cross, derrick, fort, VP shield, letters, numbers | all |
| hexside, along | river, major river, coast line, border of several kinds, blocked, seawall, zone edge, ridge, front line, trench | all |
| hexside, across (a link step) | rail, double rail, road, track, off-map continuation | all but DDAT |
| hexside midpoint, oriented | bridge bar, ford, strait arrows, fire dot, redoubt, pier | SMW, Invasion, DS, DDAT, Borodino |
| vertex | corner-only grids; mountain corner triangles | Arctic Disaster |
| region (derived) | hex set with tint and/or outline: countries, defence lines, sea areas, patrol areas, approach zones, military districts | BFM, Downfall, TK, DS, Arctic, DDAT, TRC |
| label (derived) | place names beside a glyph, area names spaced across hexes, river names rotated along the line, the hex ids themselves | all |

The vertex kind is new. Arctic Disaster prints mountains only as small triangles where three hexes meet,
and Ben notes whole maps whose grid is drawn that way. It costs nothing to enumerate vertices as well.

## 3. How each primitive is printed, across houses and decades

### 3.1 Hex area

- Flat fill bounded by the hex: BFM redone, TK, DS, Downfall, SMW rough and marsh, Target Leningrad
  forest.
- Mottle or blob CLIPPED to hexes: BFM, Target Leningrad, Invasion forest, TRC forest, Tannenberg swamp.
- Blob that IGNORES hexes: PGG woods and lakes, Tallinn forest and marsh, Bagration forest, Tannenberg
  forest, Red Dragon rough. Here the rule is "the terrain that covers the hex", and Ben's PGG note says
  a hex with a quarter of green may still count as forest. The threshold is a labelled-example question,
  never a default.
- Land and water wedges with a free-form coast: nearly every map. The hex's terrain is a game decision
  (SMW's port cities are land; PGG 1006 is swamp by rule), so half hexes are the canonical doubt.
- Letters as terrain code inside the hex: Olympic prints C and L in coastal hexes; Tannenberg prints H
  and A as reinforcement keys. The letter is data, read at a slot.
- Corner marks only: Arctic Disaster mountains.
- Out-of-play hexes drawn in the grid: Downfall's Swiss hatch, Arctic's ice, Dai Senso's boxes.

### 3.2 Hex outline as a ring

A thick coloured hexagon slightly inside the grid line: Tannenberg fortresses (blue or orange), DS and
TK strategic hexes (green, red, orange), Downfall objectives (red), DDAT position badges (six colours
that are the game's activation key), TRC and DS city outlines (grey). The ring is read on the same strip
as the hexside line, inset a few pixels; its colour is data and comes from the palette.

### 3.3 Hex centre and slots

City as a cluster of black blocks (SMW, PGG, Tallinn, TK, Red Dragon, Invasion), as a circle (TK, DS),
as a square (BFM, Tannenberg town), as a dot with a name (Downfall, Bagration, DS town). Port as an anchor
(most), sometimes doubled (DS multi-zone port). Star, cross, derrick, shield with a number (Invasion VP),
a star-fort glyph (BFM redone), a Japanese flag (DS), setup crosses and stars filling whole hexes (BFM).
Placement is the centre or one of the slots the first taxonomy named; DS puts four things in one hex.

### 3.4 Hexside, along

- River: a thin blue line (Invasion, TK, Downfall with white casing), a wide blue band (PGG, SMW,
  Tallinn), a dark band with light dashes (Target Leningrad), a teal band (Borodino), a pale band with a
  dark edge for major rivers (Tannenberg, Bagration). Always ALONG the hexside with a wobble of a few
  pixels; corner-to-corner cuts through a hex exist on PGG and DS and are a doubt.
- Coast line distinct from the drawn shoreline: TRC's thick black line is rule-bearing; the shoreline
  is decoration (Ben's earlier note, confirmed in the corner crop at Königsberg).
- Border: dash-dot grey (Tannenberg), red dash-dot and dash-dash (SMW), striped band with an outline
  (Downfall red-white, TK orange-black, DS yellow-black, Invasion purple, green and orange bands), and
  SEVERAL BANDS ON ONE HEXSIDE where borders coincide (Downfall). Military districts and sea areas as
  dotted or dashed lines (TRC green, DS white and yellow, Downfall blue dots, Arctic grey dashes).
- Blocked hexside: a thick black hexside segment (Tannenberg, TRC). Seawall: a hatched bar (DDAT).
  Mountain or ridge hexside: brown band (DS). Front line: red dashed (SMW, Bagration).
- Trenches drawn across a hex rather than along its side (Tannenberg): a hex-area mark, not a hexside
  one, despite the name.

### 3.5 Hexside, across: the link step

The one invariant that survives every house style: a rail or road step between two hexes exists if and
only if the printed line crosses their shared hexside. Inside the hex the line is free: a curve through
the centre (Tallinn, Bagration, Red Dragon, BFM), a straight line cutting hexes off-centre (PGG rail,
Tannenberg rail), a line running along a hexside for a stretch (PGG, Bagration), a junction wherever the
artist put it. None of that is structure.

Styles: black with cross-ticks (SMW, Tallinn, Bagration, Target Leningrad, BFM redone), black-white
dashed (PGG), grey dashed (TRC), blue dashed for single track and blue solid for double (Tannenberg), thin
white (TK, Downfall, DS), a double black line (Red Dragon). Roads: grey (PGG), orange (Tallinn), red
(Bagration, in two weights), yellow (BFM redone), thick black (BFM), white (DS). Tracks: dashed grey
(Tallinn). Off-map continuations: a numbered circle on the line at the edge (Tannenberg), a boxed letter
with arrows (PGG), a line simply running off the edge (most).

Ends are explained on every map by one of: the map edge, a place, a junction, a printed terminus mark
(PGG's grey circle at a road cut by a river, Ben's note), a river bank with no bridge, or the line
joining another kind (SMW roads run with rails). The prior per kind is a piece count, never a rule.

### 3.6 Hexside midpoint, oriented

Bridge bars across the hexside (SMW yellow, Invasion dark brown), DS strait arrows, DDAT fire dots as
half discs on the inside of the hexside in six colours, Borodino redoubt triangles pointing into the hex,
DDAT pier hexsides. These are the first taxonomy's SideAddr and EdgeAddr, and the survey confirms both
exist: a bridge belongs to the hexside, a fire dot to one hex's side of it.

### 3.7 Regions

Every region on the set is either a tint over a hex set (BFM defence lines in grey, DS sea shades,
Downfall out-of-play) or an outline that follows hexsides round a hex set (countries on Downfall, TK,
DS; Arctic patrol areas in dashed grey; DDAT approach zones in five colours; TRC military districts).
The outline is the boundary of the set, so the set is the structure and the outline is derived. Where
several regions share a boundary, several bands run along one hexside; reading membership per hex
avoids ever having to separate the bands.

### 3.8 Labels and the hex ids

Place names sit beside their glyph, usually right or below; area names are large, spaced, sometimes
curved (Olympic's SENDAI, Tannenberg's RUSSIA); river names run along the river, rotated. Hex ids: four
digits at the hex's top (PGG, Tannenberg, Tallinn, Downfall, Bagration), at the bottom (Red Dragon,
DDAT), on the west side rotated (SMW), rotated ninety degrees on land (Arctic), tiny at the top-left
edge (DS, TK, TRC), only in corner hexes (BFM redone), only on badges (DDAT position hexes), none at all
(BFM, Borodino, whose columns are lettered along the edge). Every placement is a fixed offset from the
centre, so the id, when printed, is a patch like any other.

### 3.9 Furniture

Panels sit anywhere: outside the grid, over live hexes (DS unit boxes, Arctic airfield boxes, Downfall
tracks, Bagration's legend), rotated by ninety (BFM redone, Bagration, DS boxes) or one hundred and
eighty degrees (TRC's TEC, Red Dragon's turn track for the far player). Borodino's setup displays are
small maps that look like the map. Control measures extend past the map edge on PGG and Tannenberg.
Every panel is a rectangle, so panels are found as rectangles with no lattice signal inside them, and a
rectangle over live hexes is a clip.

## 4. The legend

Seventeen of eighteen maps print a legend or a terrain effects chart on the sheet. Its forms:

- A panel of hex-shaped swatches with a name under each: Tannenberg, DS, TK, PGG, SMW, Bagration,
  Olympic, Downfall, BFM redone, TRC. Terrain is a filled hex; a hexside kind is a hex with ONE EDGE
  marked; a link kind is a line drawn through a hex or a bare line sample; a ring is a coloured outline;
  a glyph is drawn in an otherwise empty hex.
- Rectangular swatches: DDAT's terrain table.
- Pictorial composites: Invasion of Russia's "bridge, fortress, river" in one picture.
- None: BFM, Red Dragon, Target Leningrad, Tallinn as scanned, Arctic. These assume the reader knows
  the standard symbology of the period, which means the process needs a small default style library
  keyed by house and decade, used when a swatch is missing and reported as an assumption.

Two things the legend gives beyond names. The swatches are templates: a strip cut from the river swatch
is the thing every hexside strip is correlated with. And the terrain effects chart on TK, DS and SMW is
printed in two sections, "hex terrain type" and "hexside terrain type", which is the grammar's own
distinction printed by the publisher. The legend is read once, by a person or by one model look, into
the vocabulary and the style declarations; nothing else in the process needs a model.

## 5. The measurement for each address kind

| kind | cut | scored against | output |
|---|---|---|---|
| hex area | inner hex patch, inset a quarter | colour and texture histogram of each terrain swatch, after clustering all hexes so that clusters are labelled, not hexes | terrain, with the share of the runner-up as the doubt score |
| ring | the strip just inside the outline, all six sides | ring colours from the palette | ring kind or none |
| centre and slots | patch at centre and at each slot | glyph swatches, and text presence | glyph list; a text patch goes to the label pass |
| hexside along | strip a few pixels wide, one hexside long, read along its length | each hexside-kind swatch's colour profile | one score per kind; several kinds may coexist |
| hexside across | the same strip read across its width | each link kind's crossing signature: dark or coloured run crossing the strip transversally | one score per link kind |
| midpoint | patch at the midpoint, oriented to the hexside | midpoint glyph swatches | glyph and which side of the hexside |
| vertex | small patch at each corner | corner-mark swatches | mark or none |
| region | hex centre tint with terrain removed, and the along-strips' band colours | region tints and band colours | membership per hex; outline derived |
| label | text blobs found anywhere | none: text is read by OCR or one contact sheet | text, anchored to the nearest hex or hexside |

Confusers and how they are told apart: the grid line itself is thin and present on every strip, so it is
part of the template's background; text is dark but not a continuous line across or along a strip; city
blocks fill the centre patch, not a strip; a coast is blue on one side only; the printed hex id is a
known patch and is masked before any other measurement.

## 6. Structure: priors that rank, and never invent

- Chains by hysteresis: strong hexsides seed, adjacent hexsides above a lower threshold join. A hexside
  with no evidence is never added, whatever the gap. The SMW river worker's script inserts are the
  failure this rule exists to forbid.
- Ends explained by a printed reason at the end, or listed. The reason vocabulary per kind is in section
  3.5; the piece-count prior per map kind lives in the profile, as a report.
- Regions are closed by construction, since membership is per hex.
- Rings and glyphs have no structure to check beyond the legend's vocabulary.
- Every score in the middle band is a doubt, produced by arithmetic; the doubts and the difference image
  are what a person sees.

## 7. The lattice, and the scans that fight it

- Period and orientation from the autocorrelation of the scan: pointy and flat lattices put their peaks
  at different angles, so orientation is measured, not declared. Phase from one hexagon template at the
  period. Extent from where the periodic energy exists, which is also where furniture is not.
- Numbering from two printed ids far apart, read by a person or one crop, plus the offset convention,
  which the lattice geometry gives directly.
- Rotated maps: Borodino and Red Dragon print text for both sides; the lattice does not care, only the
  label pass does.
- Two sheets: DS prints both grids on one image with a gap; TK is two scans to splice by their printed
  ids. Both are two lattices with their own fit, and a seam declared between named columns.
- Photograph with wrinkles (Olympic): one global lattice fails. Ben's marked cells are the answer: a
  lattice fitted piecewise, each patch anchored by counting hexes from a known id, which is possible
  precisely because the print is a perfect grid. The tool needs a local phase tracker, not a warp.
- Haze and low resolution (Invasion, Borodino, TRC at 1224 px): the strips still work, the doubt band
  widens, and the doubts list gets longer. Report the hex size in pixels before starting; under about
  25 px the answer is a better scan, not a better reader.
- Grid drawn only over land (Target Leningrad), grid invisible over land (Arctic), sea hexes cut by the
  map edge (TRC): all clip cases, found by the periodic-energy map plus one overview for a person's eye.

## 8. What this asks of the sheet grammar

Proposals only; every one is a review gate in PLAN.md.

1. A vertex mark: `<vertex at="HEX:CORNER" mark="..."/>` or a `corner` slot on the hex. Arctic Disaster
   needs it; corner-only grids need it for the grid stroke itself.
2. The legend as declared marks (the shape and meaning split already proposed), so that reading the
   legend produces the style document and nothing else does.
3. A ring is a mark kind on the hex, not a special attribute pair, once marks exist.
4. Several lines on one hexside are already expressible as several `edge` elements; the renderer needs
   a lateral offset rule per line kind so coincident borders draw as printed (Downfall).
5. Link chains gain an end reason at each end (`edge`, `place`, `junction`, `dot`, `bank`), since the
   rules read them (PGG's road cut by a river) and the checker needs them.
6. Letter codes in hexes (Olympic C and L, Tannenberg H and A) are glyphs of kind `text` with meaning
   bound in the legend, not terrain.
7. Regions stay hex sets; the outline is never authored.
8. A default style library by house and decade, for maps with no legend, declared as an assumption in
   the document.

## 9. The eighteen maps

| map | house, year | orientation | ids | legend | what it teaches |
|---|---|---|---|---|---|
| Arctic Disaster | LPS 2016 | pointy | rotated on land, faint | none on sheet | vertex marks; grid invisible over land; ice out of play; boxes over hexes |
| Battle for Moscow | GDW 1986 | pointy | none | none | standard symbology; regions as tint; blobs clipped to hexes |
| BFM redone | fan, later | pointy | corners only | TEC rotated | same map, different style: structure identical |
| Borodino | SPI 1972 | pointy | edge letters | TEC on sheet | rotated labels; redoubts as side glyphs; low resolution |
| Tannenberg | SPI 1978 | pointy | top | full key | two rail kinds; blocked hexsides; rings; off-map numbers; trenches |
| D-Day at Tarawa | DG 2014 | pointy | badges only | two panels | side glyphs in thousands; colour as data; irregular outline |
| Dai Senso | DG 2011 | pointy, two grids | tiny | full key and TEC | four glyphs a hex; six border kinds; boxes over hexes |
| Downfall | GMT 2023 | pointy | tiny | key | coincident border bands; abstract links; objective rings |
| Olympic | DG 2012 | pointy | tiny | TEC | wrinkled photo; letter codes; piecewise lattice |
| PGG (Russian) | AH 1976 art | flat | top | key | off-centre straight rail; blobs ignoring hexes; grey terminus dots |
| Red Dragon Blue Dragon | ATO 2015 | pointy | bottom | none | standard symbology; double-line link; two-sided furniture |
| Siege of Tallinn | unknown | pointy | top | none as scanned | three link kinds; clean case |
| Stalin Moves West | DG 2018 | pointy | west, rotated | TEC | thin rail; two border dash styles; bridges; irregular outline |
| Target Leningrad | VPG 2010 | pointy | top | none | grid only over land; river bands; edge markers |
| Invasion of Russia, two editions | ES 2021 | pointy | small | pictorial | same map twice: borders as bands; bridges as bars; haze |
| Russian Campaign 5th | GMT 2021 | pointy | tiny | TEC rotated | rule-bearing coast line; districts; low resolution |
| Totaler Krieg, two scans | DG 2011 | pointy, two sheets | tiny | key and TEC | splice by ids; TEC split hex and hexside |
| Bagration Stopped | DG 2023 | pointy | tiny | key rotated | dense road mesh in two weights; front line |

## 10. The rendering-swap test (Ben, 2026-09-18)

None of these games is a reproduction target. They are use cases for the STRUCTURE: that a road, a
railroad, a minor river, a major river, a border of a given kind, a ring, a bridge can each be
represented, and that the representation is the same whichever house drew it. How SMW draws a railway
(thin black with ticks) and how PGG draws one (black-white dashes cutting hexes) are two renderings of
one structure, and swapping them must be optional and harmless.

So section 3's catalogue of how each primitive is printed is not only a reading aid; it is the test of
the design. Three tests follow from it, all automatic and none needing a model:

1. **Swap.** Take the structure document of map X and the style document of map Y; the result must
   validate and render. Ugly is allowed. Failure to render means a structure leaked into a style or the
   reverse. Run it over every pair; there are eighteen styles for every structure.
2. **Common style.** Render every map's structure with ONE neutral style. Every feature kind on every
   map must appear; a kind that only renders under its own house style is not a kind, it is a picture.
3. **Round trip.** Read a map to structure, render it with any style, read the render back to structure.
   The two structures must be identical. Since the reader measures cells against the style's own
   swatches, a clean render is the easiest possible input; a difference is a defect in the reader, the
   renderer or the split, and this test finds it at zero model cost on every map, every build.

The rendering catalogue (section 3, made into a table per kind with the swatch crops as its entries)
becomes the style library that maps with no legend borrow from, and each new map adds its house's
rows.

## 11. What to build, in order, once this is approved

1. Lattice finder with extent, furniture rectangles and a piecewise mode.
2. Legend reader: panel finder, swatch cutter, one model look to name the swatches, style declarations
   out.
3. Cell scorer: the strips and patches of section 5, templates from the swatches, scores per cell.
4. Chain and region builder with hysteresis and end reasons, writing the chain files the checker reads.
5. Doubts and difference image, then the editor of task 16.

Copyright Ben Paul Wise. All Rights Reserved.
