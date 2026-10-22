Copyright Ben Paul Wise. All Rights Reserved.

# Task 09: Dai Senso map sheet cleanup and network continuity (milestone M6d)

status: review
worker: W5 (opus)            started: 2026-09-14
resume: done 2026-09-14 - dai-senso.xml repaired (network_check 1989 -> 0 broken, tidy idempotent), TRC/PGG 0, SVG/PNG regenerated and looked at, validate-xml 4 valid, banners 0 failures, hygiene_map_networks covers dai-senso.xml, foreground ctest --preset win-msvc-debug 185/185; no git writes; awaiting review
inputs:
  hexgames/PLAN.md "Terms", RESUME HERE, decision log entries of 2026-09-14 "Maps", "TRC country borders",
    "Smoothed roads and railways" and "Smoothed rivers"; CLAUDE.md
  hexgames/tasks/08-map-networks.md          (the TRC/PGG precedent: method, tools, lessons; read it first)
  hexgames/map_graphics/xml/hexsheet.xsd, README.md, hexsheet2svg.py
  hexgames/map_graphics/xml/dai-senso.xml     (the sheet you repair)
  hexgames/map_graphics/xml/tools/network_check.py, tidy_networks.py, build_ds.py, other tools/*.py
  hexgames/game_rules/dai-senso.md, game_rules/xml/dai-senso.xml, map_graphics/dai-senso-graphics.md
    (what the map's features mean in play: roads, borders, weather/zone lines, mountain hexsides, ports)
  reference images in C:\Library\War-Games\Dai Senso Pacific WWII, READ ONLY THROUGH SMALL COPIES OR CROPS:
    map-detail.png (3168 x 4896, 46 MB), pic4573603.png (4400 x 3562, 10 MB),
    Dai Senso game map adjusted.png (3990 x 3198, 15 MB; the scan the sheet's pixel frame and hex labels follow),
    pic841587 adjusted.png and the smaller pic*.jpg photos
    Note from map_graphics/xml/README.md: map-detail.png numbers hexes one row and one column lower than the
    game-map scan; the game-map scan's own labels are the ones the sheet follows.
  for any test that pins DS sheet data only: hexxml/test/SheetDocTest.cpp and other tests that read dai-senso.xml
outputs:
  hexgames/map_graphics/xml/dai-senso.xml     repaired (road links, border, zone and mountain hexside lines, and any
    misplaced terrain, city, capital or port the images show to be wrong); still valid against hexsheet.xsd
  hexgames/map_graphics/xml/dai-senso.{svg,png} regenerated with hexsheet2svg.py (roads smoothed as the renderer
    now does; border, zone and mountain lines stay straight along the hexsides)
  network_check.py and tidy_networks.py extended with Dai Senso's own acceptance rules (per-sheet configuration,
    not special cases scattered in code); the hygiene_map_networks ctest entry covers dai-senso.xml as well
  tests that pinned old DS sheet data updated, each named with its reason
acceptance:
  roads: on every landmass (a connected set of land hexes) that has two or more named places, the road network
    is one connected piece reaching every capital and city on it and every port the images show on a road; roads
    continue across the seam between the west and east grids where the images show them; no road enters a sea
    hex; no road piece shorter than 3 hexes unless the images show one (name each such exception)
  borders: every border piece is a continuous hexside chain whose ends lie on sea, a map edge, the seam onto the
    other grid's continuation, or another border; they follow the images to within a hex
  zone lines: the same continuity rule as borders, following the printed weather/zone boundaries
  mountain hexsides: they form ranges as printed; no isolated one- or two-hexside specks unless printed
  ports: on coastal hexes (at least one sea neighbour); capitals and cities on land or on a coastal hex
  no hexside line inside a sea hex pair where the images show open water; nothing else in the sheet changes
    without a stated reason (terrain, labels, panels, the region)
  python tools/validate-xml.py map_graphics/xml -> all valid; hexsheet2svg.py renders with no new warnings;
  network_check.py exits 0 on all three sheets (TRC, PGG unchanged); tools/banner-check.py . -> 0 failures;
  ctest --preset win-msvc-debug 100% (CLion's ctest.exe)
  rail (scope addition 2026-09-14): a `rail` line style in <lines> reading like the print (white, grey casing, ticks
    if the key shows them) and <link kind="rail"> chains following the printed rail city to city; one connected
    rail network on each landmass that has rail, continuing across the seam where printed; no rail into sea hexes;
    no short stray pieces unless printed (name each). network_check.py carries Dai Senso's rail rules.
  Ben's own checks, against pic4573603.png as the primary reference (cross-check with the game-map scan and
    map-detail.png under counters): (a) India's transportation network, every printed rail and road line and
    junction, city to city; (b) the dashed boundaries of the central Pacific, both the red/yellow country and
    dependent borders and the white/grey naval zone borders, round the Eastern Carolines, the Marshall Islands, the
    Gilbert Islands and out to Johnston Island, exactly as printed.
  sea boundaries (clarification): border and zone lines printed over open water are correct; "no hexside line inside
    a sea hex pair" applies only to lines the print does not show. At sea a line's ends lie on a map edge, the seam
    (continuing on the other grid), another boundary, or land where the print ends it.
log:
- 2026-09-14 created by the coordinator. At creation: two pointy grids (west 27x53, east 29x53, one lattice);
  hexside lines border 283, zone 682, mountain 434; road links 517; glyphs capital 47, city 24, port 330; no rail
  or river lines. network_check.py (built for TRC/PGG rules) reported 546 broken rules, most of them its "road
  reaches every named place" rule, which cannot hold on an island map; hence Dai Senso's own rules above.
- 2026-09-14 scope addition (Ben, via the coordinator): add Dai Senso's rail network (a `rail` line style and
  <link kind="rail"> chains, per-sheet rail rules in network_check.py; renderer unchanged). Ben will check the result
  against pic4573603.png for India's rail and road network and for the central Pacific borders and naval zone
  borders (Eastern Carolines, Marshalls, Gilberts, out to Johnston Island). Printed sea boundaries are correct; see
  the acceptance notes above.
- 2026-09-14 clarification (Ben, via the coordinator): India's network and the central Pacific boundaries are
  examples, not the limit. Fix the whole map to that standard against pic4573603.png (primary; the game-map scan
  and map-detail.png under counters): every rail and road line as printed (city to city, junctions, across the
  seam); every country/dependent border, naval zone border and region border; mountain hexsides; lakes and rivers
  where printed; all cities, capitals and ports. Region by region across the whole sheet.

## Choices and results (worker fills in: method, per-kind before/after counts, exceptions)

### Summary
Method, in order:
1. Calibrated the game-map scan (the sheet's own frame) and pic4573603.png (photo x = 9.05 + 1.0904 * sheet x,
   y = 50.25 + 1.0904 * sheet y) against printed hex numbers; map-detail.png is a different grid, used by eye only.
2. Read the whole printed map on the photo in 92 tiles drawn with the sheet's grid and ids centred on the hexes
   (land 1.5x, sea 1.0x), plus the scan and map-detail.png where counters hide the print; exact hexes of places under
   counters from glyph pixels (scratchpad hexat.py). Every reading is recorded below, region by region.
3. Hexside lines (border, zone, region, mountain, river, lake) are scored on both images along every hexside against
   6 px inset bands (scratchpad sample_ds.py, sample_photo.py, classify_ds.py) and written into the sheet by a one-off
   trace (scratchpad ds_trace.py), as tasks/08's TRC border trace was; tidy_networks.py then repairs them.
4. Links and places are authored data read off the print, in tidy_networks.py SHEETS["dai-senso"].

Tools (both keep the copyright line first and last; TRC and PGG results unchanged, both files byte-identical):
- network_check.py: per-sheet rule profiles (PROFILES; an unknown sheet id or kind is an error). Neighbours now cross
  grids that share a lattice (the DS seam). Dai Senso profile: boundaries (border, zone, region) continuous with ends
  on the map edge, a coast or another boundary, over land or open water; mountains between land hexes, no specks;
  rivers drain; links on land only; rail pieces and whole networks each hold exactly one named printed system
  (DS_RAIL_SYSTEMS, DS_NETWORKS) with connected straits (DS_STRAITS) as joins; every capital and city on the network
  unless listed (DS_UNLINKED); short printed links listed (DS_SHORT); places on land, ports coastal (DS_DELTA_PORTS
  for Dacca). DS_OFFMAP lists the hexes outside the printed map (paper margin, off-map boxes) as the map edge.
- tidy_networks.py: Boundaries (systematic repair of a sheet's own hexside lines: drop disallowed hexsides, join a
  loose end within 4 hexsides (rivers 2) to where it may end or to the line outside its own branch, prune spurs,
  drop specks and undrained rivers; authored boundary_add / boundary_remove first); authored links routed over land
  between listed hexes; authored places; land fixes; rewrite() adds new line and link kinds, place glyphs and a
  clear-terrain block, and keeps one count comment. A second run on each sheet: "nothing to do".
- dai-senso.xml <lines>: rail (white, grey casing, ticks, as the Terrain Key), road now a thin grey line (the print's
  road; rail took the white cased style), region (pale dotted), river and lake (blue); four palette colours added.
- CMakeLists.txt: hygiene_map_networks also runs network_check.py on dai-senso.xml.

network_check.py on dai-senso.xml, before (the start state under Dai Senso's own rules) and after:

| kind | before | after |
|---|---|---|
| border | 283 hexsides / 111 pieces | 361 / 12 (largest 239) |
| zone | 682 / 249 | 377 / 32 (largest 149) |
| region | none | 129 / 21 |
| mountain | 434 / 147 | 113 / 19 (largest 16) |
| river | none | 171 / 35 |
| lake | none | 18 / 15 |
| rail | none | 218 hexes / 13 pieces (72 chains): 11 named systems plus the listed two-hex lines |
| road | 574 hexes / 120 pieces | 40 / 6 (8 chains) |
| places | capital 47, city 24, port 330 (traced, mostly noise) | capital 27, city 63, town 60, port 99 |
| terrain | - | 115 hexes sea -> clear (printed places and rail on coastal hexes) |
| broken rules | 1989 | 0 |

Explicit data and exceptions (each named):
- Land fix, 115 hexes: 103 place hexes and 12 rail hexes the sampled terrain called sea (see repair log).
- DS_UNLINKED (capitals and cities with no printed link): Lhasa w4715, Kyzyl Khoto w5818, Yenan w5021, Petropavlovsk
  e5810, Colombo w3509; Borneo w3220 w3322 w3022 w3423 w3223; Celebes w3126 w2925 w2824; New Guinea e3004 e2906
  e2707 e3001 e2606; Davao w3426, Rabaul e2808, Honolulu e4226.
- DS_SHORT (printed two-hex links): rail Palembang-Telukbetung, Taihoku-Tainan, the South Island line; road
  Ledo-Myitkyina, Kweilin-Nanning.
- DS_STRAITS (connected strait arrows counted as joins): e4801-e4902, e4802-e4803, e4802-e4902, e5105-e5205,
  e1317-e1318.
- DS_DELTA_PORTS: Dacca w4314 (slot sw).
- boundary_add: zone w3108:se w3007:e w3007:se w2907:e; region w4313:e w4213:ne w4213:e. boundary_remove: region
  w4207:ne w4208:ne w4307:se w4308:se.
- Deviation from the acceptance wording: networks and rail systems are named rather than derived from "a connected
  set of land hexes", because at hex scale coastal hexes touch across straits the print shows as open water
  (Malacca, Sunda, Korea, Formosa, Tatar, La Perouse, Palk), which would merge islands into Asia.

### network_check.py under Dai Senso's own rules, before any repair
- The untouched sheet (2026-09-14 start state): 1989 broken rules. Largest groups: 279 road hexes off the printed
  map, 206 one-hexside zone pieces, 159 ports on sea hexes, 403 zone hexsides on the map edge or off-map, 125
  one-hexside border pieces, 114 road hexes in sea, 276 mountain hexsides not between two land hexes, 104 mountain
  specks, 62 two-hex road pieces, 40 ports with no sea neighbour, 25 capitals on sea hexes; no rail. Totals:
  border 283 hexsides / 111 pieces; mountain 434 / 147; zone 682 / 249; road 574 hexes / 120 pieces.

### Image findings (recorded as they are made)
- Calibration (2026-09-14): the sheet grids (size 40.95, west ox 62.54, east ox 1977.63, oy -8.05) overlay
  "Dai Senso game map adjusted.png" exactly; printed hex numbers match the sheet ids (checked w5127 near Heijo,
  w4927 by Fusan, e4902 Hiroshima/Kyoto area). The seam is seamless: east col 01 sits one hex east of west col 27.
  The scan carries counters (placed game) that hide features under them; pic4573603.png has counters elsewhere.
- Printed legend (Terrain Key): Rail = white line with grey casing; Road = thin grey line; Town = small white
  square; City = yellow disc, red rim; Provisional Capital = red star on yellow disc; Capital = grey urban blob
  with the name in capitals (TOKYO); Port = anchor on a blue disc (multi-zone: two tones; key port: ship);
  Naval Zone Border = white dashed on sea hexsides; Country/Dependent border = yellow-orange dashed hexside;
  Region border = pale grey-white dotted hexside; Mountain hexside = brown bar; Lake and River hexsides blue.
- map-detail.png (West Map only, no counters): calibrate.py finds size 62.30, ox -40.74, oy -80.05 on a sea crop,
  but it is not a plain rescale of the game-map scan: Colombo is gm w3509 / md label 3308, Singapore gm w3218 /
  md label 3117 (row offset 2 vs 1). Used by eye only, to see features under counters; geography decides the hex.
- pic4573603.png: calibrate.py on open sea gives size 44.65, ox -38.76, oy -25.50 (scale 1.090 to the sheet);
  counters in different places from the game-map scan, so a second look under counters.
- pic4573603.png pinned to the sheet ids (scratchpad ovl45.py): photo x = 9.05 + 1.0904 * sheet x, photo y = 50.25 +
  1.0904 * sheet y (the calibrated photo grid, offset 1.5 columns and 1 row from the sheet's west grid). Checked
  on Omsk w6012, Novosibirsk w6015, Semipalatinsk w5714, Krasnoyarsk w6018, New Delhi w4610, Dacca w4314, Mandalay
  w4215. The photo's India and Burma are nearly free of counters, so it is the better source there.
- hexmodel/BoardBuilder.cpp skips a <link> kind the package does not bind (line 287), so adding rail links to the
  sheet leaves BoardTest.DaiSensoTwoGridsOneLattice unaffected.
- Links are not centre to centre on the print: rail (white, grey casing, ticks) and road (grey) run city to
  city in straight or gently curved strokes; a sheet link is the chain of hexes the stroke passes through.
- Region R01 (scan x700-1320, y0-470; Siberia, Sinkiang north): rail along row 60 from the west edge
  w6011-w6012 Omsk (city)-w6013-w6014-w6015 Novosibirsk (city)-w6016-w6017-w6018 Krasnoyarsk (town); rail
  Omsk south-east to Semipalatinsk w5714 (town) via w5913 w5813; rail Novosibirsk south-west to Semipalatinsk
  via w5915 w5814; rail Semipalatinsk south via w5613 w5513 w5412 to Alma Ata w5411 (city); grey road from the
  rail near w5613 east via w5514 to Urumchi w5415 (Soviet strategic ring) and on east along row 54.
- Region R02 (x1280-1900, y0-470; Baikal, Mongolia, Manchukuo north): rail row 60 w6018-w6019-w6020, Taishet,
  then w5921-Irkutsk w5821 (provisional capital, red star)-round Baikal's south end-Ulan Ude w5722 (town)-w5723
  -w5724-w5824 (ring)-w5925-w5926 east (Trans-Baikal, continues to the seam at w5827/e5801); branch w5824 south-
  east to Hailar w5625 (town) into Manchukuo; grey road Ulan Ude south to Ulan Bator w5621 (capital ring,
  under a counter), then south-east into Inner Mongolia and east to Nomonhan w5624.
- Region R03 (x250-870, y420-890; north-west India): Kashgar w5210 (town); Peshawar w4908 (town); Lahore
  w4709 (city); rail Peshawar-w4808-Lahore and on south-east. The coast of the map's west margin is off-grid.
- Region R04 (x830-1450, y420-890; Sinkiang, Tibet, Kansu): Aksu w5212 (town), Khotan w5012 (town), Xining w5118
  (city), Lanchow w5019 (ring, under a counter), Lhasa w4715 (under a counter), Chengtu w4719 (city); grey road
  Urumchi w5415-w5416-w5317-w5217-Xining w5118-w5119-Lanchow w5019. Tibet and Szechwan carry long brown mountain
  hexside ranges along the hex rows 48-50.
- Region R05 (x1400-2000, y420-890; Hopeh, Manchukuo south, Korea; many counters): Kweisui w5221 (town),
  Peiping w5223 (capital, green ring), Chengteh w5224 (town), Tientsin w5124 (port), Dairen w5126 (city, port,
  ring), Tsinan w5024 (town), Tsingtao w5025 (port), Taiyuan w5022, Kaifeng w4923 (town), Tungshan w4924 (town),
  Chengchow w4922, Sian w4920, Hsinking w5327 (town), Mukden w5227 (city), Heijo w5127 (town), Keijo w5027
  (ring), Fusan w4927/e4901 (port). Rail: Peiping-Tientsin-Tsinan-Tungshan-south to Nanking; Peiping south via
  w5123 w5023 to Chengchow w4922 and on south to Hankow; Sian w4920-Chengchow-Kaifeng w4923-Tungshan w4924 along
  row 49; Tientsin north-east along the coast to Mukden w5227; Mukden-Hsinking w5327-Harbin w5427 north; Mukden
  south-west to Dairen w5126; Mukden south-east via Heijo w5127 to Keijo w5027 and Fusan; Peiping north-west to
  Kweisui w5221 and north into Mongolia.
- Region R06 (x1100-1720, y850-1320; Yunnan, south China, Tonkin): Chungking w4620 (capital, green ring),
  Kweiyang w4520 (city), Kunming w4418 (ring), Nanning w4320 (town), Hanoi w4219 (city, port), Haikou w4121
  (Hainan), Canton w4322, Hong Kong w4222 (green ring, port), Changsha w4623, Amoy w4324 (port), Nanking w4724,
  Myitkyina w4416 (town), Lashio w4216 (town), Chiengmai w4016 (town), Vientiane w4018 (town). Burma Road (grey)
  Lashio-Kunming; road Kunming-Kweiyang-Chungking; rail Kunming-Hanoi (Yunnan railway); rail Hanoi-Nanning north
  towards Kweiyang; rail Hankow-Changsha-Canton-Hong Kong; rail from Hanoi south along the coast.
- Region R07 (x500-1120, y850-1320; India, Bengal): New Delhi w4610 (provisional capital), Jaipur w4509 (city),
  Cawnpore w4410 (city), Patna w4412 (town), Ahmadabad w4308 (city), Nagpur w4209 (town), Hyderabad w4009 (city),
  Calcutta w4213 (ring, port, under a counter), Dacca w4314 (provisional capital), Imphal w4415 (town), Akyab
  w4115 (town, port), Rangoon w4015 (provisional capital), Mandalay w4215 (city), Katmandu w4513 (town), ports
  Brahmapur w4111 and Kakinada w4010. Rail: Lahore-New Delhi; New Delhi-Jaipur-Ahmadabad; New Delhi-Cawnpore-
  Patna-east along row 44 to Imphal; Calcutta-Dacca; Calcutta west to Nagpur-Ahmadabad; Hyderabad-Kakinada;
  Ahmadabad south to Bombay; Calcutta south-west along the coast.
- Region R08 (x1000-1620, y1280-1750; Siam, Indochina, Malaya north): Rangoon w4015 (provisional capital,
  port), Moulmein w3916 (town), Vientiane w4018 (town), Bangkok w3817 (capital, under a counter), Hue w3920 (city,
  port), Phnom Penh w3618 (town), Saigon w3619 (green ring, port), Singora w3517 (port), Kota Bharu w3417 (port),
  Medan w3316 (port), Brunei w3322 (city, oil). Rail: Rangoon-Moulmein-south down the Kra isthmus by Singora to
  Malaya; Bangkok north to Chiengmai; Bangkok-Phnom Penh-Saigon; Saigon north along the coast by Hue to Hanoi.
- Region R09 (x380-1000, y1280-1750; south India, Ceylon): Bangalore w3808 (city), Mangalore w3807 (port), Madras
  w3809 (port), Madurai w3608 (town), Jaffna w3609 (town), Colombo w3509 (provisional capital, port, green
  ring), Trincomalee w3510 (port), Male w3406 (port), Port Blair w3714 (port). Rail: Hyderabad south to
  Bangalore; Bangalore-Mangalore; Bangalore-Madras; Bangalore north-west towards Bombay.
- Region R10 (x1000-1620, y1700-2170; Malaya south, Sumatra, Java, Borneo west): Kuala Lumpur w3217 (town),
  Singapore w3218 (provisional capital, port; pale-blue coastal hex), Kuching w3220 (city, port), Padang w3016
  (town), Palembang w2919 (city, oil), Telukbetung w2818 (port), Batavia w2819 (capital, ring, under a counter),
  Bandung w2720 (town), Surabaya w2722 (city, port), Bundjermasin w3021 (town), Balikpapan w3022 (city, oil,
  green ring). Rail: Malaya south to Kuala Lumpur and Singapore; Palembang-Telukbetung; Batavia-Bandung-Surabaya
  along Java. Sumatra has a brown mountain range w3117-w3017-w2918.
- Region R11 (x1580-2200, y1300-1770; Philippines, Borneo north-east, Palau): Manila w3924 (capital under a
  counter, port, ring), Aparri (north Luzon, port), Legaspi w3825 (town), Cebu w3625 (port), Leyte w3626 (port),
  Davao w3426 (city, port, green ring), Sandakan w3423 (city, port), Tarakan w3323 (city, port), Palau e3402
  (Axis ring). Rail Aparri south to Manila. A dependent border (yellow dashes) rings the Western Caroline Islands
  in open sea (e3502-e3603): DS prints dependent borders round island groups, so border hexsides in sea are real.
- Region R12 (x2150-2770, y1650-2120; Bismarck Sea, north New Guinea; East Map): Hollandia e3004 (city,
  port), Wewak e2906 (city, port), Madang e2806 (town), Lae e2707 (city, port), Rabaul e2808 (city, port, green
  ring), Kavieng e2909 (port), Buin e2710 (port), Truk (Axis ring, row 34), Ponape e3411. A brown range runs along
  New Guinea e2904-e2805-e2706; the Eastern Caroline Islands dependent border rings the islands in open sea. No
  roads or rail on New Guinea.
- Region R13 (x1650-2270, y2080-2550; Timor, Arafura, Darwin; straddles the seam): Koepang w2625 (port, pale
  hex), Dili w2626 (town), Darwin e2401 (port), Broome w2225 (port), Cloncurry e2004 (town). No roads or rail
  here; a naval zone border runs south from Timor to the Australian coast at w2426.
- Region R14 (x2700-3320, y1750-2220; Gilbert and Ellice Islands, Solomons east): Nauru e3014 (port), Tarawa
  e3117 (port), Guadalcanal e2711 (port), Funafati e2618 (port). The Gilbert Islands dependent border rings
  e3115-e3117-e3016-e3014 in open sea; naval zone borders cross the area north-south. No land links.
- Region R15 (x1950-2570, y150-620; Russian Far East, Sakhalin, Hokkaido; East Map): Blagoveshchensk e5701 (under
  a counter), Volochaevsk e5703 (town), Komsomolsk e5803 (town), Khabarovsk e5603 (city), Sovetskaya Gavan e5604
  (town), Vladivostok e5302 (ring, port, under a counter), Okha e5806 (port, ring), Alexandrovsk e5706 (town),
  Shikuka e5606 (town), Toyohara e5506 (under a counter), Wakkanai e5405, Asahigawa e5406 (town), Tokotan e5407
  (port, ring), Kushiro e5307 (port), Sapporo e5305 (city, ring), Hakodate e5205 (port). Rail: Trans-Siberian
  from the seam by Blagoveshchensk east to Volochaevsk e5703-Khabarovsk e5603, then south by e5503 e5402 to
  Vladivostok; branch Volochaevsk north to Komsomolsk e5803 and east to Sovetskaya Gavan e5604. Sakhalin: line
  Alexandrovsk-Shikuka-Toyohara. Hokkaido: Wakkanai-Asahigawa-Sapporo-Hakodate and Asahigawa-Kushiro.
- Region R16 (x1980-2600, y1600-2070; Palau, western New Guinea): Sorong e3001 (city, oil, port), Biak e3003,
  Hollandia e3004 (city, port), Palau e3402 (Axis ring), Truk e3408 (Axis ring). Dependent borders ring the
  Palau and Eastern Caroline groups in sea; a border runs along New Guinea's north coast by Hollandia.
- Region R17 (x200-820, y950-1420; west India): Karachi w4505 (provisional capital, port), Hyderabad (Sind) w4506
  (town), Bombay w4107 (city, port, green ring, under a counter), Ahmadabad w4308 (city), Nagpur w4209 (town),
  Hyderabad w4009 (city), Kakinada w4010 (port). Rail: Karachi-Hyderabad (Sind)-east along row 45 to Jaipur;
  Ahmadabad south by w4207 to Bombay; Bombay east by w4108 to Nagpur; Bombay south along the west coast;
  Hyderabad north to Nagpur and south to Bangalore; Nagpur east towards Calcutta.
- Region R18 (x1480-2100, y780-1250; Kiangsu coast, Formosa): Shanghai w4825 (ring, port, under a counter),
  Nanking w4724 (green ring, under a counter), Hankow w4722, Ningpo w4725 (town), Wenchow w4525 (town), Foochow
  w4424 (under a counter), Amoy w4324 (town), Taihoku w4326 (Axis ring, under a counter), Tainan w4225, Okinawa
  w4427 (Axis ring), Nagasaki e4801, Kagoshima e4702. Rail Nanking-Shanghai; Formosa Taihoku-Tainan.
- Region R19 (x1820-2440, y1620-2090; Celebes north, Moluccas; straddles the seam): Menado w3126 (city, port,
  pale hex), Ternate w3127 (town), Amboina w2927 (port, pale hex), Sorong e3001 (city, oil, port), Davao w3426,
  Palau e3402. No land links.
- Region R20 (x2150-2770, y450-920; Honshu east, Hokkaido south): Hakodate e5205 (port, pale hex), Aomori e5105
  (town), Ominato e5106 (port), Niigata e5004 (town), Sendai e5005 (city, port, Axis ring), Tokyo e4905
  (capital, port, Axis ring), Nagoya e4904 (city), Osaka e4803 (city). Rail Hakodate-Aomori across the
  connected strait; Aomori-Sendai-Tokyo; Aomori-Niigata-Kanazawa along the Sea of Japan coast; Niigata-Tokyo;
  Tokyo-Nagoya-Kyoto-Osaka and on west to Hiroshima (see the Japan crop at the start).
- Region R21 (x2000-2620, y2600-3070; south-east Australia): Alice Springs e1802 (town), Rockhampton e1808
  (port), Brisbane e1609 (under a counter), Newcastle e1408 (port), Sydney e1309 (city, port, green ring),
  Canberra e1308 (provisional capital), Adelaide e1304 (city, port), Melbourne e1207 (under a counter). Rail:
  Rockhampton south along the coast by Brisbane and Newcastle to Sydney; Sydney-Canberra-west along row 13 to
  Adelaide; Canberra south-west to Melbourne; Adelaide-Melbourne; Adelaide north-west round the gulf and west
  (the trans-Australian line).
- Region R22 (x1500-2120, y2650-3120; south-west Australia): Geraldton w1622 (town), Perth w1423 (city, port).
  Rail Geraldton-Perth; Perth east along row 15 (the trans-Australian line) across the seam to Adelaide.
- Region R23 (x2800-3420, y2700-3170; New Zealand): Auckland e1517 (green ring, under a counter), Wellington
  e1318 (provisional capital, port), Christchurch e1217 (port). Road Auckland-e1417-Wellington; South Island
  Christchurch north to the Cook Strait (connected strait arrow to Wellington).
- READING PITFALL (found on the photo crops): the overlays drew each id BELOW its hex centre, so a glyph in the
  upper half of a hex reads as the hex above. New Delhi is w4510, not w4610 as written in R07. The hex of every
  place above is therefore provisional: final place hexes come from glyph centroids detected on the scan
  (scratchpad blobs_ds.py, nearest hex centre), with the readings giving only name and kind. Overlays now centre
  the id on the hex centre.
- Photo India (primary for Ben's check): rail Peshawar-Lahore-New Delhi; New Delhi-Jaipur-Hyderabad (Sind)-
  Karachi; Jaipur-Ahmadabad; New Delhi-Cawnpore-Patna-east along the Ganges to Imphal-north-east to Ledo;
  Calcutta north-east to the Patna-Imphal line (by Dacca); Calcutta south-west along the coast by Brahmapur;
  Calcutta west to Nagpur; Ahmadabad south along the coast to Bombay; Ahmadabad-Nagpur; Bombay-east-Nagpur;
  Nagpur-Hyderabad; Hyderabad-Kakinada; Hyderabad-Bangalore; Bombay south along the west coast to Mangalore;
  Bangalore-Mangalore; Bangalore-Madras. Burma: rail Myitkyina south to Mandalay; Mandalay-Lashio; Mandalay south
  to Rangoon; grey road Ledo-Myitkyina and Lashio north-east to Kunming (Burma Road). The white dotted lines in
  India (Pakistan, Bangladesh) are region borders, not roads. Ceylon has no lines.
- Photo central Pacific (primary for Ben's check): Saipan e3907, Guam e3806 (port), Ulithi e3604 (port), Yap
  (town), Eniwetok e3612 (ring), Kwajalein e3514, Wotje e3515, Majuro e3516 (port), Ponape e3411 (port), Nauru
  e3014 (port), Tarawa e3117 (port), Johnston Island e3923 (port), French Frigate Shoals e4323 (port). Dependent
  borders (red/yellow dashes) ring the Ulithi/Yap group, the Eastern Carolines (Ponape), the Marshall Islands
  (Eniwetok to Majuro, its south side along e3413-e3415) and the Gilbert Islands (Nauru to Tarawa). Naval zone
  borders (white dashes): from the north down e4015-e3916-e3816-e3717 to the Marshalls border; from e4321 south
  by e4221-e4122-e4022 to Johnston Island, and from Johnston west and south-west (e3921-e3820-e3720); from the
  Marshalls south by e3317-e3216 to Tarawa and on south from e3016/e2917. A cross-hatched sea patch south of Guam
  (e3605-e3508) is not a line.
- Whole-map candidate overlay: zone lines follow the naval zone borders across the Pacific; borders follow China,
  Manchukuo, Mongolia, Tibet, Burma, NEI and New Guinea; mountains follow Tibet, Burma, Sumatra, New Guinea.
  False positives sit in the off-map furniture (turn track, VP track, delay boxes, US boxes, Terrain Key) and the
  paper margin west of Russia: the grids cover the whole scan, so the printed map area has to be declared.
- Off-map hexes (scratchpad offmap_ds.py): 860 hexes are outside the printed map, either paper/white round the
  centre on the game-map scan or inside an off-map furniture panel of the sheet (naval zone boxes excluded). The
  preview matches the print: the paper margin west of Russia and India, the boxes and tracks, the Terrain Key,
  the East Map's side boxes. Lines ending on an off-map hex end on the printed map edge.
- Glyph snapping abandoned: detected anchors, discs and squares were too unreliable (most ports and towns missed,
  Nanking snapped onto Changsha's hex). Places are read on photo tiles drawn with the grid and ids centred on the
  hexes, together with the automated line candidates, so each tile checks lines and reads places and links.
- TILE READINGS on pic4573603 (grid + centred ids; these supersede the provisional hexes above). Link paths list
  the hexes a printed line passes through.
  - India (tiles L1001 L1100 L1011 L1110 L2001 L2100). Places: Peshawar w4908 town; Lahore w4709 city; New Delhi
    w4510 capital (provisional); Jaipur w4509 city; Hyderabad (Sind) w4406 town; Karachi w4405 capital + port;
    Ahmadabad w4308 city; Cawnpore w4410 city; Patna w4412 town; Katmandu w4513 town; Nagpur w4209 town; Bombay
    w4107 city + port (under a counter; ring); Hyderabad w4009 city; Kakinada w4010 port; Calcutta w4213 city (ring,
    under a counter); Dacca w4314 capital (provisional) + port; Brahmapur w4112 port; Akyab w4115 town; Mangalore
    w3807 port; Bangalore w3808 city; Madras w3809 port; Madurai w3608 town; Jaffna w3609 town; Colombo w3509
    capital (provisional) + port; Trincomalee w3510 port; Male w3406 port; Port Blair w3714 port.
    Rail: w4908 w4808 w4709 w4609 w4510 (Peshawar-Lahore-New Delhi); w4510 w4509 (New Delhi-Jaipur); w4509 w4508
    w4507 w4406 w4405 (Jaipur-Hyderabad (Sind)-Karachi); w4509 w4408 w4308 (Jaipur-Ahmadabad); w4510 w4410 w4411
    w4412 w4413 w4414 w4415 (New Delhi-Cawnpore-Patna-Imphal); w4308 w4208 w4209 (Ahmadabad-Nagpur); w4107 w4108
    w4109 w4209 (Bombay-Nagpur); w4209 w4110 w4009 (Nagpur-Hyderabad); w4009 w4010 (Hyderabad-Kakinada); w4009 w3909
    w3808 (Hyderabad-Bangalore); w4107 w4007 w3908 w3807 (Bombay south to Mangalore); w3807 w3808 w3809
    (Mangalore-Bangalore-Madras); w4213 w4314 w4414 (Calcutta-Dacca-north to the Imphal line); w4213 w4212 w4211
    w4210 w4209 (Calcutta west to Nagpur); w4213 w4212 w4112 w4011 w4010 (Calcutta down the coast by Brahmapur to
    Kakinada). Ceylon, Nepal and Tibet: no lines. Brown mountain ridges lie UNDER the Nepal and Tibet border dashes,
    so mountain and border share hexsides there (classifier fixed to test them independently).
  - Siberia, Sinkiang, Kansu (tiles L0100 L0101 L0110 L0111; L0001 L0011 are furniture). Places: Omsk w6012 city;
    Novosibirsk w6015 city; Krasnoyarsk w6018 town; Semipalatinsk w5714 town; Alma Ata w5411 city; Kyzyl Khoto w5818
    capital (under a counter); Kashgar w5210 town; Aksu w5212 town; Khotan w5012 town; Urumchi w5415 capital (ring,
    under a counter); Xining w5118 city; Lanchow w5019 capital (grey urban glyph).
    Rail: w6011 w6012 w6013 w6014 w6015 w6016 w6017 w6018 w6019 (Trans-Siberian from the West Map edge, on east);
    w6012 w5913 w5813 w5714 (Omsk-Semipalatinsk); w6015 w5915 w5814 w5714 (Novosibirsk-Semipalatinsk); w5714 w5613
    w5513 w5412 w5411 (Semipalatinsk-Alma Ata). Road (grey): w5415 w5416 w5317 w5217 w5118 w5119 w5019 (Urumchi-
    Xining-Lanchow). To verify: a grey road from the Semipalatinsk rail east to Urumchi.
    Lakes: Balkhash (w5612/w5512 hexsides). Rivers: Irtysh and Ob (partly caught as river candidates). A white-grey
    dotted region border runs w5413-w5212-w5113 (Sinkiang / East Turkestan); region candidates miss it. The dark red
    circle glyphs down the West Map edge (w6011 w5912 w5811 w5711 w5611 w5511 w5411 w5311) are printed symbols
    outside this task's element kinds (left alone).
  - Baikal, Manchukuo, north China, Korea (tiles L0200 L0201 L0210 L0211). Under counters a place's hex is computed
    from its pixel on the game-map scan (row and column straight from sheet pixels), which is exact. Places: Taishet
    w6020 town; Irkutsk w5821 capital (provisional); Ulan Ude w5722 town; Ulan Bator w5621 capital; Chita w5824
    capital (provisional); Hailar w5625 town; Tsitsihar w5527 town (scan pixel); Harbin w5427 capital (provisional;
    scan pixel); Hsinking w5327 town; Mukden w5226 city (under a counter); Dairen w5126 city + port (under a
    counter); Kweisui w5222 town (under a counter); Peiping w5223 capital (green ring); Taiyuan w5022 city; Tientsin
    w5124 port; Tsinan w5024 town; Tsingtao w5025 port; Heijo w5127 town; Keijo w5027 capital (ring, under a
    counter); Genzan e5101 port (under a counter); Blagoveshchensk e5701 city.
    Rail: w6019 w6020 w5921 w5821 (Taishet-Irkutsk); w5821 w5722 w5723 w5724 w5824 (round Baikal to Chita); w5824
    w5825 w5926 w5927 w5827 e5701 (Chita-Amur-Blagoveshchensk, across the seam); w5824 w5725 w5625 (Chita-Hailar);
    w5625 w5626 w5527 w5427 (Hailar-Tsitsihar-Harbin); w5427 w5327 w5226 (Harbin-Hsinking-Mukden); w5226 w5126
    (Mukden-Dairen); w5226 w5225 w5124 (Mukden-Tientsin); w5226 w5127 w5027 (Mukden-Heijo-Keijo); w5327 e5301 e5302
    (Hsinking-Vladivostok); w5427 e5401 e5302 (Harbin-Vladivostok). Road (grey): w5722 w5621 (Ulan Ude-Ulan Bator);
    w5621 w5522 w5422 w5323 w5222 (Ulan Bator south to Kweisui, partly under counters; to verify).
  - Russian Far East, Sakhalin, Hokkaido, north Honshu (tiles L0300 L0310). Places: Volochaevsk e5703 town;
    Komsomolsk e5803 town; Khabarovsk e5603 capital (provisional, ring); Sovetskaya Gavan e5604 port; Nikolaevsk
    e5805 port; Okha e5806 port (ring, under a counter); Alexandrovsk e5706 town; Shikuka e5606 town; Vladivostok
    e5302 city + port (ring); Sapporo e5305 city (ring); Asahigawa e5306 town; Kushiro e5307 port; Hakodate e5205
    port (under a counter); Aomori e5105 town; Ominato e5106 port; Niigata e5004 town; Sendai e5005 city + port
    (ring, under a counter).
    Rail: e5701 e5702 e5703 e5603 (Blagoveshchensk-Volochaevsk-Khabarovsk); e5703 e5803 e5704 e5604 (Volochaevsk-
    Komsomolsk-Sovetskaya Gavan); e5603 e5503 e5402 e5302 (Khabarovsk-Vladivostok); e5405 e5306 e5305 e5205
    (Wakkanai-Asahigawa-Sapporo-Hakodate); e5306 e5307 (Asahigawa-Kushiro); e5105 e5004 (Aomori-Niigata); e5105
    e5005 (Aomori-Sendai). Road (grey): e5803 e5804 e5805 (Komsomolsk-Nikolaevsk); e5806 e5706 e5606 (Sakhalin).
    Connected strait arrow Hakodate e5205 - Aomori e5105 (and Wakkanai-Sakhalin).
  - Checked at 2.5x (v_urumchi): a grey road enters Urumchi from the north-west as well as leaving east, so the road
    from the Semipalatinsk-Alma Ata rail to Urumchi is printed: w5613 w5514 w5415 (approximate hexes).
  - Kamchatka, Kurils, western Aleutians (tiles L0301 L0311 L0400 L0401 L0410 L0411; L0410/L0411 are open sea).
    Places: Petropavlovsk e5810 capital (provisional) + port; Paramushiro e5609 port (Axis ring, under a counter);
    Komandorski e5813 port; Attu e5714 port (under a counter); Kiska e5616 port; Adak e5618 port; Dutch Harbor e5721
    port (green ring, under a counter). No land links. Zone candidates follow the printed naval zone borders here
    (Kurils south-east, Aleutians); a short red country border cuts Kamchatka's south tip (e5710/e5711) and the
    Kurils group is outlined by dependent-border dashes.
  - Alaska, north and central Pacific (tiles L0500 L0501 L0510 L0511 S14 S15; L0500/L0501/L0511 are furniture, the
    Western US Box and Panama Canal Box sit off-map). Places: Cold Bay e5723 port; Wake Island e4114 port; Midway
    e4520 port (green ring); French Frigate Shoals e4323 port; Honolulu e4226 city (green ring, under a counter);
    Hilo e4128 port; Johnston Island e3923 port. No land links. The zone candidates follow every printed naval zone
    border in these tiles (Aleutians south, Wake, Midway, the International Dateline and Central Pacific zones, the
    Hawaiian Islands, the Northeast Pacific); zone borders end on the off-map East Map margin.
  - North China coast, Formosa, Hainan, Japan coast (tiles L1200 L1201 L1210 L1211 L1300 L1301; the photo has many
    counters here, so central China topology comes from map-detail.png and Japan from the game-map scan). Places
    read so far: Sian w4921 city (tile pixel, scratchpad hexat.py); Kaifeng w4923 town; Chengtu w4719 city;
    Kweiyang w4520 city; Kweilin w4420 city; Ningpo w4625 town; Wenchow w4525 town; Tainan w4224 town; Hue w3920
    city + port; Haikou w4221 port; Aparri w4025 port (under a counter); Tokyo e4905 capital + port (under a
    Devastation marker). Zone candidates follow the Yellow Sea, Formosa, Japanese Coast and Bonins zone borders.
  - Exact hexes (scratchpad hexat.py, from glyph pixels on photo tiles or the scan; these override anything above):
    Tungshan w4925 town; Kaifeng w4924 town; Ningpo w4625 town; Wenchow w4525 town; Chengtu w4719 city; Kweiyang w4520
    city; Kweilin w4420 city; Hanoi w4219 city + port; Hue w3920 city + port; Tainan w4224 town; Haikou w4121 port;
    Phnom Penh w3618 town; Singora w3517 port; Kota Bharu w3417 port; Kuala Lumpur w3217 town; Singapore w3218
    capital (provisional) + port; Padang w3016 town; Palembang w2919 city; Telukbetung w2818 port; Sandakan w3423 city
    + port; Nagasaki e4801 city; Sasebo e4801 port; Genzan e5101 port; Fusan e4901 port; Hiroshima e4902 city + port;
    Kyoto e4903 city; Nagoya e4904 city + port; Osaka e4803 city + port; Imabari e4802 town; Kanazawa e5003 town;
    Niigata e5004 town.
  - Central China, topology from map-detail.png (no counters) with hexes from the photo and the scan: Yenan w5021
    city; Taiyuan w5022 city (photo disc; map-detail's own estimate w5123 rejected); Chengchow w4923 city; Wuhan
    (Hankow) w4723 city; Changsha w4622 city; Tsinan w5024 town; Tsingtao w5025 port; Nanking w4725 capital (urban
    glyph just east of its label); Shanghai w4726 city + port. Rail: Sian w4921-w4922-Chengchow w4923-Kaifeng w4924-
    Tungshan w4925 (east-west trunk); Peiping w5223-Taiyuan side? no: Peiping south by w5123 w5023 to Chengchow, then
    south w4823 to Wuhan w4723, w4622 Changsha and on south-west to Kweilin and Canton; Tsinan w5024-Tungshan w4925-
    w4825-Nanking w4725-Shanghai w4726; Tsinan-Tsingtao w5025; Peiping-Tientsin-Tsinan. Road (grey): Lanchow w5019-
    east to Sian w4921; Lanchow south to Chengtu w4719 and Chungking w4620.
  - Japan (scan crop with grid): rail Hiroshima e4902-Kyoto e4903-Nagoya e4904-east to Tokyo e4905; Kyoto-Osaka e4803;
    Nagoya-Osaka; Nagoya and Kyoto north to Kanazawa e5003-Niigata e5004-Aomori; connected strait arrows Hiroshima-
    Kyushu (e4801), Osaka-Shikoku (Imabari e4802)-Hiroshima; Kyushu line south from Nagasaki to Kagoshima.
  - Philippines, Borneo, Celebes, Carolines, Marshalls, Gilberts (tiles L2201 L2210 L2211 L2300 L2301 S24): Legaspi
    town, Cebu w3625 port, Leyte w3626 port, Davao port + city (under a counter), Manila capital + port (ring, under a
    counter, row 38), Kuching w3220 city, Brunei w3322 city, Tarakan w3223 city + port, Balikpapan w3022 city (ring,
    under a counter), Bundjermasin town, Kendari city + port, Makassar city + port, Menado port (under a counter),
    Amboina port, Guam e3806 port, Ulithi e3604 port, Yap e3504 town, Palau e3402 (ring, counter), Truk e3408 (ring,
    counter), Ponape e3411 port, Eniwetok e3612 (ring), Kwajalein e3514 port, Majuro e3516 port, Nauru e3014 port,
    Tarawa e3117 port. Zone candidates follow every naval zone border in S24 and the Carolines; border candidates ring
    the Western and Eastern Carolines, the Marshall Islands and the Gilbert Islands as printed. Many connected strait
    arrows join the Philippine islands (straits, not links).
    Exact hexes (hexat.py): Legaspi w3825 town; Davao w3426 port + city; Manila w3824 capital + port; Bundjermasin
    w2922 town; Kuching w3220 city + port; Brunei w3322 city; Tarakan w3223 city + port; Kendari w2925 city + port;
    Makassar w2824 city + port; Amboina w2927 port; Menado w3126 port + city; Yap e3504 town; Ulithi e3604 port;
    Guam e3806 port.
  - New Guinea north, Solomons, eastern Pacific (tiles L2310 L2311 L2500 L2501 L2510 L2511; L2501/L2511 are the
    Panama Canal Box and Terrain Effects Chart, off-map): Rabaul e2808 city + port (green ring, under a counter);
    Kavieng e2909 port (under a counter); Line Islands e3425 port; Phoenix Islands e2922 port; Biak, Hollandia,
    Wewak, Madang and the Admiralty Islands port (hexes from hexat, below). Zone candidates follow the Bismarck Sea,
    Micronesia, Eastern Pacific and Southeast Pacific borders; border candidates follow New Guinea's north coast
    and the Eastern Carolines ring. No land links on New Guinea.
    Exact hexes (hexat.py): Biak e3003 town; Hollandia e3004 city + port; Wewak e2906 city + port; Madang e2806 town;
    Admiralty Islands e2907 port.
  - Indian Ocean, Java, Timor, north-west Australia (tiles S30 S31 L3200 L3201 L3210 L3211; S30 and the west of S31
    are furniture). Places: Surabaya w2722 city + port (under a counter); Koepang w2625 port (under a counter);
    Darwin e2401 port; Broome w2225 port; Christmas Island, Bandung and Dili (hexes from hexat, below). Rail: Batavia
    w2819-Bandung w2720-w2721-Surabaya w2722 along Java. The Lesser Sunda Islands carry connected strait arrows
    (straits, not links). The cross-hatched sea north of Darwin is a printed sea-area pattern, not a line. Zone
    candidates follow the Southeast Indian Ocean, Timor and Arafura borders. Rivers: the Victoria/Fitzroy near
    w2226/w2227 are caught in part. Exact hexes (hexat.py): Christmas Island w2519 port; Bandung w2720 town; Dili
    w2626 town; Darwin e2401 port.
  - Papua, Queensland, Coral Sea, New Hebrides to Tonga (tiles L3300 L3301 L3310 L3311 S34 L3500; L3500 is the Terrain
    Effects Chart, off-map). Places: Port Moresby city + port (US impact star); Daru town; Cairns port; Lae city;
    Buna town; Gili Gili town; Buin (counter); Guadalcanal port (counter); Cloncurry town; Alice Springs town;
    Townsville city (green ring, under a counter); Rockhampton port; Espiritu Santo, Noumea (US impact star), Suva,
    Funafati, Uvea, Tonga ports; Tutuila (Western Samoa) port with a green ring at the East Map edge (hexes from
    hexat, below). Rail: Cloncurry east to a junction, then north-east to Townsville and on up the coast to Cairns;
    Townsville south down the coast to Rockhampton and beyond; a branch west from Rockhampton inland. Zone candidates
    follow the Coral Sea, Polynesia and South Pacific zone borders; no borders printed on these islands other than
    Papua's.
    Exact hexes (hexat.py): Daru e2605 town; Port Moresby e2606 city + port; Cairns e2206 port; Lae e2707 city; Buna
    e2607 town; Gili Gili e2508 town; Cloncurry e2004 town; Alice Springs e1802 town; Townsville e2107 city;
    Rockhampton e1808 port; Espiritu Santo e2314 port; Noumea e1914 port; Suva e2118 port; Funafati e2618 port; Uvea
    e2420 port; Tonga e1920 port. Rail points: e2005 (on the Cloncurry line), e1906 (the junction), e2206 (the line
    reaching Cairns), e1908 (Townsville-Rockhampton coast line), e1806 (Rockhampton's inland branch). Rail: e2004
    e2005 e1906 e2006 e2107 (Cloncurry-Townsville); e2107 e2206 (Townsville-Cairns); e2107 e2007 e1908 e1808
    (Townsville-Rockhampton) and on south by e1709; e1808 e1807 e1806 (inland branch).
  - Tiles L3510 L3511 L4101 L4210: furniture only (Terrain Effects Chart, Terrain Key, Ceded Lands Box, Weather
    Areas).
  - South-west Australia (tiles L4200 L4201): Geraldton town and Perth city + port (hexes from hexat, below). Rail
    Geraldton-w1523-Perth; Perth east along row 15: w1423 w1424 w1525 w1526 w1527 e1501 e1502 and on east (the
    trans-Australian line, across the seam). Exact hexes (hexat.py): Geraldton w1622 town; Perth w1423 city + port.
  - South and south-east Australia (tiles L4300 L4301; L4211 L4310 L4311 are the map's south margin). Places:
    Adelaide e1304 city + port; Canberra e1308 capital (provisional); Sydney e1309 city + port (green ring); Newcastle
    e1408 port; Brisbane e1609 city + port (under a counter); Melbourne e1107 city + port (under a counter).
    Rail: e1502 e1503 e1504 e1404 e1304 (the trans-Australian line down the gulf into Adelaide); e1304 e1305 e1306
    e1307 e1308 (Adelaide-Canberra); e1308 e1309 (Canberra-Sydney); e1309 e1408 e1509 e1609 (Sydney-Newcastle-
    Brisbane, joining the Rockhampton line by e1709); e1308 e1207 e1107 (Canberra-Melbourne); e1304 e1204 e1105 e1106
    e1107 (Adelaide-Melbourne along the coast). Rivers: the Darling and Murray, drawn along hexsides from e1406 and
    e1306 to the coast by Adelaide (caught in part).
  - New Zealand (tile L4401; L4410 L4411 L4500 L4501 are the south margin, the French Polynesia Box, the Terrain Key
    and the title). Places: Auckland e1517 city + port (green ring, under a counter); Wellington e1318 capital
    (provisional) + port; Christchurch e1217 port. Rail: e1517 e1417 e1318 (Auckland-Wellington); South Island
    e1217 north to e1317 at the Cook Strait (a connected strait arrow joins e1317 and Wellington e1318).
  - Tibet, Szechwan, Yunnan, Burma (tiles L1101 L1111). Places: Lhasa w4715 capital (under a Neutrality marker);
    Ledo w4516 town; Myitkyina w4416 town; Mandalay w4215 city; Lashio w4216 town; Chiengmai w4016 town; Moulmein
    w3916 town; Vientiane w4018 town; Rangoon w4015 capital (provisional, under a counter) + port. Rail: Imphal
    w4415 east to Ledo w4516; Myitkyina w4416 south to Mandalay w4215 by w4316; Mandalay south by w4115 to Rangoon
    w4015; Rangoon east to Moulmein w3916. Road (grey): Ledo w4516-Myitkyina w4416; Mandalay-Lashio w4216; Lashio
    north-east by w4317 w4318 to Kunming w4418 (the Burma Road). Tibet and Szechwan: long mountain ranges and the
    Chinese-country borders, no links.
  - Western Indian Ocean (tiles L1000 L1010 L2000 L2010 L2011; L1000 L2010 are furniture). Places: Addu Atoll w3206
    port; Diego Garcia w2806 port. The zone candidates follow the West Indian Ocean zone borders.
  - READING COMPLETE: every tile of the printed map has been read (land tiles at 1.5x, sea tiles at 1.0x).
  - South China, checked on map-detail.png (no counters): Hengyang w4521 town on the Changsha-Canton rail, with a rail
    branch south-west to Kweilin w4420; rail Kweiyang w4520 south to Nanning w4320 and on to Hanoi w4219; Kweilin-
    Nanning is a grey road; Canton w4322 city + port (the photo's counter hides it; w4322 kept).

### Repair log
- Trace step (scratchpad ds_trace.py, one-off, like tasks/08's border trace): the scan-classified hexsides replace the
  sheet's traced border, zone and mountain edges. After it: border 274 hexsides / 53 pieces (largest 78, 58),
  zone 307 / 104, mountain 80 / 31; network_check 1069 broken rules (the old road and place data untouched yet).
  Tidy on TRC and PGG after the tool changes: "nothing to do", both files byte-identical.
- Land fix (terrain change, stated reason): 103 of the authored place hexes are "sea" in the sheet's sampled terrain
  (none off-map). They are not only the print's pale-blue island hexes: coastal cities such as Bombay w4107, Calcutta
  w4213, Kuala Lumpur w3217, Palembang w2919 and Batavia w2819 are land on the print, and the sampler called their
  mostly-water hexes sea. Every one holds a printed place, and DS places stand on land, so all 103 become clear
  terrain, written by tidy_networks.py as one <hexes terrain="clear"> after the sheet's terrain (SHEETS land list).
  See open question on terrain.
- Land fix, rail hexes (same reason): a dry run found printed rail lines crossing 12 coastal hexes the sampled
  terrain calls sea: w4825 (Kiangsu coast, Tungshan-Nanking), w3716 and w3317 (Kra isthmus and Malaya), w4011 (Bengal
  coast, Calcutta-Kakinada), w1523 (Geraldton-Perth), w3925 (central Luzon, Aparri-Manila), e5405 (Wakkanai),
  e1509 (Newcastle-Brisbane), e1105 and e1106 (Adelaide-Melbourne), e1417 (North Island), e1317 (South Island).
  All 12 added to the land list: 115 land hexes in all.
- Dacca w4314: the printed anchor is in w4314 (hexat 0.62 from its centre) but all six neighbours are land at hex
  scale (Arakan hills w4214, the delta, Calcutta w4213); the print's Bay of Bengal lies inside the hex. Kept as a
  printed port and listed as a delta port (network_check DS_DELTA_PORTS "w4314:sw": no sea neighbour required, glyph
  slot facing the bay). See open questions.
- First real tidy run on dai-senso.xml (2026-09-14): network_check 1069 -> 18 broken rules. Border 304 hexsides /
  13 pieces (largest 238); zone 386 / 35 (largest 150); mountain 44 / 10 (speck pruning of a weak trace; second
  pass pending); rail 218 hexes / 13 pieces; road 40 / 6. Remaining: 9 border ends in open land (Nepal/Tibet,
  Sinkiang, Tannu Tuva, Szechwan, Mongolia, Baikal) and 1 zone end in open sea (w3108), trace gaps to check on the
  photo; 5 printed two-hex links (Palembang-Telukbetung, Taihoku-Tainan, South Island rail; Ledo-Myitkyina and
  Kweilin-Nanning roads); "rail has 8 pieces on one landmass" and a split network: the print has separate rail
  systems joined only by roads (Russia-China, India, Burma, Indochina-Siam-Malaya), and land-fixed coastal hexes
  touch across the Korea and Sunda straits so hex adjacency joins Japan, Java and Sumatra to Asia; Nagasaki has no
  link, though its connected strait to Hiroshima should count (joined() only joins hexes already on links).
- Hex-scale landmasses do not match the print. Listing the land adjacencies that merge separate land components
  shows the land-fixed coastal hexes (and, for Formosa, the base terrain) touching across straits the print shows as
  open water: Malacca (w3216-w3217, w3316-w3317: Malaya-Sumatra), Sunda (w2818-w2819: Sumatra-Java), Korea Strait
  (Fusan e4901 beside Nagasaki e4801 and Hiroshima e4902), Formosa Strait (Taihoku w4325 beside the mainland), Tatar
  Strait (e5805-e5806: Sakhalin-Asia), La Perouse (e5405-e5506: Hokkaido-Sakhalin), Palk Strait (w3608 beside Jaffna
  and Colombo). So "a connected set of land hexes" joins islands the print keeps apart. Decision (reported as a
  deviation): the network and rail rules name the print's separate networks and rail systems explicitly (one hex
  each) instead of deriving landmasses: every piece must hold exactly one named hex, and every capital and city must
  be on the network unless listed unlinked.
- The 10 open boundary ends, each checked on a photo crop with the sheet drawn (scratchpad ends_ds.py): all are trace
  gaps, not printed ends. Tibet/Nepal w4811:sw, w4612:ne, w4613:sw, w4814:ne: the dashes run under dark brown
  ridges, where the classifier missed them; the printed Nepal and Tibet rings are closed. Tannu Tuva w5917:se: gap
  under the TANNU TUVA label. Szechwan w4619:se: the border runs along Chungking's green strategic ring, which hides
  it. Mongolia w5620:ne and w5722:se: two ends of one border about two hexsides apart under the Ulan Ude label and
  rail. Sinkiang w5414:e: the border ends on a white dotted region border (Sinkiang / East Turkestan), not traced yet.
  Zone w3108:e: the printed dashes go on south-west by w3008 towards w2907. Repair: Boundaries.join may now also join
  a loose end to another loose (invalid) end of its own piece, closing a broken ring, and GAP is 4.
- Second tidy run with the named-system rules (DS_NETWORKS, DS_RAIL_SYSTEMS, DS_SHORT filled; connected straits
  join places): network_check 18 -> 8 broken rules; TRC and PGG still 0. Border 311 hexsides / 13 pieces (largest
  245); zone 386 / 35; rail 218 hexes / 13 pieces; road 40 / 6. Left: border ends w4811:sw and w4814:ne (Tibet, gaps
  still longer than the joiner reaches), w4619:se (Chungking ring), w5414:e (waits for the region border trace);
  zone end w3108:e; and three cities alone on their islands with no printed link, Davao w3426 (Mindanao), Rabaul
  e2808 (New Britain), Honolulu e4226 (Oahu), which go on the unlinked list.
- The four open border ends are not spurs: the branch behind each runs 4 (w4811:sw), 8 (w4814:ne), 6 (w4619:se) and 3
  (w5414:e) hexsides back to a junction. They are printed border stretches stopping short of the border they meet
  (Tibet's west border meeting Nepal's ring, the border north of Lhasa, the Szechwan-Yunnan border meeting the
  Chungking ring), and that border is part of the same piece, so the joiner (which refused every vertex of its own
  piece) could not make the T-junction. Boundaries.join now refuses only the end's own branch. Zone gap at w3108,
  read off the crop (scratchpad sideat.py): the printed dashes go on along w3108:se w3007:e w3007:se w2907:e towards
  Diego Garcia; added as explicit data (SHEETS boundary_add).
- Third tidy run (joins may reach the line's own piece outside the end's branch; zone additions): network_check
  8 -> 0 broken rules. Border 318 hexsides / 13 pieces (largest 252); zone 391 / 35 (largest 150); mountain 44 / 10;
  rail 218 hexes / 13 pieces (the 11 named systems plus the two-hex printed pieces); road 40 / 6. Still to do: the
  second trace pass for mountain (thin), region, river and lake, then render and verify on the photo.
  Tidy idempotent on dai-senso.xml ("nothing to do"); validate-xml 4 files valid.
- Second pass, candidates checked on photo overlays (retuned classifier: mountain brown >= 0.15, region dots >= 0.15,
  river/lake split by blue 14 px out): mountain 117 candidates now follow the printed ridges of Tibet and Szechwan,
  including those under border dashes, so they are traced. Rivers on this print meander through hex interiors (the
  Indus crosses whole hexes; the Darling and Murray too), so hexside scoring catches only fragments: rivers need
  authored source-to-mouth guides routed along hexsides, as tasks/08 did for TRC. Region borders (white dots on
  hexsides, e.g. Pakistan) are missed on the game-map scan; a photo sampler (scratchpad sample_photo.py) scores the
  dots on pic4573603.png instead. Lakes: 11 candidates, to check with the photo scores.
- Photo scores (1558 on-map land-land hexsides): region dots at least 0.15 on the hexside and twice the inset share,
  about 70 hexsides, fall where the print has dotted region borders (Pakistan w4607:e w4507:e; Sinkiang / East
  Turkestan w5212:se w5112:se w5414:ne; Bangladesh w4413:se w4313:e w4314:e). Brown ridges form a clear cluster
  (102 hexsides at 0.35 or more). Blue on the hexside: about 136, most with no blue 14 px out (rivers) and about 30
  with it (lake shores). The classifier now takes a mountain, region or water hexside if the scan or the photo shows
  it on the hexside.
- Photo-merged candidates (mountain 148, region 84, river 118, lake 18) checked on overlays: India (the Pakistan and
  Bangladesh dotted region borders caught in part; the Ganges, which the print does draw along hexsides, and the
  Irrawaddy caught well; Nepal/Tibet ridges good); Sinkiang to Manchukuo (region dots in part, Inner Mongolia's
  missed; Tibet/Szechwan ranges good; the Yellow River in part); Baikal (the lake shores and some Mongol Frontier
  dots; Tannu Tuva ranges good); New Guinea (central ranges good). Traced as they are; the repair joins gaps within
  4 hexsides, prunes spurs and specks, and now drops river pieces that still do not reach a coast, a lake or the map
  edge.
- Fourth tidy run, after tracing mountain, region, river and lake: mountain 113 hexsides / 19 pieces (largest 16);
  region 124 / 21 (largest 21); river 262 / 30 (largest 46); lake 18 / 15 (pieces of 3 or fewer: fragments to
  judge); border, zone, rail, road unchanged. network_check: 1 broken rule, a 4-hexside region piece ending inland at
  w4208:ne (central India), to check on the photo.
- w4208:ne on the photo: no region border there; the 4 hexsides w4207:ne w4208:ne w4307:se w4308:se trace the pale
  edge of the dark-green forest round w4309/w4310. Removed as explicit data (SHEETS boundary_remove, applied first by
  the repair).
- First render (hexsheet2svg.py --png --scale 1: 0 warnings; small JPEG copies looked at): India's rail net reads as
  printed; the Pakistan and Bangladesh region dots show. But blue river lines also ring the dark-green forest
  hexes of central India (false positives mixed with the printed rivers), and in the central Pacific the dependent
  borders are incomplete: the Marshall Islands ring only in part, the Gilbert Islands ring barely, the Eastern
  Carolines ring broken round Truk and Ponape. On the game-map scan the naval zone boxes and counters cover those
  dashes; the photo shows them. Next: score border and zone dashes on the photo too and merge, and keep only river
  hexsides both images support.
- River check: of 118 river candidates 87 are supported by both images, 13 by the scan only, 18 by the photo only;
  so the loops round India's forest hexes came from the repair's joins (rivers grew from 118 to 262 hexsides), not
  from the candidates. Rivers now join only within 2 hexsides (LINE_GAP); pieces that still do not drain are dropped.
- Border and zone dashes scored on the photo too (sample_photo.py yellow, red, orange, pale classes) and merged: border
  candidates 274 -> 374, zone 307 -> 354. Photo overlay of the central Pacific: the Eastern Carolines ring (Truk,
  Ponape), the Marshall Islands ring (Eniwetok, Kwajalein, Majuro) and the Gilbert Islands ring (Nauru, Tarawa) are
  now whole, and the zone lines follow the printed dashes from Wake to the Marshalls, from Johnston Island west and
  south, and south of the Marianas. North China showed one false border: Lanchow's red Soviet strategic ring. Printed
  border dashes alternate yellow and red and rings are solid, so a border hexside now needs a yellow share on the line
  too (border_yellow 0.06).
- Fifth tidy run, all six hexside kinds retraced from the merged candidates (border 339, zone 354, region 82, mountain
  148, river 117, lake 18): border 360 hexsides / 12 pieces (largest 239); zone 389 / 35; region 119 / 19; mountain
  113 / 19; river 171 / 35 (joins now short); lake 18 / 15; rail and road unchanged. network_check: 3 broken rules,
  loose ends on land: region w4413:se (by the Bangladesh region border), zone e5703:e (Volochaevsk) and zone w6023:se
  (north end of Lake Baikal), checked on photo crops next.
- Crops: e5703:e and w6023:se are false zone rings drawn round swamp and lake shore on land, where the print has no
  naval zone border; zone candidates now need a sea hex on one side (the classifier's sea-only rule), which removes
  them. w4413:se is a real gap: the Bangladesh region dots run on south past Dacca to the Bay of Bengal; added as data
  (boundary_add region w4313:e w4213:ne w4213:e).
- Sixth tidy run (zone and region retraced): network_check 0 broken rules with every hexside kind present. Border 361
  hexsides / 12 pieces (largest 239); zone 377 / 32 (largest 149); region 129 / 21; mountain 113 / 19; river 171 /
  35; lake 18 / 15; rail 218 hexes / 13 pieces; road 40 / 6.
- Second render (0 warnings), small JPEG crops looked at: India's rail net, the Pakistan and Bangladesh region dots
  and the rivers read like the print (the forest-edge loops are gone); in the central Pacific the Eastern Carolines,
  Marshall Islands and Gilbert Islands rings are closed and the zone lines follow the printed dashes; China, Korea,
  Japan and Manchukuo read right. Seen but not changed: several place-name labels from the original build sit a hex
  or more away from their places (for example VLADIVOSTOK beside Mukden, far from Vladivostok e5302); see open
  questions.
- tidy on dai-senso.xml a second time: "nothing to do". TRC and PGG still 0 broken. validate-xml 4 files valid;
  banner-check 410 files, 0 failures. CMakeLists.txt: hygiene_map_networks now also runs on dai-senso.xml.
- ctest (after checking no other ninja/cl/ctest/cmake process was running): tools\build-dev.cmd win-msvc-debug, then
  CLion's ctest.exe --preset win-msvc-debug in the foreground: 185/185 passed, including hygiene_map_networks on all
  three sheets. No test pinned old Dai Senso sheet data (SheetDocTest checks only its two grids; BoardTest skips link
  kinds the package does not bind), so no test changed. No git writes.
- Zone candidates (pale on-hexside dashes, twice the inset share) follow the printed naval zone borders.
- Sea (x1600-2400, y900-1500): naval zone borders are pale blue-white dashes (not pure white) on sea hexsides,
  running coast to coast and zone to zone (Formosa-Okinawa-Iwo Jima, Formosa-Luzon, round the Philippines).
  Ports seen: Amoy, Tainan (Formosa), Okinawa w4427 (port ring), Iwo Jima e4404, Aparri, Manila (ring), Legaspi,
  Guam. Strategic rings (orange Axis, green Western) sit round many island hexes.
- Automated scoring (scratchpad sample_ds.py / classify_ds.py): borders detected reliably when the on-hexside
  yellow+red share is at least 0.22 and 1.5x the share 6 px inside (strategic rings are inset, so they drop out);
  mountain hexsides by brown on the hexside; rail/road crossings and city discs were NOT reliable (grid lines,
  hex numbers and counters defeat colour thresholds), so links and places are read by eye.
- Sheet at start: traced data is mostly noise (e.g. capitals in a regular column pattern w1004..w4504, zone and
  mountain hexsides scattered in open sea). The glyph lists need re-reading, not tidying.

## Open questions for review (worker fills in)

1. Terrain: 115 hexes changed from sea to clear so that printed places and rail stand on land. The sheet's sampled
   terrain is wrong along many coasts beyond these (mostly-water coastal hexes sampled as sea). Re-sample terrain from
   the print as its own task?
2. Named networks instead of landmasses (see the deviation above): acceptable, or should the sheet gain all-sea strait
   hexsides so landmasses can be derived?
3. Connected straits are not in the sheet (the XSD has a strait symbol): add <edge symbol="strait"> for the printed
   strait arrows (Japan, Philippines, Lesser Sunda Islands, Cook Strait, Hokkaido-Honshu), which would replace
   DS_STRAITS?
4. Place-name labels from the original build are a hex or more off in places (VLADIVOSTOK beside Mukden, others).
   Left unchanged as outside this task; re-place them from the authored place hexes?
5. Rivers and lakes come from hexside scoring of meandering printed rivers: where the print runs a river through hex
   interiors the sheet carries only the hexsides it touches. Good enough, or author source-to-mouth river guides as
   tasks/08 did for TRC? Lakes are 18 hexsides in small pieces (Baikal's shore and similar).
6. Places under counters on both images were placed from map-detail.png topology and nearby glyphs: Canton w4322,
   Taihoku w4325 (rail only; not placed as a city), Nanking w4725, Shanghai w4726, Manila w3824, Bombay w4107. Some ports
   under counters were not placed (Saipan, Palau, Truk, Kwajalein, Iwo Jima, Okinawa, Balikpapan, Dairen, Medan,
   Tutuila) because no anchor could be seen; confirm from a counter-free copy of the East Map.
7. Rail versus road on the print was judged by line style (white cased with ticks versus grey). The DS rules price them
   alike; a few lines under counters (Kweisui road, Formosa rail) are uncertain.
8. The strategic hex rings, the dark red map-edge symbols down the West Map edge, and oil / lend-lease / US impact
   symbols were left as they were.

## Brief for the worker (paste as the agent prompt, prefixed by the standard W-role preamble)
You are worker W5. Read this task file first, then tasks/08-map-networks.md, and stay within the inputs. Ben wants
the Dai Senso sheet cleaned up so its network features are continuous and match the printed map: roads connected
on each landmass and across the seam between the two map halves, borders and weather/zone lines as unbroken
chains, mountain hexsides as ranges, and every city, capital and port where the map shows it. Look carefully at
the images, through small crops only: make a cropped or downscaled JPEG of at most about 300 KB with PyMuPDF
(`fitz`) and Read that; never Read an image over about 1 MB, because large images have dropped earlier workers'
connections. Calibrate each image to the sheet (the sheet's pixel frame follows "Dai Senso game map adjusted.png";
map-detail.png is offset by one row and column in its labels), then compare feature by feature, a region at a time,
recording findings in this file as you go so a relaunch loses nothing. Extend the TRC/PGG tools with per-sheet
rules rather than writing new ones, repair the XML (by tool where the change is systematic, by explicit data where
it must match the printed map), regenerate SVG and PNG, and look at the result. House style as in CLAUDE.md
(copyright line first and last in every .py and .md). Update the resume line after every step. Finish with
status: review, before/after counts, and open questions.

Copyright Ben Paul Wise. All Rights Reserved.
