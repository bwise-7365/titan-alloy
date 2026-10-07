Copyright Ben Paul Wise. All Rights Reserved.

# Circling Dragons: the map sheet and how it was made

Written 2026-10-05, with the first sheet `map_graphics/xml/circling-dragons.xml`. This is a designed map,
not a scan: no suitable map of China at this scale existed, so the sheet was built from open geographic
data on a perfect lattice and then ruled by hand against the design memorandum
(`circling_dragons_map_and_rules_V3.tex`; V2 is the previous version). The builder and its cached data are in
`map_graphics/xml/tools/cd/` (see its README for a rebuild).

## 1. The lattice

- Flat-top hexes, one grid, 35 columns by 34 rows, 1190 hexes, no holes, odd columns half a hex down.
  Ids are `{col:02}{row:02}` from the north-west corner: 0101 is top left, 3534 bottom right.
- One hex is 100 km between centres in every direction. The projection is an Albers equal-area conic
  (standard parallels 27 N and 45 N, origin 35 N 118 E), so every hex covers 8660 square kilometres
  wherever it lies, and the column pitch is 86.6 km on the sheet.
- Extent: 20.5 N to about 50.5 N, 102 E to about 135.5 E. Kunming sits two hexes from the west edge,
  Hanoi one row from the south edge, Khabarovsk on the east edge, Hailar and Heihe on the north edge.
  Hainan and Burma are off the map; the Burma Road leaves the west edge at Kunming.
- Of the 1190 hexes 315 are sea, 188 clear, 259 broken, 189 mountain, 219 steppe and 20 marsh. The
  south-east sea holds the furniture, as Downfall and Dai Senso do.
- The sheet is 2558 by 2869 px; the hex circumradius is 46 px.

The memorandum asked how many hexes high and wide a 100 km map would be. The answer is 35 wide and
34 high, and about two fifths of that is sea or Mongolian steppe that costs nothing to play on.

## 2. Terrain: one value per hex

Classes are the memorandum's: clear, broken, mountain, marsh/floodplain, arid/steppe, sea.

1. Land or sea came from Natural Earth 1:50M land polygons rasterised onto the lattice; a hex is land
   if half or more of it is. Six port hexes the rasteriser called sea (Shanghai, Ningbo, Dalian,
   Chefoo, Shanhaiguan, Pusan) are land by ruling.
2. Each land hex was sampled at 19 points of the ETOPO1 relief model. Mountain means a median above
   1800 m, or a relief (standard deviation of the samples) above 400 m, or a range above 1400 m with a
   mean above 500 m. Broken means a relief above 130 m or a mean above 700 m. The rest is clear.
3. Steppe is a hand polygon: the Inner Mongolian plateau north of the Yinshan and Great Wall line,
   Hulunbuir, the Horqin sands and the Ordos. Mountain wins over steppe (the Greater Khingan).
4. Marsh is a hand polygon or a lake: the Sanjiang plain (Amur, Ussuri and Sungari floodplain), the
   Nen and Sungari lowland west of Harbin, and the Yellow River flood zone of 1938-47 south-east of
   Kaifeng (seven hexes). Tai Hu, Hongze and Khanka are marsh hexes with a lake mark.
5. Named ranges were then forced as explicit hex lists, because the data makes the long low ranges
   "broken" and the memorandum wants them legible as barriers: Taihang (1513-1517, 1713), Yan Shan
   (1811, 1912, 2011), Luliang (1314-1316), Qinling and Funiu (row 19, 0719-1419), Daba and Wu Shan
   (0721-1222), Dabie (1722, 1822), Xuefeng (1225, 1226, 1127, 1128), Nanling (1228, 1329-1629),
   Wuyi (2026, 2126, 1927, 1827, 1828), Greater Khingan (2301-2206, 2107, 2108, 2008, 2009), Lesser
   Khingan (2602-2902, 3003), Changbai (2811, 2911, 2910, 3009, 3008, 3010), Zhangguangcai (3005-3007).
   Three lesser ranges are forced broken: Wanda, the Shandong hills, the Luoxiao.
6. Place rulings last: a city on the plain is clear even where its hex also holds hills (Peiping's
   Western Hills), the plateau cities are broken (Taiyuan, Kalgan, Datong, Yan'an, Lanzhou, Guiyang,
   Kunming, Chongqing), and Tongguan stays a mountain hex because it is the pass.

## 3. Rivers: the operational barriers only

Rivers run along hexsides. Each chosen river's Natural Earth centreline was densified, snapped to the
nearest lattice vertex, joined by the cheapest hexside path, and cut of loops. Only the rivers the
memorandum names are on the sheet: Yangtze, Yellow River, Han and Xiang in China; Amur, Argun, Ussuri,
Sungari, Nen and Liao in Manchuria; the Yalu and Tumen as Korea's border. Every other river is terrain.

Each river is written as `path` chains with ids, and every confluence is a `junction` at a hex corner
named in ABC: the six corners of a hex are h+A, h+B, h+C, h-A, h-B, h-C, and on a flat-top sheet A
points due east, B to the north-west corner and C to the south-west corner (hexcoord `Grid.cpp`,
`basisOf`), so `1423:-B` is the south-east corner of hex 1423. The five junctions: Xiang into the
Yangtze (1423:-B), Han into the Yangtze (1423:A), Nen and Second Sungari forming the Sungari (2706:-B),
Sungari into the Amur (3202:-B), Ussuri into the Amur (3401:-B). The Yalu and Tumen touch at their
sources on Paektu without joining, so that vertex has no junction. Path ends carry a reason: edge, sea,
junction or source.

Why paths and not edges (the PGG form): a bag of river edges tells the rules where water is but not
which hexsides are the Yangtze, and the memorandum's river transport between Yangtze and Sungari ports
needs exactly that; and only chains can carry junctions. Until 2026-10-06 the two renderers rounded
river edges but drew path chains straight, so HexMapEd showed path rivers without its waviness. Both
were taught on that day to draw a river path chain as they draw river edge chains: `hexsheet2svg.py`
(`layer_edges`) and hexview (`MapLineLayers.cpp`, `addPaths`: rounded in the reference style,
scratched when the editor's style asks). The SVG golden test now includes this sheet, and hexview's
drawing of it equals the reference element for element; two MapSceneBuilder tests pin the rounded and
the scratched river paths. `RIVER_FORM` in `build_cd.py` can still write the edge form.

Chains are cut over all rivers together, so a tributary arriving mid-chain cuts the main river; a piece
of a main river that begins where a tributary ends and runs to a fork is relabelled the tributary (the
Xiang's last hexside, the Nen below Qiqihar, which the data calls Songhua). Two ends the data left
short were extended by the builder: the Yalu to the sea below Antung, the Yellow River to the west edge.

Two decisions here need Ben's eye:

- The Yellow River is drawn on its 1938-47 course: from the Huayuankou breach south-east through
  eastern Henan to the Huai, Hongze Lake and the sea. That is the river of 1944-46; the Shandong
  channel was dry. The dry channel is not drawn.
- The Yangtze estuary below Zhenjiang is drawn by hand (the centreline data stops there).

The Yangtze and Sungari give strategic transport between river ports; the ports carry a dark-blue
anchor mark (Chongqing, Wanxian, Yichang, Wuhan, Jiujiang, Anqing, Wuhu, Nanjing; Harbin, Jiamusi).
The transport rule lives in the rules, not in a drawn link.

## 4. Networks: only lines that change a decision

Railways (black, ticked): Pinghan, Yuehan, Hunan-Guangxi and its Guizhou extension to Dushan,
Longhai, Jinpu, Peiping-Suiyuan, Tongpu, Shijiazhuang-Taiyuan, Jinan-Qingdao, Peiping-Mukden with the
Jehol loop through Chengde, Chifeng-Tongliao-Siping, the South Manchuria lines, the Chinese Eastern
(Manzhouli to Suifenhe), Harbin-Beian-Heihe, the Jiamusi lines, Changchun-Kirin-Tumen, Mukden-Antung,
the Baicheng-Solun Khingan line, the lower Yangtze and Zhejiang-Jiangxi lines, Canton-Kowloon, the
metre-gauge Yunnan-Indochina line, the Korean trunk and east-coast lines, and the Soviet feeders
(Borzya-Manzhouli, Khabarovsk-Vladivostok-Suifenhe). The railway reaches neither Chongqing nor
Yan'an; that is a fact of 1944 and the network checker's profile excepts those two capitals.

Strategic roads (brown, dashed): the Burma Road out of Kunming, Kunming-Guiyang, Guiyang-Chongqing,
Chongqing-Chengdu, Guiyang-Dushan, Hengyang-Baoqing-Chihchiang-Guiyang (the 1945 Chihchiang axis),
Chengdu-Hanzhong-Baoji over the Qinling, Xi'an-Yan'an, Xi'an-Lanzhou, Liuzhou-Nanning-Lang Son,
Canton-Wuzhou-Liuzhou (the second Ichi-Go pincer), Wanxian-Enshi-Yichang, Laohekou-Nanyang-Luoyang,
Kalgan-Dolonnor. Nothing else is printed.

Lines are straight hex chains between places, so a railway crossing a mountain hex is the pass (the
Khingan tunnel at Boketu, Niangziguan at Taiyuan, the Nanling at Lingling and Chenzhou). Junctions
are explicit: every hex two rail links share is a junction, likewise for roads.

## 5. Places

Operational nodes from the memorandum's table plus what the networks need. Major cities (red square):
Peiping, Tientsin, Wuhan, Shanghai, Nanjing, Canton, Chongqing, Mukden, Harbin. Capitals (star):
Chongqing, Yan'an, Nanjing, Changchun. Airfield triangles: Hengyang, Lingling, Guilin, Liuzhou,
Nanning, Chihchiang, Laohekou, Chengdu, Kunming, Ganzhou. Ports carry an anchor toward the sea.

Four places were moved one hex from where the projection puts them, to keep corridors and ranges
apart at 100 km: Taiyuan into the Fen valley column between the Luliang and the Taihang, Hanzhong
into the Han valley row between the Qinling and the Daba, Baoqing east of the Xuefeng wall, Kaifeng
east of Zhengzhou (the two cities share a hex otherwise). Qinhuangdao is folded into Shanhaiguan,
Huludao into Jinzhou, Port Arthur into Dalian, Sinuiju into Antung, Blagoveshchensk is off the map
behind Heihe.

## 5a. Mongolia, the Soviet entry hexes and the furniture (2026-10-06)

Outer Mongolia saw no fighting in 1944-45; eastern Mongolia was the Trans-Baikal Front's staging ground.
The sheet records that side of the war with a legend mark, the red star "Soviet entry hex (August
1945)", on Manzhouli (36th Army), Tamsag Bulag (the main thrust over the Greater Khingan), Sain Shand
(Pliyev's Soviet-Mongolian cavalry-mechanized group, 450 km across the Gobi to Dolonnor and Kalgan,
reaching them about 19-21 August), Heihe, Tongjiang and Khabarovsk (2nd Far Eastern Front over the Amur
and up the Sungari), Iman, Suifenhe and Tumen (1st Far Eastern Front). Choibalsan is on the sheet with
the Borzya-Choibalsan railway of 1939, the railhead for the western wing; beyond it supply moved by
truck. Mongolia's own part in the war was political: Yalta recognised its status, and the Sino-Soviet
treaty of 14 August 1945 made Chiang accept the independence plebiscite of 20 October 1945. That belongs
on the Pacific War track as an event, not on the map.

The furniture sits in the corners the war left quiet, as Downfall lays its tracks: the Pacific War track
and the Trans-Baikal staging boxes over western Mongolia, the Far Eastern staging boxes over the Sea of
Japan, the Hump and Ledo Road box over Laos, and the title, key, time track, legitimacy tracks, Japanese
pools and the two remaining off-map boxes in the East China Sea, the column resting on the sheet's foot.

The rules memorandum (`circling_dragons_map_and_rules_V2.tex`, section "Current best rules") now carries
these as "Mongolia and the Soviet western wing" (sanctuary, entry and axes, steppe supply, Mengjiang and
the Suiyuan corridor, Soviet occupation of Chahar and Jehol, the withdrawal track) and "The Sino-Soviet
treaty" (a priced KMT decision on the Pacific War track); added 2026-10-06.

## 5b. The Mongolian operations and the political layer (2026-10-06)

Built first as trial versions 2 and 3 beside the base sheet, to judge whether more Mongolian detail helps
without pulling the eye from the Mao-Chiang struggle. Ben chose version 3, which is now the standard
`circling-dragons.xml` (`build_cd.py`; `build_cd.py 1` and `2` still write the earlier forms as `-v1` and
`-v2` files for comparison, not kept in the repository). The political layer is under active development:
its elements are placed, the rules behind them (memorandum, "The political layer") are provisional.

Version 2, the Mongolian operations:
- Passes as a white diamond mark: the Boketu rail tunnel, the Arshaan gap on the Solun line, the Lubei
  trail the 6th Guards Tank Army took, and the Kalgan gap through the Yinshan.
- Gobi tracks as a dotted brown "track" link kind, held to the road rule in the checker: the Kalgan-Urga
  road through Erenhot and Sain Shand to Ulan Bator (new town), the Erenhot-Sonid-Dolonnor trail, the
  Tamsag Bulag-Lubei trail over the Khingan to Tongliao, and the Choibalsan-Tamsag supply road.
- A "desert" terrain (waterless Gobi and Alashan, duller sand) split out of the steppe by hand polygons,
  with water points as small blue discs at Erenhot, Sonid and Lubei.
- Kwantung Army fortified zones as a small black square: Hailar, Aihui (Heihe), Sunwu, Arshaan, Hutou,
  Dongning.
- Soviet airfields (the airfield triangle) at Tamsag Bulag, Matad and Choibalsan, and a "Fuel airlift"
  box in the Trans-Baikal staging panel.

Version 3 adds the political layer:
- Mengjiang as a light wash over Chahar and Suiyuan with its name; the Inner Mongolian autonomy
  declaration at Sonid as a green triangle.
- The Tonghua redoubt: the Changbai hexes along the Korean border outlined in dashed brown, Tonghua as
  a town.
- The weather divide along the Qinling and the Huai as a sparse dotted blue line between the winter zone
  and the monsoon zone, with a small weather table in the Mongolian corner.
- The speculative Soviet liaison route from Yan'an across the Ordos to Erenhot as a faint dotted red line.

All of it lives in hexes already on the map. Mark, line, link and panel ids share one id space in a sheet
(xs:ID), so colour ids must differ from mark and line ids; the builder names them apart.

## 6. The memorandum's four empirical checks, read off the sheet

1. Ichi-Go at 100 km per hex: Zhengzhou to Wuhan 5 hexes, Wuhan to Changsha 3, Changsha to Hengyang 2,
   Hengyang to Guilin 3, Guilin to Liuzhou 1, Liuzhou to Nanning 3; 17 hexes from Zhengzhou to Nanning
   along the corridor. A campaign, not a move.
2. Manchuria: Manzhouli to Harbin 9 hexes with the Greater Khingan wall at columns 22-23; Suifenhe to
   Harbin 4 hexes with the Zhangguangcai between; Heihe to Harbin 4 hexes over the Lesser Khingan;
   Tamsag Bulag to Mukden 8 hexes. The three Soviet directions are legible.
3. CCP presence: the North China plain between the Pinghan and Jinpu lines is two to three hexes wide,
   with the Taihang base area one column west; a marker carpet is avoidable.
4. The surrender race: Shanhaiguan to Mukden 4 hexes, Peiping to Mukden 7, Chongqing to Wuhan 9 by
   river. Whether that is enough time is a rules question, not a map one.

## 7. Doubts and decisions for Ben

1. Panel children: the reference renderer draws a panel's texts, boxes, tracks and tables in document
   order, hexview kind by kind (texts, then boxes, tracks, tables). The older sheets group them by
   kind, so the difference never showed; this sheet's builder groups them too. A sheet that interleaves
   them would draw in two orders. Not fixed; recorded here.
   The Han's confluence sits one hex west of Wuhan (1423:A), where the snapped chains meet.
   `network_check.py` reads path chains as hexside lines; the C++ loader reads path junctions and, as
   its comment says, ignores them (a confluence is not modelled in hexside terrain).
2. The Nanling is mountain on the sheet; the memorandum calls it "broken terrain". The Luliang and
   Yan Shan are forced mountain though the memorandum does not name them. Easy to change in
   `cd_data.RANGES`.
3. Chengde's hex is mountain (the Yan Shan runs through it), so the Jehol railway crosses two mountain
   hexes. Chifeng and Dolonnor are broken.
4. The Yunnan-Indochina railway is drawn whole though it was cut at the border from 1940; rail state
   (Open, Interdicted, Broken) is play state, so the start scenario should mark it.
5. Korea, the Soviet Far East, the Kyushu corner (Nagasaki) and Formosa are on the sheet because the
   rectangle holds them. The rules can treat them as off-map boxes; the hexes exist for a later game.
6. Dongting and Poyang have no hex of their own: they share hexes with Yueyang and Jiujiang. Only Tai
   Hu, Hongze and Khanka carry a lake mark.
7. The Yellow River flood zone is seven marsh hexes (1719, 1819, 1720, 1820, 1920, 2020, 2120); the
   true flooded area was about 54,000 square kilometres, six hexes.
8. Four rail pairs share one step (Mukden-Liaoyang, Changchun-Siping, Baicheng-Solun, Khabarovsk line
   near Vladivostok); drawn twice, harmless.
9. The Manchukuo boundary is a hand polygon along the Great Wall and the Jehol-Chahar line; the
   region tint is light. Mengjiang is not drawn.
10. Furniture is a placeholder laid out from the rules concept: a 24-month initiative track, a
    Pacific War track ending in Soviet entry, surrender and endgame, KMT and CCP legitimacy tracks,
    four shared Japanese pools, four off-map boxes. None of it is rules.
11. Elevation came through a public API (opentopodata.org, ETOPO1). The samples are cached in
    `elev_hex.json`, so the build runs offline.

## 7a. The logo as SVG (2026-10-06)

`circling-dragons-logo.svg` is a trace of the simplified (right-hand) panel of the AI-made composite
`file_0000000093a481f58355b3278556e667.png`, cropped to `circling-dragons-logo.png` (930 x 796). The
colours were cut to twelve: each dragon a dark, a middle and a light shade (the light shade is mostly the
edge fringe) with the white highlights, and six neutrals for the background, the hills in their greys,
the Great Wall tower, the pagoda, the bridge, the map of China and the text. Every silhouette is the
raster's own edge, traced by potrace into smooth curves; nothing was redrawn. `circling-dragons-logo-flat.svg`
is the same drawing with each colour traced once and a half-pixel same-colour stroke closing the seams,
half the size (0.47 MB against 1.0 MB), indistinguishable at the bridge and the dragons' heads. The tracer
is `map_graphics/xml/tools/cd/vectorize_logo.py`.

The sheet language can now carry a picture (approved 2026-10-06): hexsheet.xsd's Panel gained an `image`
child, a file named relative to the sheet and drawn scaled to fit its box, presentation only. Both
renderers draw it: `hexsheet2svg.py` writes an SVG image, hexview an ImageShape (SVG and JSON writers, hit
test), HexMapEd paints it through Qt6::Svg and writes it back on save; the golden normaliser compares
images by their four corners and href; a fixture sheet in MapSceneBuilderTest exercises it. The logo was
placed in a panel at the foot of the south-east column and then taken out again the same day (Ben: two
logos on one map is too much; the dragons belong on the box). The south-east column of panels now sits
at the foot of the sheet. The logo files stay in this folder.

## 8. Next steps toward an AI

- A `hexrules` document for the current rules concept (orders, rail state, supply by network,
  presence markers) and a `hexpackage` binding it to this sheet; the engine then has a board.
- A start scenario (spring 1944) in `hexsave`: Japanese-held rail and cities, KMT war areas, CCP base
  areas, rail segment states.
- The network checker profile `circling-dragons` in `map_graphics/xml/tools/network_check.py` is the
  gate for any map edit; `tools/validate-xml.py` the other.


## 9. Unit density: the Downfall benchmark (2026-10-06)

Ben asked how many unit pieces Mao, Japan and Chiang should have, judged by Downfall's pieces per hex
played. Measured from the copy in `C:\Library\War-Games\Downfall` (rulebook; the OKH and Western setup
charts, which are exact; the InsideGMT preview picture of the November 1942 setup, from which the Soviet
and OKW counts are read by eye and may be off by two or three).

- Downfall: 110 km hexes, flat-top, printed only where there is land in play; a unit of 3-4 steps is an
  army, 2 a corps, 1 a division. On the 2500-px map image the lattice is circumradius 28.2 px (found by
  `calibrate.py`; the first run locked on a double-size alias). Counting lattice cells whose interior is
  at least a quarter land, with the tracks, boxes and panels masked, gives 771 in-play land hexes
  (827 at 15%, 636 at half). Call it 800.
- Land pieces on the map at the start: OKH 30 (16 German including 4 security, 14 minor-ally), OKW about
  30 (German, Italian, coastal detachments), Soviet about 22, Western 7 (four British armies, three
  detachments): about 89, one piece per 9 hexes. Later arrivals: OKH 10, Western 12, Soviet from the
  pool; the board peaks near 105-110, one per 7-8 hexes, most of it on the two fronts.
- CD has 672 in-play land hexes (China 505, Manchukuo 140, Korea 27) and 131 Soviet-only Mongolian hexes.
  Downfall's start density gives 70-75 pieces. Proposal: Japan 38 (China Expeditionary Army 24 line pieces,
  one per division or pair of independent brigades, plus 6 one-step garrison pieces on the railways;
  Kwantung Army 6; Korea 2); KMT 22 (20 group-army pieces of 3-4 steps, 2 US-equipped Alpha Force pieces,
  2 more returning from Burma in 1945); CCP 12 field pieces (6 Eighth Route Army regions, 5 New Fourth
  Army, 1 South China), growing toward 18 by concentration of presence; 30 presence markers, which are
  not unit pieces (Downfall's analogue is ten partisans a side); Soviets 9 scripted groupings from
  August 1945. Start total 72, one per 9.3 hexes; 81 with the Soviets.
- The CD piece is therefore Downfall's 2-step corps-level unit, not its 4-step army: at army scale Japan
  would be 12 pieces and the KMT's twelve war areas 12, one piece per 28 hexes, too thin for a 35 x 34
  sheet.
- Counter budget: about 80 land pieces, 30 presence markers, 4 initiative markers, some 20 rail-state and
  30 control markers and the track markers, about 200 pieces against Downfall's 351.


## 10. Counter set and illustrations (2026-10-06)

Ben accepted the budget of about 200 pieces as preliminary and asked for the sheets, a section of the
memorandum for each of the four major actions, and two illustrations for each. Built:

- `unit_graphics/xml/circling-dragons.xml` in the hexcounters language, generated by
  `unit_graphics/xml/tools/build_cd.py`: 164 records, 226 counters on two 12 x 11 sheets at 5/8 inch,
  rendered by `counters2svg.py` to `circling-dragons-sheet-1|2-front|back|both.svg|png`. Styles: japan
  (China Expeditionary Army, orange), kwantung (ochre), nationalist (blue with a white symbol box; a grey
  box marks regional troops, a gold box US-equipped pieces), communist (red, white box; P in the box is a
  presence marker), puppet, manchukuo, mengjiang, soviet (brown), usa (green), plain. Value lines are
  strength-movement placeholders; backs are the reduced faces.
- `map_graphics/xml/tools/cd/illustrate.py`: eight pictures in `Circling Dragons/illustrations/` (svg and
  png), each a crop of the standard sheet's reference render with counters drawn through the counter
  renderer, plus arrows, battle bursts and notes; the data are tables per picture (stacks by hex id,
  arrow paths, bursts, notes). The SVGs (about 850 KB each) are regenerable; the memorandum includes the
  PNGs.
- The memorandum gained a cover page with Ben's logo (`circling-dragons-logo-V2.svg`, converted to PDF by
  Inkscape), the subsection "Piece scale and counts (preliminary)", and the section "The four major
  actions" with the eight figures; 23 pages.

Doubts: the illustrations' order of battle is approximate where the set has no piece (the 13th and 52nd
Armies of the sealift are shown by 9th War Area pieces, the Peiping airlift by 5th and 6th War Area
pieces); Hutou and Iman, and Heihe and Sunwu, share hexes on the sheet, so the Soviet star and the
Japanese fortified zone coincide there.


## 11. The 75 km trial sheet and the maneuver studies (2026-10-06)

Ben asked for a trial sheet at 75 km beside the standard one, and for several figures each of Xue Yue's
defence of Changsha and the envelopment of Hengyang on it. Done by Opus 5.5 from the Fable handoff
(`2026-10-06-handoff-75km.md`).

- The scale is a parameter: `CD_HEX_KM=75` for every stage and the builder (tools/cd/README.md, "Another
  scale"). The north-west corner and the pixel size of a hex stay; the lattice is 47 x 46 = 2,162 hexes,
  1,187 in play against 672. Caches carry a `-75` suffix. The standard sheet is byte-identical after the
  changes (checked after each change).
- Rulings: the named ranges and the redoubt map from their 100 km hex lists (every 75 km hex whose centre
  falls in a listed hex). The five 100 km place pins are not used at 75 km (`PLACE_HEX_BY_SCALE`). Three
  coastal hexes just under half land were ruled land so the railways stay on land (`LAND_HEXES_BY_SCALE`:
  3117 Liaodong spine, 3131 Hangzhou Bay south shore, 3617 Korean west coast).
- Builder: an exit line now walks to the map edge (`exit_path`; one step at 100 km, two at 75 for the
  Hanoi-Saigon line and the Burma Road), and a river may be extended to join another river
  (`EXTEND_TO["ussuri"] = ("amur", Khabarovsk)`: at 75 km the Ussuri stopped two hexsides short of the
  Amur). The network checker's no-rail capitals (Chongqing, Yan'an) are keyed by sheet id.
- Terrain at 75 km comes out in the same proportions (of the hexes in play: clear 27 per cent against 26,
  broken 32 against 33, mountain 23 against 24, steppe 15, marsh 3), so the thresholds were not retuned.
- Measured: the clear belt across the corridor is two hexes at 100 km and three at 75 km on both the
  Changsha and the Hengyang parallels; Yueyang to Changsha is one hex and two.
- The study `maneuver_studies_75km.pdf`: the trial sheet, conventions, Changsha (five figures: the
  position, the advance, the flank groups strike, the withdrawal, 1944 on three columns), Hengyang (four:
  the position, the ring closes, siege and relief, the fall), the rules the figures assume (ZOC that blocks
  supply, withdrawal before combat, fortified cities, air supply, out of supply, stacking two), and
  conclusions. All rules and values provisional.

Doubts and open points:
- The minor rivers north of Changsha (Xinqiang, Miluo, Laodao) are drawn in the figures only, not printed
  on the sheet; if the Changsha defence is wanted, they may deserve a "minor river" hexside class.
- The group-army pieces stand in for the armies of 1941 and 1944 (the counter set uses 1944 numbers).
- Choosing between the scales is Ben's design decision; the study's last section states the trade.


## 12. 75 km becomes the standard; minor rivers; 250 counters; the studies in the rules (2026-10-06)

Ben liked the 75 km sheet and the maneuver studies and asked for: about 250 counters; 75 km as the default;
the coastal and Ussuri rulings kept; a minor-river class used by the XML and the SVG; the studies in the
rules, replacing the 100 km versions.

- Default scale 75 km (`lattice.DEFAULT_KM`). The standard sheet is `circling-dragons.xml`, 47 x 46, id
  `circling-dragons`; `CD_HEX_KM=100` builds the first sheet as `circling-dragons-100.xml` (byte-identical to
  the old standard except its id and title). Caches: plain names are 75 km, `-100` suffix the first sheet.
  The trial files `circling-dragons-75.*` are gone.
- Yueyang: the snapped Yangtze ran round the south of the Yueyang hex, so the Yuehan railway crossed it twice
  between Hankow and Changsha. Ruled (`RIVER_EDITS_BY_SCALE`): the river passes north of the hex, the Xiang
  comes up its west side to meet it; a Puqi waypoint keeps the railway south of the river.
- Minor rivers (`MINOR_RIVERS_BY_SCALE`): the Xinqiang (1932:s 1933:ne 2032:s) and the Miluo (1933:s 1934:ne
  2033:s), each joining the Xiang at a junction; written as river paths with the line `minor-river` (riverblue,
  width 3.5), so both renderers round them and HexMapEd scratches them; network_check has a minor-river rule
  (on land, joins a river); labelled; named in the key panel. The Laodao lies inside the Changsha hex.
- Counters: 250 (126 units: Japan 51, KMT 35, CCP 23, Soviets 11, puppets 6; 5 air support; 119 markers).
  Two-state markers are double-sided (presence / base area, KMT / CCP control, rail interdicted / broken,
  port denied / open, airfield captured / destroyed, a Japanese formation's front).
- Figures: all seventeen on the standard sheet. The action figures' tables stay in 100 km ids and are
  converted (a place's hex to the same place's hex); their notes are a layer of their own. Old ids of the
  merged markers are aliased in `illustrate.py` (`ALIAS`).
- Rules memorandum: 75 km in the design conclusions, map extent, prototype list and the first open question
  (now one of tempo: fifteen hexes from Hankow to Dushan); the minor rivers; "The standard sheet at 75 km";
  the piece table for 250 counters; section 9, "Two maneuver studies: the Hunan corridor", with a seventh
  provisional rule for minor rivers. The sheet and studies text is shared with the standalone study through
  `Circling Dragons/sections/*.tex`, so the two documents cannot drift apart. 37 pages.

Doubts: the minor-river movement cost and the major-river cost are not yet set anywhere (the rule says only
"less than a major river"); the US-equipped labels on the 13, 52 and 94 Armies are provisional.


## 13. Memorandum version 3: contents and locator maps (2026-10-06)

Ben asked, to help players get oriented, for a version 3 of the memorandum with a clickable table of contents
and a full-page map, with the discussed area boxed in black, right before each of the four major actions and
the two maneuver studies.

- `circling_dragons_map_and_rules_V3.tex` / `.pdf` (46 pages), made from V2, which is kept unchanged as the
  previous version. The date line says version 3. Contents on its own page (set small to fit one page); with
  hyperref every entry links to its section, and the PDF has numbered, opened bookmarks.
- Six locator maps (Ichi-Go, the reflux, August 1945, the race, Changsha, Hengyang), each a float page of its
  own between two page breaks, so it falls directly before its subsection. `sections/locator.tex` defines
  `\locator`; the map is set once in a box, so the PDF carries one copy of the image (the six maps added
  about 24 KB). The boxes are written by `illustrate.py` into `sections/locator-boxes.tex` from the same crops
  as the figures (`illustrate.py --boxes` writes only that file), so they follow any change of crop.
- The two maneuver-study locators are in the shared `sections/maneuver-studies.tex`, so the standalone study
  has them too (17 pages).
- The sheet's `source` attribute now gives the scale from the build (it said 100 km after the switch) and
  cites the V3 memorandum; the SVG is unchanged by it.

Doubt: a contents link lands on the subsection's heading, one page after its locator map.


## 14. Colors, style and American spelling (2026-10-06)

Ben added page breaks to V3 (kept), and asked for the link and heading colors of
`C:/repos/ghub-per/CPVI/shared-tex/prolog-article.tex`, the CPVI `style-instructions.md` applied to the text of the
rules, and American spelling.

- Colors: the prolog's `myBlue1` (RGB 75, 99, 175) for internal links, sections, subsections and subsubsections
  (sectsty), cyan for URLs, `myTeal` for citations; `hidelinks` removed. Only the memorandum has them; the
  standalone study keeps its plain look.
- Style: the design conclusions and the design rules are statements of the design, not orders to the reader;
  metaphor and personification removed ("rules that bite" became "rules that apply"; "skeleton", "phase
  transition", "earn their place", "hollow", "marker carpet", "payoff", "war machine" and others replaced);
  rhetorical questions became statements; "crucial" and "genuine" removed; the Glantz attribution became a
  citation and the FRUS volume is cited where the text relies on it; fragments in the "Rules that apply"
  paragraphs became sentences; terms made consistent (marsh, markers, railways). Ben's own new sentences were
  given the same treatment, and "Chiang Kai-Shek" became "Chiang Kai-shek".
- American spelling in the TeX, the figure text (notes, titles: "Changsha: the counterattack"), the counter
  set ("Coastal defense") and the map (title panel "centers", the source attribute); closed compass compounds
  (northwest, southeast). Dates stay in the day-month-year form of the military histories.


## 15. Copyright notice on the sheet (2026-10-06)

Ben asked for "Copyright Ben Paul Wise" in black on the orange margin in the lower-left and upper-right corners.
The builder writes two labels (palette entry `copyright`, #000000, size 26), each centered in its 60 px margin band
and flush with the left or right edge of the hex area, so they appear at every scale. hexview draws them the same
as the reference renderer (golden test passes). The figures were not redrawn; their crops do not reach the
corners, and the memorandum's full-map pages show the notice on the next compile.


## 16. Six test cases from the figures (2026-10-06)

Ben asked for a written record of the actions behind the four major-action figures and the two maneuver
studies, arranged as six test cases for a future rule set: `circling-dragons-test-cases.md` in this folder.
For each case it lists the starting position, each piece's moves in 75 km hex ids, the attacks and their
results, supply, stacking, the final position, the results a rule set must reproduce (items T) and the
points where the figures, the provisional rules and the sheet disagree (items D). The positions were read
from `illustrate.py` and `maneuvers.py`, the terrain and rivers from the sheet, the values from the counter
set; the hex routes, distances and supply paths were computed, not read off the pictures.

Findings that need Ben's ruling (the sheet and the figures are unchanged):
- The Yellow River chain ran along 1824:ne, 1924:s and 1924:se, so Luoyang was on the north bank and the
  Longhai crossed the river between Zhengzhou and Luoyang (D1.1). Resolved on 7 October (section 17).
- The Hengyang ring is open at Lingling 1737 under the ZOC rule as written, because both Japanese neighbors of
  1737 are across the Xiang; the city traces supply along the Hunan-Guangxi railway to Guilin (D3.1).
- The Changsha east wing is out of supply only under a local-path limit of eight hexes or fewer (D2.3).
- The withdrawal rule's trigger (an enemy moving adjacent) does not cover the screens' withdrawals when
  attacked (D2.1); a 3-step piece has no 2-step face.
- Wuhan holds three divisions in two figures; the Ichi-Go play figure uses four airfield markers, the set has
  three.
- The August setup puts the Sunwu zone in 3703, not on the sheet's Sunwu mark (3702), and leaves out the Aihui
  zone at Heihe 3701 (D5.1, D5.2).
- Qinhuangdao and Shanhaiguan share 2917, which the CCP holds when the KMT lands there (D6.1); the Qingdao
  Marines marker starts on land (D6.2).

The open rulings are collected, with options and recommendations, in `circling-dragons-rulings-needed.md`.


## 17. The Yellow River at Luoyang (2026-10-07)

Ben ruled that the Yellow River pass north of Luoyang, as the Yangtze passes north of Yueyang. The snapped
river ran round the south of the Sanmenxia hex 1724 and the Luoyang hex 1924 and the north of 1824, so the
Longhai railway crossed it four times between Tongguan and Zhengzhou, although Sanmenxia, Luoyang and the
railway are all on the south bank. The ruling (`RIVER_EDITS_BY_SCALE[75]["yellow"]` in `cd_data.py`) drops
1624:ne 1724:s 1724:se 1824:ne 1924:s 1924:se and adds the north sides of 1724 and 1924; it was carried one
hex west of Luoyang, to 1724, because the same railway crossed there (rulings list, item A1).

- The Longhai now crosses the Yellow River only on the 1938 course between Zhengzhou and Kaifeng (2024:ne);
  the Pinghan crosses at the Zhengzhou bridge (2024:ne); the Tongpu crosses from Shanxi into 1724 (1724:ne),
  where it crossed into Tongguan before. The Yuncheng basin, which shares hex 1724, counts as south bank.
- Rebuilt: `rivers.json` (only the six Yellow River hexsides changed), `terrain_hex.json` (unchanged), the
  sheet XML (only the Yellow River path changed), its SVG and PNG. The sheets validate, the network check
  reports 0 broken rules, and the hexview golden, frame and scene-builder tests pass.
- Figures: all seventeen redrawn. The PNGs of the six figures that show the area (Ichi-Go, the reflux, the
  race) changed; the maneuver PNGs are identical (their SVGs change because each embeds the whole sheet).
  The August crops reached the top margin, where the copyright notice of section 15 showed as a one-pixel
  sliver; they now start 6 px lower (`NE_PAD_TOP` in `illustrate.py`), which changes the two August PNGs
  and moves the August locator box by 0.0016 of the sheet's height.
- The memorandum and the standalone study say so (`sections/sheet-75km.tex`) and were recompiled (47 and 17
  pages).


## 18. Shared style file and the HexKrieg-style rules proposal (2026-10-07)

Ben asked to compare the test cases and the rulings list with two HexKrieg documents
(`C:\repos\ghub-per\HexKrieg\doc\hexkrieg_system_rules.tex` and `supply_model_land_sea_draft.tex`), to find the
smallest revisions of the HexKrieg movement, combat and supply rules that fit Circling Dragons, and to write
them up as a new document. He ruled that Circling Dragons keeps its initiative system and that the HexKrieg
turns and phases are dropped.

- `sections/cd-style.tex`: the packages, colors, headings and footer of the memorandum's preamble, now shared.
  The memorandum inputs it and sets its own pdftitle and `\myversion`; its text is unchanged (the extracted
  text of the 47-page PDF is identical). The standalone maneuver study keeps its own preamble.
- `circling_dragons_hexkrieg_rules.tex` (12 pages, draft v01): sources, a comparison table, fourteen revisions
  (M1 to M14) with reasons, the proposed rules (activations; terrain and edge costs; pieces with offense,
  defense and steps; movement with the HexKrieg stop-on-entry zones and the R1 exceptions; combat by declared
  attacks with return fire, withdrawal before combat, modifiers, a combat table shifted one row toward the
  defender, results and advance; graded supply traced along the network with attrition at level 0 and air
  supply at level 1; recovery), checks against the six test cases, the rulings it settles, and open points.
- Main findings: the four structural differences are the sequence, who fights, what carries supply, and the
  effect of zones of control. Under the proposal most test-case events hold; the attackers' step losses at
  Changsha (a4) and Hengyang (b4) do not, because the return fire of a weak or air-supplied defender is small.
  The recommendation of rulings item B4 changed from "zones affect supply only" to the HexKrieg stop rule.


## 19. Assault losses and supply pieces (2026-10-07, proposal v02)

Ben asked for the attacker-loss clause and asked whether explicit supply pieces with a limited reach along the
network would change Circling Dragons much; he intends to use them and to set their reach by play testing.

- Assault losses (rule 4.5): after the results, if an attacked fortified city or Kwantung fortified zone is
  still held, each piece that attacked it loses a step on a roll of 8 to 10. It covers the zones as well
  because the rules treat them as fortified cities. Changsha a4 (`jp-d34`) and Hengyang b3/b4 now hold, the
  latter over the two general assaults of July (51 percent per division).
- Supply pieces (rule 4.6): the HexKrieg draft's supply units kept, with their reach counted along the
  network. A source covers the network within the reach R at level 3; a supply piece on a covered hex relays a
  further R at one level less; a piece off the network takes the lower of that coverage and the level its local
  path allows (3, 5 and 7 points). Placeholders: R = 6; Japanese 6 pieces in China and 2 in Manchuria and Korea,
  KMT 5, none for the Soviets (the Mongolian rules stay) or the CCP (base areas are sources).
- Analysis: with R longer than any distance on the sheet the rule is the v01 trace, so supply pieces generalize
  it. At R = 6 they change little at Changsha, in August and in the race; they slow and thin deep advances: the
  Wuhan source reaches Zhuzhou, one piece carries level 2 to Hengyang, Lingling and Guilin, Liuzhou needs a
  second piece (level 1) or the route from Canton (level 2). That gives Ichi-Go a tempo mechanism (the
  memorandum's open question), lowers the Hengyang ring to level 2 (a longer siege), and can make the Dushan
  halt a supply effect. Costs: about 13 counters, more orders, a new target for raids.
- Version v03 (Ben's request): a one-page unnumbered section "To the play testers" before section 1 says that
  the rules will change and that reports and suggestions are welcome, and lists the sixteen choices under
  play test (P1 to P16: supply 7, combat 6, movement 3) with the alternatives considered. Each rule under test
  carries its tag, e.g., [P1], linked to the list (macro `\ptest`). The open-points section now points to it.
- Version v04 (Ben's request): a three-page unnumbered "Introduction" for play testers new to the game,
  before the play-tester page; the main content is unchanged and the section numbers stay. Subsections: what
  the game is about (the double contest; the four operations), two players and four factions (crossed control,
  directives, shared pools, Legitimacy; the US and the Soviets), time and initiative, the map, the pieces,
  movement, combat and supply in outline, the course of a game, how the game is won, this document and play
  testing. 17 pages in all.

Copyright Ben Paul Wise. All Rights Reserved.
