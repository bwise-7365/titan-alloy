Copyright Ben Paul Wise. All Rights Reserved.

# Circling Dragons: six test cases from the memorandum's figures

This document records the actions behind the seventeen figures of the rules memorandum, version 3
(`circling_dragons_map_and_rules_V3.tex`, sections "The four major actions" and "Two maneuver studies: the
Hunan corridor"). It arranges them as six test cases for a future rule set: Ichi-Go, Changsha, Hengyang, the
reflux of 1945, August 1945, and the race. For each case it gives the starting position, every movement of
every piece, which pieces attack which, the results (step losses, retreats, eliminations), supply, stacking,
the final position, the results a rule set must reproduce, and the points on which the figures, the
provisional rules and the map disagree.

The figures were read from their scripts, `map_graphics/xml/tools/cd/illustrate.py` and `maneuvers.py`, on 6
October 2026. Terrain, rivers and railways were read from the standard sheet,
`map_graphics/xml/circling-dragons.xml` (75 km between hex centers), and piece values from the counter set,
`unit_graphics/xml/circling-dragons.xml`. The decisions these cases leave open are collected, with options
and recommendations, in `circling-dragons-rulings-needed.md`.

**Hex ids.** The four-digit ids printed on the sheet: two digits of column, then two of row, counted from
0101 at the northwest corner. The hexes are flat-topped, and the even columns stand half a hex lower than the odd ones.
The neighbors of hex (c, r) are (c, r-1) to the north and (c, r+1) to the south. To the northeast and
southeast they are (c+1, r) and (c+1, r+1) when c is even, and (c+1, r-1) and (c+1, r) when c is odd; to
the northwest and southwest, the same rows in column c-1. For example, 1933 touches 1932, 2032, 2033,
1934, 1833 and 1832, but not 2034.

**Hexsides.** A hexside is named by a hex and a direction (n, ne, se, s, sw, nw), e.g., 1932:s, the side
between 1932 and 1933. The sheet names each side once, so a side may appear under either of its hexes;
1833:ne is the side between 1833 and 1933. Major rivers (Yangtze, Yellow, Han, Xiang, Amur, Argun, Ussuri,
Sungari, Nen, Liao, Yalu, Tumen) and the two minor rivers (Xinqiang, Miluo) run along hexsides. Terrain is
one value per hex: clear, broken, mountain, marsh, steppe, desert or sea.

**Pieces.** Pieces are named by their counter ids: `jp-d40` Japanese 40th Division, `jp-tk3` 3rd Tank
Division, `jp-g-*` Japanese railway garrison, `kw-*` Kwantung Army, `kr-34a` Korea Army, `pup-*` puppet army,
`kmt-ga27` KMT 27th Group Army, `kmt-n6a` and `kmt-94a` US-equipped armies, `kmt-hq*` headquarters, `ccp-*`
CCP field forces and columns, `sov-*` Soviet groupings, `us-*` US air support and Marines. A value "3-4" is
strength 3, movement 4. Steps are the dots on the piece; a piece that has lost a step shows its back face.
Air-support pieces (`jp-air`, `us-14af`, `us-cacw`) and markers do not count for stacking. Headquarters are
unit pieces and do.

**Drawn and inferred.** "Drawn" means the figure shows it: a piece, an arrow, a numbered move, a battle burst
or a note. "Inferred" means it follows from comparing two figures and is not drawn. In the maneuver studies,
moves and attacks carry the numbers of the figure's discs. Where a figure shows a piece at the start and not
at the end, the case lists it as "not shown" and does not guess its fate.

**Provisional rules.** The maneuver studies assume seven rules (memorandum, "Rules the maneuvers assume"),
cited here as R1 to R7:

- R1 Zones of control (ZOC). A formation's ZOC covers the six hexes around it, except across a major river
  or into a mountain hex. A supply path may not enter a hex in an enemy ZOC unless a friendly formation
  occupies it. R1 gives ZOC no effect on movement.
- R2 Minor rivers. A minor-river hexside costs one extra movement point to cross, less than a major river,
  and does not block ZOC.
- R3 Withdrawal before combat. A formation in broken or mountain terrain, or behind a minor river, may
  withdraw one hex when an enemy formation moves adjacent, at the cost of one initiative space.
- R4 Fortified cities. Attacks on a fortified city are halved, and the defender may convert one step loss
  into a retreat refused. The marker is removed when the city falls.
- R5 Air supply. A fortified city within range of a friendly air-support piece is in limited supply even
  when surrounded: it does not lose steps for lack of supply, but it may not attack.
- R6 Out of supply. A formation out of supply attacks at half strength, and loses a step if it is still out
  of supply at the end of its next activation.
- R7 Stacking. Two unit pieces may stand in a hex, with any number of markers and one air-support piece.

The four major actions also use rules from the memorandum's section "Current best rules", cited by
subsection title, e.g., "Supply and combat" (a conventional formation traces a short local path to an open
railway, strategic road or river route, then along that network to a source; mountain, marsh and arid
terrain sharply restrict off-network supply).

**How the figures were built.** The maneuver studies (figures a1 to a5 and b1 to b4) were written in 75 km
hex ids in `maneuvers.py`, one entry per numbered move or attack, so the moves listed for cases 2 and 3 are
the moves drawn. The four major actions were first drawn on the 100 km sheet, and their tables are still
written in 100 km ids. `illustrate.py` converts each old id to the 75 km hex of the most important place the
old hex held, or else to the 75 km hex under the old hex's center. When two old hexes fall into one new hex,
a per-figure override moves one of them; where a figure lacks the override, an arrow can start in the wrong
hex. Arrows are straight lines between the converted waypoints; the hexes listed for an arrow are the hexes
its line crosses, and a case says so where that line is not a legal route. The action figures summarize
months of play: they show where pieces stand at the end, with arrows for the main movements, and not each
activation.

**Movement costs.** The provisional rules give no movement costs. The maneuver figures fit this table: clear
1, broken 1, mountain 2, a minor river 1 extra, a major river at least 1 extra. If broken terrain costs 2,
four of the drawn moves exceed the mover's allowance: Changsha a2 move 3 and a4 move 1 and Hengyang b2 move 5
(allowance 4), and Hengyang b3 move 4 (allowance 3).

**Questions common to several cases.**

- Step faces. A 3-step piece has one reduced face, its 1-step back. The figures flip a piece to that face
  when the caption says it lost one step, so the face shows two steps lost. The counter set has no face for
  a 3-step piece with two steps left (cases 2 and 3).
- R3's trigger. R3 lets a formation withdraw when an enemy moves adjacent. Several drawn withdrawals happen
  when an enemy that was already adjacent attacks (cases 2 and 3).
- ZOC and movement. R1 defines ZOC for supply only. Several drawn moves pass from one enemy ZOC hex to
  another; they stay legal only while ZOC has no effect on movement (cases 2 and 3).
- Stacking at Wuhan. Two of the action figures put three divisions in Wuhan 2131 (cases 1 and 4).

## 1. Ichi-Go and the Hunan corridor, 1944

This section holds three cases. Case 1 is the whole of Ichi-Go as the two action figures show it. Cases 2
and 3 are the maneuver studies, which show move by move what case 1 shows as one arrow: the advance from
Yueyang through Changsha to Hengyang. Case 2 also covers Xue Yue's defense of Changsha in 1941-42, the model
for the defender's maneuver.

### Test case 1: Ichi-Go, April to December 1944

#### Figures and claim

Figures `1-ichigo-setup` ("Ichi-Go: the opening position, April 1944") and `1-ichigo-play` ("Ichi-Go: one way
it plays, April to December 1944"); memorandum, "Ichi-Go, April to December 1944". Title-box markers: on the
setup, `jp-dir-ichigo1` (directive Ichi-Go I, Henan); on the play figure, `jp-dir-ichigo3` (Ichi-Go III,
Guangxi) and `jp-pool-ops` (the shared operational-support pool).

The CCP player commands the Japanese pieces facing the KMT and conducts Ichi-Go. The KMT player commands the
North China garrisons facing the CCP. In nine months three directives in turn open the railway corridor
from the Yellow River to Guangxi and take the Fourteenth Air Force's fields; Hengyang holds 47 days; the KMT
armies withdraw west; Alpha Force forms; CCP presence spreads behind the departing Japanese.

#### Rules exercised

- The Yellow River rail bridge: marker `mk-bridge-yellow` in Zhengzhou 2024. The Pinghan railway crosses
  the river at hexside 2024:ne.
- Rail redeployment along the corridor; directives Ichi-Go I, II and III; the shared Japanese pools.
- R4 at Hengyang; airfield markers (`mk-airfield`, captured or destroyed face).
- Crossed control; the Legitimacy cost of the Henan collapse (a placeholder in the memorandum).
- Formation of Alpha Force and the airlift of the New 6th Army.
- CCP presence spreading into hexes the Japanese leave.

#### Initial position, April 1944

Japanese divisions are 3-4 with 3 steps, `jp-tk3` 4-6 with 3 steps, garrisons 1-2 with 1 step.

| Hex | Place, terrain | Pieces |
|---|---|---|
| 2124 | Xinxiang/Kaifeng, clear | `jp-d110`, `jp-d37` |
| 2224 | clear | `jp-d62`, `jp-tk3` |
| 2024 | Zhengzhou, clear | `mk-bridge-yellow` |
| 2219 | Shijiazhuang, clear | `jp-g-pinghan` |
| 2522 | Jinan, clear | `jp-g-jinpu` |
| 2525 | Xuzhou, clear | `jp-d65`, `jp-g-longhai` |
| 2131 | Wuhan, clear | `jp-d3`, `jp-d68`, `jp-d116`, `jp-air` |
| 1730 | Yichang, clear | `jp-d13`, `jp-d39` |
| 1932 | Yueyang, clear | `jp-d34`, `jp-d40` |
| 2332 | Jiujiang, clear | `jp-d58` |
| 2128 | Xinyang, clear | `jp-d27` |
| 3229 | Shanghai, clear | `jp-d60` (outside the figure's crop) |
| 3031 | Hangzhou, clear | `jp-d70` (outside the crop) |
| 1942 | Canton, clear | `jp-d104`, `jp-d22` |

KMT regular group armies are 2-3 with 3 steps; the Guangxi and Guangdong regional group armies (`ga16`,
`ga35`, `ga12`) 1-3 with 2 steps; headquarters 0-4.

| Hex | Place, terrain | Pieces |
|---|---|---|
| 2125 | marsh | `kmt-ga15`, `kmt-ga31` |
| 1924 | Luoyang, clear | `kmt-ga19`, `kmt-hq1` |
| 1927 | Nanyang, clear | `kmt-ga28` |
| 1728 | Laohekou, clear | `kmt-ga22`, `kmt-ga33` |
| 1430 | broken | `kmt-ga10` |
| 1733 | Changde, clear | `kmt-ga26` |
| 1934 | Changsha, clear | `kmt-ga24`, `kmt-hq9` |
| 1836 | Hengyang, clear | `kmt-ga27`, `kmt-fort`, `us-14af` |
| 1935 | Zhuzhou, clear | `kmt-ga30` |
| 1538 | Guilin, broken | `kmt-ga16`, `us-14af` |
| 1339 | Liuzhou, clear | `kmt-ga35`, `us-cacw` |
| 2039 | Shaoguan, broken | `kmt-ga12` |
| 2734 | broken | `kmt-ga23` |
| 1031 | Chongqing, broken | `kmt-hqalpha` |

CCP field forces are 2-4 with 2 steps.

| Hex | Place, terrain | Pieces |
|---|---|---|
| 2021 | mountain | `ccp-jjly`, base area (`ccp-presence`, back face) |
| 2029 | clear | `ccp-n4a5`, base area |
| 2427 | marsh | `ccp-n4a4` |
| 2628 | clear | `ccp-n4a2` |
| 3026 | clear | `ccp-n4a1` (outside the crop) |
| 2727 | marsh | `ccp-n4a3` |
| 2722 | broken | `ccp-sd`, base area |
| 2222 (Anyang), 2321, 2123, 2424, 2526, 2826, 2019, 2328 | | `ccp-presence`, one in each |

#### Sequence

Phase I, Henan, April and May (directive Ichi-Go I):

1. `jp-d110` 2124 → 2024 → Luoyang 1924 (drawn arrow, "I. Henan, April-May: the river crossed, Luoyang
   besieged"). Two hexes. It crosses the Yellow River once, at the bridge (2024:ne); Zhengzhou and Luoyang
   are both on the south bank (see D1.1). Battle at Luoyang (burst; "Luoyang, May").
2. `jp-d62` and `jp-tk3` 2224 → Zhengzhou 2024 (inferred; no arrow). Two hexes, crossing the Yellow River
   once: through 2124 at 2024:ne, or through 2125 at 2125:ne.
3. `jp-d37` 2124 → 2125 (drawn arrow 2124 → 2125 → 2127, through 2126). It crosses the Yellow River at
   2124:s into the marsh hex held by `kmt-ga15` and `kmt-ga31`, and ends there. The arrow continues south
   along the Pinghan railway to 2127, toward `jp-d27` at Xinyang 2128; no piece is drawn at 2127.
4. KMT in Henan (inferred). `kmt-ga15` leaves 2125 and ends at Tongguan 1624, five hexes west. `kmt-hq1`
   leaves Luoyang for Tongguan (three hexes) and stacks with it. `kmt-ga19` leaves Luoyang for Nanyang 1927
   (three hexes) and stacks with `kmt-ga28`. `kmt-ga31` is not shown at the end. The memorandum's frame
   says the 12th Army destroyed Tang Enbo's group armies; the figure draws no step losses on the KMT pieces
   that remain. `kmt-ga22` and `kmt-ga33` stay at Laohekou 1728.

Phase II, Hunan, May to August (directive Ichi-Go II):

5. The 11th Army moves down the Yuehan railway (drawn arrow 2131 → 1932 → 1934 → 1836, "II. Hunan,
   May-August: Changsha falls, Hengyang holds 47 days"). The rail route is 2131 2132 2032 1932 1933 1934
   1935 1835 1836, eight hexes. It crosses the Xinqiang (1932:s), the Miluo (1933:s) and the Xiang twice
   (1934:s, 1835:s). The arrow, drawn straight from Wuhan to Yueyang, passes through 2031 and crosses the
   Yangtze twice (2031:ne, 1932:ne); the railway does not cross it.
   - `jp-d34` Yueyang 1932 → Changsha 1934 (two hexes).
   - `jp-d68` and `jp-d116` Wuhan 2131 → Hengyang 1836 (seven hexes in a straight line, eight by rail).
   - Cases 2 and 3 give this phase move by move.
6. KMT in Hunan. `kmt-ga24` withdraws from Changsha to Changde 1733 (drawn arrow 1934 → 1833 → 1733,
   crossing the Xiang at 1833:se) and stacks with `kmt-ga26`. `kmt-hq9` ends at Baoqing 1635 (inferred,
   three hexes from Changsha). `kmt-ga27` withdraws from Hengyang to Baoqing (drawn arrow 1836 → 1736 →
   1635, crossing the Xiang at 1736:se) and stacks with `kmt-hq9`. `kmt-ga30` ends in 1536, a mountain hex
   (inferred, four hexes from Zhuzhou). A burst marks the siege of Hengyang; the airfield marker there shows
   its destroyed face, and the fortified-city marker is not shown.

Phase III, Guangxi, September to November (directive Ichi-Go III):

7. Drawn arrow 1836 → 1737 → 1538 → 1339 along the Hunan-Guangxi railway (1836 1737 1637 1538 1438 1339,
   five hexes, no river; "III. Guangxi, Sept-Nov: Guilin and Liuzhou").
   - `jp-d3` Wuhan 2131 → Lingling 1737 (eight hexes); airfield marker on its destroyed face.
   - `jp-d58` Jiujiang 2332 → Guilin 1538 (ten hexes); airfield marker on its captured face. Burst at Guilin.
   - `jp-d13` Yichang 1730 → Liuzhou 1339 (eleven hexes); airfield marker on its captured face.
8. KMT in Guangxi. `kmt-ga16` withdraws from Guilin to 1139 (drawn arrow, four hexes; its straight line
   passes through Liuzhou 1339). `kmt-ga35` leaves Liuzhou for 1139 (inferred, two hexes) and stacks with
   `kmt-ga16`. `us-cacw` is not shown at the end.
9. The 23rd Army: `jp-d22` Canton 1942 → Nanning 1142 (drawn arrow 1942 → 1741 → 1142, eight hexes; "23
   Army to Nanning"). `jp-d104` is not shown at the end.
10. December: `jp-d40` Yueyang 1932 → Dushan 1137 (nine hexes; drawn arrow 1339 → 1238 → 1137 through 1138;
    "December: Dushan, then the halt"). The arrow passes 1238 and 1138, both adjacent to the KMT stack at
    1139. Burst at Dushan.

Elsewhere:

11. `jp-d39` Yichang 1730 → Wuhan 2131 (inferred, four hexes); `jp-air` stays at Wuhan. Not shown at the end:
    `jp-d27`, `jp-d65`, `jp-g-longhai`, and `jp-d60` and `jp-d70` (both outside the crop).
12. The North China garrisons (KMT player). `jp-g-jinpu` at Jinan 2522 and `jp-g-pinghan` at Shijiazhuang
    2219 each receive a `jp-security` marker (security zone, blockhouse line). Figure note: "the KMT player raids
    the bases with the North China garrisons while the shared pools are drawn down". No base is removed.
13. Alpha Force. `kmt-ga11` (Y-Force, 3-3, 3 steps), new in the play figure, appears at Kunming 0337 and
    moves to Guiyang 0936 (drawn arrow, six hexes along the Kunming-Guiyang road). `kmt-hqalpha` moves from Chongqing to Guiyang
    (inferred, five hexes) and stacks with it.
14. `kmt-n6a` (New 6th Army, 4-4, 3 steps) is flown from Kunming 0337 to Chihchiang 1434 (dashed US arrow,
    eleven hexes). One `us-14af` piece ends there; the other is not shown at the end. Figure note: "Alpha Force
    forms; New 6 Army flown back from Burma".
15. CCP presence spreads along the Pinghan as the garrisons thin. Drawn arrow 2021 → 2124 (three hexes):
    presence in 2124, the hex `jp-d110` and `jp-d37` left. Drawn arrow 2722 → 2421 (three hexes; its
    straight line passes through Jinan 2522, held by `jp-g-jinpu`): presence in 2421. Presence stays at
    Anyang 2222 and `ccp-jjly` with its base at 2021. Not shown at the end: `ccp-sd`, `ccp-n4a1` to
    `ccp-n4a5`, and the presence markers at 2321, 2123, 2424, 2526, 2826, 2019 and 2328.

#### Supply

The figures make no explicit supply claim; the halt at Dushan is the one point where supply could decide the
outcome. At the end, `jp-d40` at Dushan 1137 traces a local path of three hexes to Liuzhou 1339 (1137 1237 1338 1339), which the Japanese hold and which the
Hunan-Guangxi railway joins to Hengyang. The path avoids the ZOC of the KMT stack at 1139. Under R1,
`jp-d40` is in supply, with or without a restriction on mountain and marsh hexes. The halt must therefore
come from the directive, the shared pools or the time track, not from supply. The siege of Hengyang depends
on R4 and R5 (case 3).

#### Stacking and counters

- Setup: Wuhan 2131 holds three divisions and `jp-air`, one division over R7. The note "11 Army at Wuhan:
  nine divisions" refers to the whole army: Wuhan 3, Yichang 2, Yueyang 2, Jiujiang 1, Xinyang 1.
- Play figure: every hex is within R7.
- The play figure uses four airfield markers (1836, 1737, 1538, 1339); the counter set has three.
- The two `us-14af` pieces of the setup agree with the counter set, which has two.

#### Final position, December 1944

| Side | Hex | Pieces |
|---|---|---|
| Japanese | 1924 Luoyang | `jp-d110` |
| | 2024 Zhengzhou | `jp-d62`, `jp-tk3` |
| | 2125 | `jp-d37` |
| | 1934 Changsha | `jp-d34` |
| | 1836 Hengyang | `jp-d68`, `jp-d116`, airfield (destroyed) |
| | 1737 Lingling | `jp-d3`, airfield (destroyed) |
| | 1538 Guilin | `jp-d58`, airfield (captured) |
| | 1339 Liuzhou | `jp-d13`, airfield (captured) |
| | 1137 Dushan | `jp-d40` |
| | 1142 Nanning | `jp-d22` |
| | 2131 Wuhan | `jp-d39`, `jp-air` |
| | 2522 Jinan; 2219 Shijiazhuang | `jp-g-jinpu`, `jp-security`; `jp-g-pinghan`, `jp-security` |
| KMT | 1927 Nanyang | `kmt-ga19`, `kmt-ga28` |
| | 1624 Tongguan | `kmt-ga15`, `kmt-hq1` |
| | 1728 Laohekou | `kmt-ga22`, `kmt-ga33` |
| | 1733 Changde | `kmt-ga26`, `kmt-ga24` |
| | 1635 Baoqing | `kmt-ga27`, `kmt-hq9` |
| | 1536 | `kmt-ga30` |
| | 1139 | `kmt-ga16`, `kmt-ga35` |
| | 0936 Guiyang | `kmt-ga11`, `kmt-hqalpha` |
| | 1434 Chihchiang | `kmt-n6a`, `us-14af` |
| CCP | 2021 | `ccp-jjly`, base area |
| | 2124, 2222, 2421 | presence |

#### Results a rule set must reproduce

- T1.1 The 12th Army crosses the Yellow River at the bridge (2024:ne) and takes Luoyang in May.
- T1.2 The KMT pieces in Henan are driven from 2125 and Luoyang to Tongguan 1624 and Nanyang 1927;
  `kmt-ga31` is eliminated or leaves the area (to be decided).
- T1.3 The 11th Army moves eight hexes by rail from Wuhan to Hengyang across two minor rivers and the Xiang
  twice; Changsha falls by 18 June, and Hengyang holds from 23 June to 8 August (case 3).
- T1.4 By November the Japanese hold Lingling, Guilin and Liuzhou; the fields at Hengyang and Lingling are
  destroyed, those at Guilin and Liuzhou captured.
- T1.5 `jp-d22` reaches Nanning, eight hexes from Canton, by 24 November.
- T1.6 `jp-d40` reaches Dushan by 2 December and halts there while in supply (D1.4).
- T1.7 `kmt-n6a` can be flown eleven hexes from Kunming to Chihchiang; `kmt-ga11` and `kmt-hqalpha` reach
  Guiyang.
- T1.8 The CCP player can place presence in a hex the Japanese have left (2124) and beside a garrison
  (2421), while the KMT player uses the garrisons against the bases.
- T1.9 The whole runs from April to early December, about eight calendar intervals.

#### Discrepancies to resolve

- D1.1 Yellow River at Luoyang (resolved 7 October 2026). The snapped river ran along the south sides of
  the Sanmenxia hex 1724 and the Luoyang hex 1924 and the north side of 1824, so the Longhai railway crossed
  it four times between Tongguan and Zhengzhou, and `jp-d110` crossed it twice. Ben ruled that the river
  pass north of Luoyang, as at Yueyang; the ruling (`RIVER_EDITS_BY_SCALE`) also moves it north of 1724,
  where the same railway crossed. The Longhai now crosses the Yellow River only on the 1938 course between
  Zhengzhou and Kaifeng (2024:ne); the Tongpu railway crosses it from Shanxi into 1724 (1724:ne).
- D1.2 The unlabeled Japanese arrow starts at 2124, the hex of `jp-d110` and `jp-d37`. In the 100 km table
  it started at the hex of `jp-d62` and `jp-tk3`. The setup figure moves that hex to 2224 by a per-figure
  override, which the play figure lacks.
- D1.3 The phase II arrow passes through 2031 and crosses the Yangtze twice; the railway it represents does
  not.
- D1.4 The halt at Dushan is not a supply effect under R1 (see Supply).
- D1.5 The Hengyang garrison. Here `kmt-ga27` defends Hengyang and withdraws to Baoqing. In case 3 the
  garrison is `kmt-ga10`, which surrenders, and `kmt-ga27` is the relief at Baoqing. The 10th Army
  surrendered on 8 August 1944.
- D1.6 `kmt-ga11` is named "Y-Force (returns 1945)" in the counter set but stands at Guiyang in December 1944.
- D1.7 Four airfield markers are used; the counter set has three.
- D1.8 Pieces present at the start and not shown at the end: `kmt-ga31`, `kmt-ga10`, `kmt-ga12`,
  `kmt-ga23`, `us-cacw`, one `us-14af`, `jp-d27`, `jp-d104`, `jp-d65`, `jp-g-longhai`, `jp-d60`, `jp-d70`,
  `ccp-sd`, `ccp-n4a1` to `ccp-n4a5`, and seven presence markers. A complete test needs an end position for
  each.

### Test case 2: Changsha, 1941-42 and 1944

#### Figures and claim

Figures `a1-changsha-position`, `a2-changsha-advance`, `a3-changsha-furnace` ("Changsha: the
counterattack"), `a4-changsha-withdrawal` and `a5-changsha-1944`; memorandum, "Xue Yue's defense of
Changsha". The figures show the area from 1630 to 2336. The title box of a1 carries `kmt-hq9`, that of a5
`jp-dir-ichigo2`.

1941-42 (a1 to a4): screens on the river lines step aside to the flanks instead of falling back on the
city; the main body waits in the hills off the axis; the garrison holds the fortified city. When the
attacker's center reaches the city with its east wing spread, the flank groups strike into the lane behind
the wing and cut its supply. The attacker withdraws two steps weaker, and the position is restored.

1944 (a5): eight divisions on three columns fill the axis and both lanes; the city falls on 18 June, and
nothing remains to close in behind the center.

The pieces stand in for the armies of the text: the KMT group armies for the 9th War Area's armies, and in
1941-42 `jp-d34`, `jp-d3` and `jp-d40` for the 11th Army's divisions of that battle.

#### Terrain of the area

- The Yuehan railway runs 2131 Wuhan, 2132, 2032, 1932 Yueyang, 1933, 1934 Changsha, 1935 Zhuzhou. The
  Zhejiang-Jiangxi railway runs east from Zhuzhou through 2034 and 2134.
- The Xinqiang runs along 1932:s, 1933:ne and 2032:s. The Miluo runs along 1933:s, 1934:ne and 2033:s. The
  Xiang runs west and south of the city along 1832:ne, 1832:se, 1833:ne, 1833:se, 1834:ne and 1934:s.
- The axis 1932, 1933, 1934 is clear, as are 2032, 2034 and the west bank 1832, 1833, 1834. The hills east of
  the railway (2132, 2033, 2133, 2134, 2233) are broken.
- 2034 touches 2033, 2134, 2135, 2035, Zhuzhou 1935 (across the Xiang) and Changsha 1934, but not 1933.
  2033 touches 2032, 2133, 2134, 2034, 1934 and 1933.

#### Initial position (a1)

| Side | Hex | Pieces | Role |
|---|---|---|---|
| Japanese | 1932 Yueyang | `jp-d34`, `jp-d3` (3-4, 3 steps), `jp-air` | center, at the railhead |
| | 2032 | `jp-d40` (3-4, 3 steps) | east wing |
| KMT | 1933 | `kmt-ga30` (2-3, 3 steps) | screen behind the Xinqiang |
| | 2033 (broken) | `kmt-ga26` | screen behind the Xinqiang |
| | 2133 (broken) | `kmt-ga24` | main body |
| | 2134 (broken) | `kmt-ga27`, `kmt-hq9` | main body and the war area's headquarters |
| | 1833 | `kmt-ga22` | west group, across the Xiang |
| | 1934 Changsha | `kmt-ga10`, `kmt-fort` | garrison |

Each screen has the Miluo at its back. All KMT group armies are 2-3 with 3 steps.

#### Sequence

a2, the advance:

1. (move 1) The center, `jp-d34` and `jp-d3` (3 + 3), attacks `kmt-ga30` in 1933 across the Xinqiang
   (burst on 1932:s).
2. (move 2) `kmt-ga30` withdraws before combat, one hex southwest across the Xiang (1833:ne) to 1833, and stacks
   with `kmt-ga22`. It does not fall back on the city. The center advances into 1933 (clear 1, minor river
   1). `jp-air` stays at Yueyang.
3. (move 3) `jp-d40` attacks `kmt-ga26` in 2033 across the Xinqiang (burst on 2032:s).
4. (move 4) `kmt-ga26` withdraws before combat, one hex to 2133, and stacks with `kmt-ga24`. `jp-d40`
   advances into 2033 and continues across the Miluo (2033:s) to 2034: broken 1, clear 1, two minor rivers,
   4 points in all. The figure numbers this withdrawal 4, after `jp-d40`'s move 3, but `jp-d40` cannot pass
   through 2033 until `kmt-ga26` has left it.

End of a2: Japanese 1932 `jp-air`; 1933 `jp-d34`, `jp-d3`; 2034 `jp-d40`. KMT 1833 `ga22`, `ga30`; 2133
`ga24`, `ga26`; 2134 `ga27`, `hq9`; 1934 `ga10`, fort. No losses.

a3, the counterattack:

1. (attack 1) `jp-d34` and `jp-d3` (3 + 3) attack `kmt-ga10` (2) in Changsha from 1933 across the Miluo
   (1933:s). R4 halves the attack: 3 against 2. The attack is repulsed; no loss is drawn on either side (for
   `jp-d34`, see a4).
2. (move 2) `kmt-ga26` moves from 2133 back into 2033, the lane behind the east wing. 2033 touches both the
   center (1933) and the east wing (2034).
3. (attack 3) `kmt-ga27` (2), with `kmt-hq9` in its hex, attacks `jp-d40` (3) in 2034 from 2134 (no river).
   No result is drawn.
4. (attack 4) `kmt-ga22` and `kmt-ga30` (2 + 2) attack `jp-d34` and `jp-d3` (3 + 3) in 1933 from 1833 across
   the Xiang (1833:ne). No result is drawn.

The center now has KMT stacks on three of its sides, 1833, 1934 and 2033 (figure note: "attacked from three
sides"); only the attack from 1833 is drawn. The east wing at 2034 is shaded red: out of supply.

a4, the withdrawal:

1. (move 1, burst on 2033:s) `jp-d40`, out of supply and so at half strength under R6 (1.5), attacks
   `kmt-ga26` (2) in 2033 across the Miluo. `kmt-ga26` gives way one hex to 2133 (move 2), and `jp-d40`
   loses one step. `jp-d40` moves 2034 → 2033 → 2032 across the Miluo and the Xinqiang (4 points) and ends
   on its reduced face, 1-4. As in a2, `kmt-ga26` must leave 2033 before `jp-d40` passes through it.
2. (move 2) `kmt-ga26` 2033 → 2133, stacking with `kmt-ga24`.
3. (move 3) The center, `jp-d34` and `jp-d3`, withdraws 1933 → Yueyang 1932 across the Xinqiang. `jp-d34`
   ends reduced (1-4). The figures do not say whether that step was lost in the assault on the city (a3
   attack 1) or to the west group's attack (a3 attack 4).
4. (move 4) `kmt-ga30` returns 1833 → 1933 across the Xiang (clear 1 plus the major-river cost), restoring
   the screen.

End of a4: Japanese 1932 `jp-air`, `jp-d34` (reduced), `jp-d3`; 2032 `jp-d40` (reduced). KMT 1933 `ga30`;
1833 `ga22`; 2133 `ga24`, `ga26`; 2134 `ga27`, `hq9`; 1934 `ga10`, fort. Losses: KMT none, Japanese two
steps.

a5, 1944. This is a separate example, not a continuation of a4. The Japanese starting hexes are not drawn;
they follow from the moves: the attackers of Changsha in 1933, `jp-d13` in 2132, `jp-d3` in 2033, and the
western column in 1832. The KMT stood (by inference from the withdrawals) with `kmt-ga24` in 2133,
`kmt-ga27` and `kmt-hq9` in 2134, `kmt-ga22` in 1833, and a garrison in Changsha that is not drawn.

1. (attack 1) The center attacks Changsha from 1933 across the Miluo, and the city falls on 18 June. At the
   end `jp-d58` and `jp-d34` hold the city and `jp-d68` holds 1933 with `jp-air`. By inference the
   attackers are `jp-d58` and `jp-d34` (two divisions, the stacking limit; halved, 3 against 2), which
   advance into the city, and `jp-d68` moves up behind them. The garrison and the fortified-city marker are
   not shown at the end; R4 removes the marker.
2. (move 2) `jp-d13` 2132 → 2133, attacking `kmt-ga24`, which retreats one hex to 2233 and loses one step
   (1-3).
3. (move 3) `jp-d3` 2033 → 2134, attacking `kmt-ga27` and `kmt-hq9`, which retreat two hexes to 2235
   (through 2135 or 2234); `kmt-ga27` loses one step (1-3). A two-hex retreat is a combat result, not an R3
   withdrawal.
4. (move 4) `jp-d116` 1832 → 1833 → 1834 down the west bank, attacking `kmt-ga22` in 1833, which retreats one
   hex to Changde 1733 and loses one step (1-3). `jp-d116` ends in 1834, across the Xiang from the city
   (figure note: "the west column takes the far bank"). `jp-d40` ends in 1833 (inferred; its move is not drawn).

End of a5: Japanese 1934 `jp-d58`, `jp-d34`; 1933 `jp-d68`, `jp-air`; 2133 `jp-d13`; 2134 `jp-d3`; 1833
`jp-d40`; 1834 `jp-d116`, all at full strength. KMT 2233 `ga24` (reduced); 2235 `ga27` (reduced), `hq9`; 1733
`ga22` (reduced).

#### Supply

- The Japanese source is the railway at Yueyang 1932, "the railhead". The center in 1933 is on the railway
  next to 1932, and no KMT formation touches 1932, so the center is in supply throughout.
- The east wing at 2034 (a3). Its neighbors toward the railhead are KMT-held (2033, 2134, 1934). The shortest
  open path runs west across the Xiang and back: 2034 1935 1835 1735 1634 1633 1632 1732 1831 1932, nine
  hexes, through two mountain hexes (1634, 1633). If mountain and marsh hexes off the network are closed to
  supply paths, the shortest path is eleven hexes, through Baoqing 1635; if ZOC extends across major
  rivers, it is ten. The east wing is out of supply under any limit of eight hexes or fewer on the local
  path. The caption's reason, "every path back to Yueyang runs through a hex held by the KMT or in its zone
  of control", is incomplete: the nine-hex path enters neither.
- The same nine-hex path exists at the end of a2, when 2033 is empty but in the ZOC of 2133, 2134 and 1934.
  Under the same limit the east wing is already out of supply then; the figure first marks it in a3.
- The Zhejiang-Jiangxi railway through 2034 does not help: to the east it runs into KMT-held 2134, to the
  west it joins the Yuehan at Zhuzhou, which leads only to KMT-held Changsha and Hengyang.
- In a4 `jp-d40` ends at 2032, on the railway next to Yueyang, in supply. Its step loss is therefore a combat
  loss; R6 takes a step only from a formation still out of supply at the end of its next activation.

#### Stacking

All stacks are within R7: in a2, 1833 (`ga22`, `ga30`) and 2133 (`ga24`, `ga26`); in a5, 1934 (`d58`, `d34`)
and 1933 (`d68`, `jp-air`).

#### Results a rule set must reproduce

- T2.1 A screen in clear terrain behind a minor river (`kmt-ga30` in 1933) may withdraw one hex across a
  major river (to 1833) when attacked by an enemy that was already adjacent, and the attacker may advance
  into the vacated hex.
- T2.2 A screen in broken terrain behind a minor river (`kmt-ga26` in 2033) may withdraw one hex to a
  friendly stack in the hills (2133). R3's trigger is met here: the center's advance into 1933 brings a
  Japanese formation adjacent to 2033.
- T2.3 A Japanese division (movement 4) can attack across the Xinqiang, advance through 2033 and cross the
  Miluo to 2034 in one activation (broken 1, clear 1, two minor rivers).
- T2.4 A KMT group army can move into a hex adjacent to two enemy stacks (2033); ZOC has no effect on
  movement.
- T2.5 Two divisions attacking the fortified city across the Miluo (halved, 3 against 2) are repulsed.
- T2.6 The east wing at 2034 is out of supply, attacks at half strength, and breaks out to 2032 at the cost
  of one step.
- T2.7 After a4 the KMT has lost nothing and the Japanese two steps; the screens are back in 1933, and
  `kmt-ga26` is in the hills.
- T2.8 In 1944 two divisions take the city (halved, 3 against 2), and each attack on a flank stack produces
  a retreat and one step loss. No KMT stack is left in a position to attack behind the center.
- T2.9 The sequences need a Move-Attack order that lets the attacker advance and keep moving after the
  defender withdraws (a2 moves 1 and 3, a4 move 1).

#### Discrepancies to resolve

- D2.1 R3's trigger (an enemy moving adjacent) does not produce a2 move 2. The Japanese at 1932 and 2032
  begin adjacent to 1933, so no enemy moves adjacent before the attack. Either R3 also triggers on an
  attack, or the a2 sequence needs the Japanese to begin further back.
- D2.2 In a2 and a4 the figure numbers the withdrawal of `kmt-ga26` after `jp-d40`'s move through 2033; the
  withdrawal must come first.
- D2.3 The a3 caption's reason for the cut supply is incomplete (see Supply). The claim holds under a
  local-path limit of eight hexes or fewer.
- D2.4 `jp-d34` and `jp-d40` each lose one step but are drawn on their 1-step backs (the step-face question).
- D2.5 The a4 caption says the position is "as it was before the attack", but `kmt-ga26` ends in 2133, not on
  its screen hex 2033.
- D2.6 The a5 caption and subtitle say eight divisions; the figure shows seven (`jp-d58`, `d34`, `d68`,
  `d13`, `d3`, `d40`, `d116`). Of the 11th Army's pieces in the Ichi-Go setup, `jp-d27` and `jp-d39` are
  absent. The study's brief named the three columns as the Dongting shore, the Xiang valley and the eastern
  hills.
- D2.7 a5 shows neither the Changsha garrison and its fate nor `jp-d40`'s move into 1833.
- D2.8 The reduced KMT pieces of a5 are drawn on their 1-step backs after one step lost (the step-face
  question).

### Test case 3: Hengyang, 22 June to 8 August 1944

#### Figures and claim

Figures `b1-hengyang-position`, `b2-hengyang-ring`, `b3-hengyang-siege` and `b4-hengyang-fall`;
memorandum, "The envelopment of Hengyang". The figures show the area from 1533 to 2239. The title box of b1
carries `jp-dir-ichigo2`.

Japanese divisions envelop Hengyang, a fortified city on the Yuehan and Hunan-Guangxi railways with a
Fourteenth Air Force field. The ring of six hexes around the city is occupied or in Japanese ZOC, so the
city is out of supply except by air. Two assaults in July are repulsed and cost the garrison a step; three
relief attempts are blocked; the 58th Division joins a third assault, and the 10th Army surrenders after 47
days. Figure note on b4: "the fortified-city rule should make the siege cost two or three Japanese activations".

#### Terrain of the area

- Hengyang 1836 (clear) lies on the Yuehan railway (north through 1835, south through 1837), the
  Hunan-Guangxi railway (southwest through 1737 and 1637 to Guilin 1538), and the road Hengyang-Chihchiang-
  Guiyang (west through 1736 to Baoqing 1635).
- The ring: 1835 north (clear), 1936 northeast (clear), 1937 southeast (broken), 1837 south (broken), 1737
  Lingling southwest (broken), 1736 northwest (broken).
- The Xiang runs along 1835:s and 1736:se, between the city and 1835 and 1736; along 1835:se, between 1835
  and 1936; along 1935:s, between Zhuzhou and 1936; along 1736:s, between 1736 and 1737; and along 1636:se
  and 1636:s, between 1636 and 1737 and 1637.
- Mountain: 1634, 1536, 1637, 1638, 1738, 1838. Broken: Baoqing 1635, 1636, 1735, 2035, 2036, 2136.

#### Initial position (b1, 22 June)

| Side | Hex | Pieces | Role |
|---|---|---|---|
| Japanese | 1934 Changsha | `jp-d34`, `jp-d58`, `jp-air` | |
| | 1935 Zhuzhou | `jp-d68` | east bank |
| | 1834 | `jp-d40`, `jp-d116` | west bank |
| | 2035 (broken) | `jp-d3`, `jp-d13` | east |
| KMT | 1836 Hengyang | `kmt-ga10`, `kmt-fort`, `us-14af` | the 10th Army |
| | 1635 Baoqing | `kmt-ga27`, `kmt-hq9` | relief from the west (79th Army) |
| | 1638 (mountain) | `kmt-ga24` | relief from Guangxi (62nd Army) |
| | 2136 (broken) | `kmt-ga30` | eastern group |

All Japanese divisions are 3-4 with 3 steps; all KMT group armies 2-3 with 3 steps.

This case does not continue case 2 piece by piece. At the end of a5, `kmt-ga24` and `kmt-ga27` were reduced
at 2233 and 2235; here they stand at full strength at 1638 and 1635 as stand-ins for other armies. The
Japanese moves from the a5 positions to b1 (e.g., `jp-d68` 1933 → 1935, `jp-d40` 1833 → 1834) are not drawn.

#### Sequence

b2, the ring closes, 23 June to 2 July:

1. (move 1) `jp-d116` 1834 → 1835 → 1736 (clear, then broken; no river), down the west bank to the hex
   northwest of the city, astride the road from Baoqing.
2. (move 2) `jp-d68` 1935 → 1936 across the Xiang (1935:s), to the hex northeast of the city.
3. (move 3) `jp-d13` 2035 → 2036 → 1937 (broken, broken), round the east side to the hex southeast of the
   city. Figure note: "13 Div cuts the railway at Leiyang". 1937 is not a railway hex; the Yuehan runs south from
   the city through 1837, which `jp-d13`'s ZOC covers.
4. (move 4) `jp-d3` 2035 → 2036 (broken), blocking the east, adjacent to `kmt-ga30` in 2136.
5. (move 5) `jp-d40` 1834 → 1735 → 1636 to screen the west, adjacent to Baoqing. The drawn route is not a
   legal path: 1735 and 1636 are not adjacent, and the straight line between them crosses Baoqing 1635,
   which the KMT holds. A legal route is 1834 → 1735 → 1736 → 1636, three broken hexes, through `jp-d116`'s
   hex. 1735, 1736 and 1636 all touch Baoqing.
6. (no number) `jp-d58` 1934 → 1935 across the Xiang (1934:s), as the reserve. `jp-d34` and `jp-air` stay at
   Changsha.

No combat. End of b2: Japanese 1934 `d34`, `jp-air`; 1935 `d58`; 1736 `d116`; 1936 `d68`; 1937 `d13`; 2036
`d3`; 1636 `d40`. KMT unchanged.

The ring at the end of b2, under R1 as written:

- 1835 is in the ZOC of `jp-d58` (1935) and `jp-d116` (1736), not of `jp-d68`, which is across the Xiang.
- 1936 and 1937 are occupied; 1837 is in the ZOC of `jp-d13`; 1736 is occupied.
- 1737 Lingling is in no Japanese ZOC. Its only Japanese neighbors, `jp-d116` (1736) and `jp-d40` (1636), are
  across the Xiang (1736:s and 1636:se).

b3, siege and relief, July:

1. (attack 1) `jp-d68` (3) attacks the city from 1936 (no river).
2. (attack 2) `jp-d116` (3) attacks the city from 1736 across the Xiang (1736:se). R4 halves each attack, 1.5
   against `kmt-ga10`'s 2; if the two are one combined attack, 3 against 2. The figure numbers them
   separately and the caption treats them together. Both are repulsed. `kmt-ga10` loses one step (reduced
   face, 1-3) and stays in the city. No loss is drawn on the attackers in b3; b4 shows `jp-d68` and
   `jp-d116` each reduced, "each having lost a step in the siege", so by inference each loses a step in
   these assaults.
3. (attack 3) `kmt-ga27` (2), with `kmt-hq9`, attacks `jp-d40` (3) in 1636 from Baoqing. Blocked; no loss
   drawn.
4. (move 4) `kmt-ga24` 1638 → 1738 → Lingling 1737 (mountain, then broken; 3 points with mountain at 2): the
   relief from Guangxi reaches the ring.
5. (attack 5) `jp-d116` (3) attacks `kmt-ga24` (2) in 1737 from 1736 across the Xiang (1736:s). `jp-d116`
   attacks twice in the figure, which covers all of July.
6. (move 6) `kmt-ga24` falls back 1737 → 1738 with no step lost (it is at full strength in b4). This is a
   combat retreat: no enemy moved adjacent to 1737, so R3 does not apply.
7. (attack 7) `kmt-ga30` (2) attacks `jp-d3` (3) in 2036 from 2136. Blocked; no loss drawn.

A dashed green arrow brings air supply from the south, 1840 → 1838 → 1837 → 1836 (figure note: "air drops: the
only supply"). `us-14af` is no longer in the city; the figure does not place it.

End of b3: as at the end of b2, with `kmt-ga10` reduced, `kmt-ga24` in 1738, and no `us-14af` in 1836.

b4, the fall, 4 to 8 August:

1. (move 1) `jp-d58` 1935 → 1835, the hex north of the city.
2. (attack 2) `jp-d58` (3) from 1835 across the Xiang (1835:s) and `jp-d13` (3) from 1937 attack `kmt-ga10`
   (reduced, 1). Halved under R4: 3 against 1. The city falls; `kmt-ga10` surrenders and is eliminated; R4
   removes `kmt-fort`.
3. (move 3) `jp-d68` 1936 → 1836 and `jp-d116` 1736 → 1836 (across the Xiang, 1736:se) enter the city, both
   on their reduced faces. The attackers do not advance; the two divisions that enter made no attack in b4.

`jp-d34` is no longer shown at Changsha; only `jp-air` remains there. A dashed arrow 1836 → 1737 → 1637 →
1538 shows the next objectives, Lingling and Guilin, along the Hunan-Guangxi railway.

End of b4: Japanese 1934 `jp-air`; 1835 `d58`; 1836 `d116` (reduced), `d68` (reduced); 1937 `d13`; 2036 `d3`;
1636 `d40`. KMT 1635 `ga27`, `hq9`; 1738 `ga24`; 2136 `ga30`.

#### Supply

- The figures claim that the ring cuts the city off (b2) and that air drops are its only supply (b3).
- Under R1 as written the claim fails. 1737 is open, and the city traces three hexes along the
  Hunan-Guangxi railway to KMT-held Guilin: 1836 1737 1637 1538. 1637 is a mountain hex, into which no ZOC
  extends; it is also a railway hex, so a restriction on off-network mountain hexes does not close it. The
  path is the same after b3 move 6.
- The claim holds if ZOC extends across major rivers, which puts 1737 in the ZOC of `jp-d116` and `jp-d40`,
  or if a Japanese formation stands in 1737. Under the first reading the city has no path at all, and only
  R5 keeps it from losing steps.
- The Japanese ring traces to the Yuehan railway at Zhuzhou 1935 within three hexes.
- Under R5 the garrison does not lose steps for lack of supply and may not attack. It makes no attack in b3
  or b4, which agrees with R5.

#### Stacking

All stacks are within R7: in b1, 1934 (`d34`, `d58`, `jp-air`), 1834 (`d40`, `d116`), 2035 (`d3`, `d13`); in
b4, 1836 (`d116`, `d68`).

#### Results a rule set must reproduce

- T3.1 The Japanese moves of b2 fit into 23 June to 2 July. `jp-d40`'s legal route through 1736 (three broken
  hexes) needs broken terrain at 1 point.
- T3.2 After b2 the city is out of supply except by air (requires ZOC across major rivers, or a Japanese
  formation in 1737; fails under R1 as written).
- T3.3 Halved assaults by single divisions, or one combined assault, fail against a garrison of 2 and cost it
  one step; each assaulting division loses one step.
- T3.4 Relief attacks from Baoqing and from 2136 (2 against 3 in broken terrain) fail.
- T3.5 A relief group can reach Lingling from 1638 in one activation and is thrown back one hex to 1738 by an
  attack across the Xiang.
- T3.6 With the garrison at 1, two divisions (halved, 3 against 1) take the city; the garrison is eliminated
  and the fortified-city marker removed.
- T3.7 The siege, 23 June to 8 August (47 days), costs the Japanese two or three activations of five
  divisions, during which the KMT and the CCP act elsewhere on the time track.
- T3.8 Air supply under R5 needs an air-support piece within range. The second `us-14af` piece stood at
  Guilin 1538 in the Ichi-Go setup, three hexes from Hengyang.

#### Discrepancies to resolve

- D3.1 The ring is open at 1737 under R1 (see Supply); either the rule or the b2 positions must change. The
  brief for the study had the 68th and 116th Divisions cross the Xiang below the city and come round from
  the west and southwest; the figure puts `jp-d68` northeast (1936), `jp-d116` northwest (1736), and no
  Japanese piece southwest (1737).
- D3.2 `jp-d40`'s drawn route (b2 move 5) jumps from 1735 to 1636 across Baoqing.
- D3.3 b3 does not draw the step losses of `jp-d68` and `jp-d116` that b4 shows.
- D3.4 `jp-d68`, `jp-d116` and `kmt-ga10` each lose one step but are drawn on their 1-step backs (the
  step-face question).
- D3.5 `us-14af` leaves the city between b2 and b3 without a drawn move, and the air-support piece that
  supplies the city under R5 is not placed.
- D3.6 R4's phrase "the defender may convert one step loss into a retreat refused" can be read two ways. The
  figures need a garrison that does not leave the city: in b3 it loses a step and stays.
- D3.7 Here the garrison, `kmt-ga10`, surrenders; in case 1 the garrison, `kmt-ga27`, withdraws to Baoqing
  (D1.5).
- D3.8 `jp-d34` is not shown in b4.

## 2. The reflux of 1945

### Test case 4: the reflux, April to August 1945

#### Figures and claim

Figures `2-reflux-setup` ("The reflux of 1945: the position in April") and `2-reflux-play` ("one way it
plays, April to August"); memorandum, "The reflux of 1945, January to August". Title-box markers: on the
setup, `jp-dir-airfields` (directive: airfield denial); on the play figure, `jp-dir-coast` (coastal
defense) and `jp-front` on its back face (facing the CCP, so the KMT player controls).

In April the CCP player commands the Japanese pieces in the south. The 20th Army's thrust over the Xuefeng
toward Zhijiang (Chihchiang) is repulsed by Alpha Force, supplied by air, and pursued. The 6th Area Army
abandons Guangxi and withdraws north; divisions that reach the Peiping axis or the coast pass to the KMT
player. The KMT retakes Nanning, Liuzhou and Guilin in July and August. CCP presence concentrates into a
field force, and the railways are interdicted. Figure note: "the Pacific War track nears Soviet entry; the players
weigh how soon Japan should lose".

#### Rules exercised

- "Supply and combat": the Xuefeng barrier works through supply.
- Air supply of Alpha Force (`us-14af` with `kmt-n6a` at Chihchiang).
- Control transfer and the anti-sabotage rule ("Players and crossed control"); the `jp-front` marker.
- Directives: airfield denial, then coastal defense.
- Rail-state markers; presence concentrating into a field force (`ccp-concentrate`); the Pacific War track.

#### Initial position, April 1945

| Side | Hex | Pieces |
|---|---|---|
| Japanese | 1635 Baoqing (broken) | `jp-d116`, `jp-d34` |
| | 1836 Hengyang | `jp-d68`, airfield (destroyed) |
| | 1935 Zhuzhou | `jp-d27` |
| | 1538 Guilin | `jp-d13`, airfield (captured) |
| | 1339 Liuzhou | `jp-d3`, airfield (captured) |
| | 1142 Nanning | `jp-d58`, `jp-d22` |
| | 2131 Wuhan | `jp-d39`, `jp-d40`, `jp-air` |
| | 1942 Canton | `jp-d104` |
| | 2332 Jiujiang; 2039 Shaoguan | `jp-g-yangtze`; `jp-g-canton` (1-2, 1 step) |
| | 1728 Laohekou | `jp-d110` (figure note: "Laohekou lost in April") |
| | 2125 (marsh) | `jp-d37`, `jp-g-longhai` |
| | 2525 Xuzhou | `jp-d65` |
| | 3229 Shanghai | `jp-d60` (outside the crop) |
| KMT | 1434 Chihchiang (broken) | `kmt-n6a` (4-4, 3 steps), `kmt-hqalpha`, `us-14af` |
| | 0936 Guiyang | `kmt-ga11` (Y-Force, 3-3, 3 steps) |
| | 1137 Dushan | `kmt-ga20` (Y-Force, 3-3, 3 steps) |
| | 1536 (mountain) | `kmt-ga27` |
| | 1733 Changde | `kmt-ga24` |
| | 1636 (broken) | `kmt-ga30`, `kmt-hq9` |
| | 1139 (clear) | `kmt-ga16` (1-3, 2 steps) |
| | 1237 (broken) | `kmt-ga35` (1-3, 2 steps) |
| | 1430 (broken) | `kmt-ga10` |
| | 1629 (mountain) | `kmt-ga26` |
| | 2734 (broken) | `kmt-ga23` |
| | 2139 (mountain) | `kmt-ga12` (1-3, 2 steps) |
| | 1234 (broken) | `us-cacw` |
| CCP | 2021 | `ccp-jjly`, base area |
| | 2029 | `ccp-n4a5`, base area |
| | 2427; 2628; 3026 (outside the crop) | `ccp-n4a4`; `ccp-n4a2`; `ccp-n4a1` |
| | 2824 Haichow | `ccp-n4a3` |
| | 2722 | `ccp-sd`, base area |
| | 2041 | `ccp-dj` (Dongjiang column) |
| | 2022, 2024, 2325, 2627, 2727, 2827, 2222, 2424 | presence, one in each |

#### Sequence

The Xuefeng thrust, April to June:

1. `jp-d116` and `jp-d34` strike west from Baoqing toward Chihchiang (drawn arrow 1635 → 1534 through 1535;
   "April-June: 20 Army over the Xuefeng toward Zhijiang"). Route: 1535, on the Hengyang-Chihchiang-Guiyang
   road, then 1534, off the road; two broken hexes. 1534 touches Chihchiang 1434. Battle at 1534 (burst,
   "Xuefeng, May").
2. The thrust is repulsed and pursued (drawn KMT arrow 1534 → 1535 → 1635; "repulsed and pursued; Alpha
   Force's first battle"). `kmt-ga20` ends at 1534, five hexes from Dushan (route not drawn). `kmt-ga27`
   enters Baoqing 1635 from 1536 (one hex). No step losses are drawn.
3. `jp-d116` ends at Zhuzhou 1935 with `jp-d27` (inferred; three hexes from Baoqing). `jp-d34` is not shown
   at the end.

The 100 km table drew the thrust with three waypoints. On the 75 km sheet the first two fall in the same
hex, 1635, so the arrow has a single leg.

The withdrawal, May to August (the directive becomes coastal defense):

4. Drawn arrow 1142 → 1339 → 1538 → 1836 → 1935 → 2131 ("May-August: 6 Area Army withdraws north; Guangxi
   abandoned"), sixteen hexes, mostly along the Nanning-Liuzhou, Hunan-Guangxi and Yuehan railways. It
   crosses the Xiang twice (1835:s, 1934:s). Its straight leg from Zhuzhou to Wuhan leaves the railway at
   Changsha and runs through 2033, crossing the Miluo (1934:ne) and the Xinqiang (2032:s).
   - `jp-d3` Liuzhou 1339 → Wuhan 2131 (twelve hexes).
   - `jp-d13` Guilin 1538 → Peiping 2416 (26 hexes), continuing on the next arrow.
   - `jp-d22` Nanning 1142 → Canton 1942 (eight hexes; not along the arrow; route not drawn).
   - `jp-d68` stays at Hengyang, `jp-d27` at Zhuzhou. `jp-d58` is not shown at the end.
   - Airfield markers: those at Hengyang and Guilin are not shown at the end; the Liuzhou marker stays, on
     its captured face, under `kmt-ga16`.
5. Drawn arrow 2131 → 2128 → 2416 ("divisions to the Peiping axis and the coast: control passes to the KMT
   player"), sixteen hexes, crossing the Yangtze (2130:s) and the Yellow River on its 1938-47 course
   (2126:ne). `jp-d13` ends at Peiping (outside the crop) with `jp-front` on its back face. `jp-d39` and
   `jp-d40` stay at Wuhan, `jp-d3` joins them, and the Wuhan stack carries `jp-front` on its back face.
   `jp-air` is not shown at the end.
6. Inferred, not drawn: `jp-d110` Laohekou 1728 → Xuzhou 2525 (eight hexes), stacking with `jp-d65`;
   `jp-d104` Canton → Shanghai 3229 (nineteen hexes, outside the crop), stacking with `jp-d60`. A burst marks
   Laohekou. Not shown at the end: `jp-d34`, `jp-d58`, `jp-d37`, `jp-g-longhai`, `jp-g-yangtze`,
   `jp-g-canton`.

The KMT return, July and August:

7. `kmt-ga16` 1139 → Liuzhou 1339 (drawn arrow through 1239, two hexes).
8. `kmt-ga35` 1237 → Guilin 1538 (drawn arrow through 1338 and 1437, three hexes; "Nanning, Liuzhou, Guilin
   retaken July-August"). `kmt-ga11` Guiyang 0936 → Guilin 1538 (inferred, six hexes).
9. `kmt-ga12` 2139 → Nanning 1142 (inferred, ten hexes).
10. `kmt-n6a`, `kmt-hqalpha` and `us-14af` stay at Chihchiang. Not shown at the end: `kmt-ga24`, `kmt-ga30`,
    `kmt-hq9`, `kmt-ga10`, `kmt-ga26`, `kmt-ga23`, `us-cacw`.

CCP:

11. Drawn arrow 2021 → 2124 (three hexes; "presence concentrates into a field force; rails interdicted"):
    presence and `ccp-concentrate` in 2124. Drawn arrow 2722 → 2821 (one hex): presence in 2821. Presence
    stays in Anyang 2222 and is new in 2224.
12. Rail markers on their interdicted face at Jinan 2522 and at 2220.
13. Unchanged: `ccp-jjly` 2021, `ccp-sd` 2722, `ccp-n4a5` 2029, `ccp-n4a4` 2427, `ccp-n4a2` 2628, `ccp-n4a1`
    3026. Not shown at the end: `ccp-n4a3`, `ccp-dj`, and the presence markers at 2022, 2024, 2325, 2627,
    2727, 2827 and 2424.

#### Supply

- The memorandum says the Xuefeng barrier works through supply: a Japanese army over the mountains is at
  the end of a long, weak supply line, and Alpha Force is supplied by air.
- At 1534 the Japanese trace to Hengyang 1836, which the Japanese hold, on the railways. Under R1 as written
  the path is four hexes, 1534 1634 1735 1835 1836, through the mountain hex 1634, into which no ZOC
  extends; the road hex 1535 is in the ZOC of the KMT stack at 1434. If mountain and marsh hexes off the
  network are closed to supply paths, the shortest path is eleven hexes, round by Yueyang and Changsha.
  The claim that the thrust is badly supplied therefore depends on that restriction in "Supply and
  combat".
- Baoqing itself, in the April position, traces three hexes to Hengyang (1635 1735 1835 1836) under every
  reading.
- Alpha Force at 1434 has `us-14af` in its hex. No written rule covers air supply of a formation in the
  field; R5 covers fortified cities only.

#### Stacking and counters

- At the end Wuhan 2131 holds three divisions, `jp-d39`, `jp-d40` and `jp-d3`: one over R7.
- 1434 holds two unit pieces (`kmt-n6a`, `kmt-hqalpha`) and `us-14af`, within R7.
- `jp-front`: two on the map and one in the title box, the three of the counter set.

#### Final position, August 1945

| Side | Hex | Pieces |
|---|---|---|
| KMT | 1434 Chihchiang | `kmt-n6a`, `kmt-hqalpha`, `us-14af` |
| | 1534 | `kmt-ga20` |
| | 1635 Baoqing | `kmt-ga27` |
| | 1339 Liuzhou | `kmt-ga16`, airfield (captured face) |
| | 1538 Guilin | `kmt-ga35`, `kmt-ga11` |
| | 1142 Nanning | `kmt-ga12` |
| Japanese | 2131 Wuhan | `jp-d39`, `jp-d40`, `jp-d3`, `jp-front` (back) |
| | 1836 Hengyang | `jp-d68` |
| | 1935 Zhuzhou | `jp-d27`, `jp-d116` |
| | 2416 Peiping (outside the crop) | `jp-d13`, `jp-front` (back) |
| | 2525 Xuzhou | `jp-d65`, `jp-d110` |
| | 3229 Shanghai (outside the crop) | `jp-d60`, `jp-d104` |
| | 1942 Canton | `jp-d22` |
| CCP | 2021; 2722; 2029 | `ccp-jjly`, `ccp-sd`, `ccp-n4a5`, each with its base area |
| | 2427; 2628; 3026 | `ccp-n4a4`; `ccp-n4a2`; `ccp-n4a1` |
| | 2124 | presence, `ccp-concentrate` |
| | 2222, 2224, 2821 | presence |
| Markers | 2522 Jinan; 2220 | rail interdicted |

#### Results a rule set must reproduce

- T4.1 The thrust from Baoqing reaches 1534, next to Chihchiang, and is repulsed by Alpha Force with air
  support; the KMT pursues to Baoqing.
- T4.2 At 1534 the Japanese are out of supply if off-network mountain hexes are closed to supply paths (an
  eleven-hex path), and in supply through 1634 if they are not.
- T4.3 Between May and August divisions move twelve to 26 hexes north, mostly by rail.
- T4.4 A division that reaches the Peiping axis or the coast turns its `jp-front` marker and passes to the
  KMT player; the anti-sabotage rule applies to the departing controller.
- T4.5 The KMT reoccupies Liuzhou, Guilin and Nanning after the Japanese leave them.
- T4.6 Presence concentrates into a field force at 2124, and the railways are interdicted at 2522 and 2220.

#### Discrepancies to resolve

- D4.1 The thrust arrow lost one leg in the conversion (two waypoints in 1635).
- D4.2 Wuhan holds three divisions at the end, one over R7.
- D4.3 The Liuzhou airfield marker keeps its captured face after the KMT retakes Liuzhou. No rule says what
  becomes of a captured airfield when its owner returns.
- D4.4 The withdrawal arrow from Zhuzhou to Wuhan runs straight from Changsha through 2033, east of the
  railway.
- D4.5 `jp-d13`'s arrow ends at Peiping, outside the figure.
- D4.6 `kmt-ga12` (Guangdong regional troops) ends at Nanning, ten hexes from its April hex; nothing in the
  figure accounts for the move.
- D4.7 Air supply of a formation in the field (Alpha Force) has no rule.
- D4.8 The pieces not shown at the end, listed in the sequence, need end positions.

## 3. August 1945

### Test case 5: Soviet entry and the surrender, 8 to 30 August 1945

#### Figures and claim

Figures `3-august-setup` ("August 1945: the eve of Soviet entry, 8 August") and `3-august-play` ("one way it
plays, 9 to 30 August"); memorandum, "August 1945: Soviet entry and the surrender". Title-box markers: on
the setup, `sov-directive` (axis choice) and `mk-pacific` (the Pacific War track); on the play figure,
`jp-dir-hold` (directive: Hold for the KMT) and `sov-withdrawal`.

Nine Soviet groupings appear at their entry stars and follow fixed axes. The 6th Guards Tank Army halts for
fuel at Lubei. Fortified zones hold groupings for an activation, and the bypassed zones at Dongning and
Hutou hold until 26 August. Airborne detachments occupy the cities. The Kwantung Army moves toward the
Tonghua redoubt. Japan surrenders on 15 August. The Eighth Route Army takes Kalgan on 23 August and
Shanhaiguan on 30 August.

#### Rules exercised

- "Soviet entry and Manchuria" and "Mongolia and the Soviet western wing": entry and axes; steppe and
  desert supply; the fuel airlift and the halt; water points; passes and fortified zones; Mengjiang;
  Soviet occupation.
- "The Sino-Soviet treaty" (14 August; no piece shows it).
- "The political layer": the Tonghua redoubt.
- "Japanese surrender as a change of phase": the directive Hold for the KMT; surrendered markers; the
  Kalgan depot, which gives replacements to the faction that first occupies Kalgan.

#### Map features used

- Passes: 3104 (Boketu, on the Chinese Eastern Railway's western line), 2809 (Lubei trail), 2906 (Arshaan),
  2215 (Kalgan).
- Water points: Erenhot 1911, Sonid 2012, Lubei 3009. The rules also name Sain Shand; the sheet gives 1709
  no water-point mark.
- Fortified-zone marks: Hailar 2903, Arshaan 2906, Heihe 3701 (the Aihui zone), Sunwu 3702, Dongning 4209.
- Tracks: the Kalgan-Urga road, the Erenhot-Sonid-Dolonnor trail, the Tamsag Bulag-Lubei trail, and the
  Choibalsan-Tamsag Bulag supply road. Airfields at Choibalsan 2204, Matad 2406 and Tamsag Bulag 2605.

#### Initial position, 8 August

| Side | Hex | Pieces |
|---|---|---|
| Soviet | 2602 Manzhouli (steppe) | `sov-36a` (3-5, 3 steps) |
| | 2605 Tamsag Bulag (steppe) | `sov-39a` (4-5, 4 steps), `sov-6gta` (6-8, 4 steps), `sov-fuel` |
| | 1709 Sain Shand (desert) | `sov-pliyev` (3-8, 3 steps) |
| | 3701 Heihe (clear) | `sov-2rb` (3-5, 3 steps) |
| | 4304 Tongjiang (marsh) | `sov-15a` (3-5, 3 steps) |
| | 4309 Suifenhe (broken) | `sov-5a`, `sov-1rb` (each 4-5, 4 steps) |
| | 4111 Tumen (broken) | `sov-25a` (3-5, 3 steps) |
| Japanese | 2903 Hailar (steppe) | `kw-fz-hailar`, `kw-4a-2` (2-4, 2 steps) |
| | 2906 Arshaan (mountain) | `kw-fz-arshaan` |
| | 3006 Solun (broken) | `kw-44a` (2-4, 2 steps) |
| | 3703 (mountain) | `kw-fz-sunwu`, `kw-4a` (2-4, 2 steps) |
| | 4109 Mudanjiang (clear) | `kw-5a` (2-4, 2 steps) |
| | 4209 Dongning (broken) | `kw-fz-dongning`, `kw-3a` (2-4, 2 steps) |
| | 4506 Iman/Hutou (broken) | `kw-fz-hutou` |
| | 3610 Changchun (clear) | `kw-30a` (2-4, 2 steps), `jp-depot-changchun` |
| | 3314 Mukden | `pup-manchukuo2` (1-2, 1 step), `jp-depot-mukden` |
| | 3708 Harbin | `pup-manchukuo1` (1-2, 1 step), `jp-depot-harbin` |
| | 2215 Kalgan (broken) | `pup-mengjiang` (1-3, 1 step), `jp-depot-kalgan` |
| | 3916 Hamhung (broken) | `kr-34a` (2-3, 2 steps) |
| | 2416 Peiping | `jp-d63` (3-4, 3 steps) |
| CCP | 2615 Chengde (mountain) | `ccp-jrl` (2-4, 2 steps) |
| | 2118 (mountain) | `ccp-jcj` (2-4, 2 steps) |
| | 2715, 2813 Chifeng, 2415 | presence, one in each |

#### Sequence

The western wing:

1. `sov-36a` Manzhouli 2602 → Hailar 2903 → 3104 → Qiqihar 3405 along the Chinese Eastern Railway's western
   line (2602 2703 2802 2903 3003 3104 3204 3305 3405, eight hexes), through the Boketu pass at 3104, crossing
   the Nen at 3305:se ("36 Army: Hailar invested 10-18 Aug, the Boketu pass, Qiqihar"). The Hailar zone and
   `kw-4a-2` are still in 2903 at the end (burst). Under the rules, a fortified zone holds a grouping that
   attacks it for one activation and, once bypassed, keeps its hex until the general surrender. The arrow
   passes through 2903; the figure does not show how the grouping gets past.
2. `sov-39a` and `sov-6gta` Tamsag Bulag 2605 → Lubei 3009 along the Tamsag Bulag-Lubei trail (2605 2606 2707
   2708 2808 2809 2910 3009, seven hexes), through the pass at 2809. `sov-6gta` ends at Lubei with
   `mk-halted` and `sov-fuel` ("halted for fuel at Lubei 12-14 Aug"). `sov-39a` continues to Tongliao 3211
   (3110, 3111, 3211, crossing the Liao at 3111:se), ten hexes from Tamsag Bulag. The arrow goes on from
   Tongliao to Changchun 3610 (through Siping 3411, crossing the Liao at 3211:ne) and branches to Mukden 3314
   (crossing the Liao at 3313:s). No grouping is drawn at Changchun or Mukden; the race setup puts `sov-39a`
   at Changchun and `sov-6gta` at Mukden.
3. `sov-pliyev` Sain Shand 1709 → Erenhot 1911 → Sonid 2012 → Dolonnor 2513, ten hexes by these waypoints
   (eight in a straight line), along the Kalgan-Urga road and the Erenhot-Sonid-Dolonnor trail; it ends at
   Dolonnor. The arrow goes on to Kalgan 2215 along the Kalgan-Dolonnor road, four hexes ("Pliyev: Erenhot,
   Sonid, Dolonnor, Kalgan by 21 Aug"). Erenhot and Sonid are water points; Dolonnor (broken) is outside the
   desert. The arrow's leg from Erenhot to Sonid runs
   between 2011 (on the road and the trail) and 1912 (on the trail); both are tracks.

The northern wing:

4. `sov-2rb` Heihe 3701 → Sunwu 3702 → 3703 → Beian 3603, three hexes; it ends at Beian. Burst at 3703;
   `kw-fz-sunwu` and `kw-4a` are not shown at the end. The arrow goes on to Harbin 3708 (crossing the Sungari
   at 3707:s).
5. `sov-15a` Tongjiang 4304 → Jiamusi 4106, three hexes, crossing the Amur (4204:ne) and the Sungari
   (4204:s); it ends at Jiamusi. The arrow goes on up the Sungari to Harbin, four hexes ("15 Army and the
   flotilla up the Sungari").

The eastern wing:

6. `sov-5a` and `sov-1rb` Suifenhe 4309 → Mudanjiang 4109, two hexes along the Chinese Eastern Railway's
   eastern line. Battle at Mudanjiang (burst; "Mutanchiang 12-16 Aug: the heaviest fighting"); both end
   there. The arrow goes on through Kirin 3710 to Harbin. `kw-5a` leaves Mudanjiang and ends at Tonghua 3613
   (inferred, seven hexes; no arrow).
7. The Dongning zone with `kw-3a` (4209) and the Hutou zone (4506) remain at the end, bypassed (bursts:
   "Dongning to 26 Aug", "Hutou to 26 Aug").
8. `sov-25a` Tumen 4111 → Chongjin 4113 → Hamhung 3916, six hexes, crossing the Tumen at 4112:s; it ends at
   Hamhung. `kr-34a`, there at the start, is not shown at the end.

Airborne detachments and occupation:

9. `sov-airborne` with `sov-occupied` at Harbin 3708 and Changchun 3610; `sov-airborne` with `mk-port` on its
   denied face at Dalian 3118; `sov-occupied` with `jp-surrendered` at Mukden 3314 (figure note: "airborne
   detachments take Harbin, Changchun, Mukden 18-20 Aug, Dairen 22 Aug"). The puppet pieces at Harbin and
   Mukden and the three Manchurian depot markers are not shown at the end.

The Kwantung Army:

10. `kw-30a` Changchun 3610 → Tonghua 3613 (drawn arrow through 3611 and 3612, three hexes; "30 and 5 Armies
    toward the redoubt"), stacking with `kw-5a`.
11. Not shown at the end: `kw-44a`, `kw-fz-arshaan`, `kw-4a`, `kw-fz-sunwu`.

The surrender, 15 August:

12. `jp-d63` at Peiping receives `jp-surrendered`, and the directive becomes Hold for the KMT (figure note: "surrender
    15 Aug: directive Hold for the KMT").
13. `pup-mengjiang` dissolves (not shown at the end); `jp-depot-kalgan` stays at Kalgan.

The CCP:

14. `ccp-jcj` 2118 → Kalgan 2215 (inferred, three hexes), taking the depot; a control marker on its CCP face.
    The drawn arrow to Kalgan ("Eighth Route Army: Kalgan 23 Aug") starts at Chengde 2615 and runs four hexes
    through 2516, 2415 and 2316.
15. `ccp-jrl` Chengde 2615 → Shanhaiguan 2917 (drawn arrow through 2716 and 2816, three hexes; "Shanhaiguan 30
    Aug"), with a control marker on its CCP face.
16. The presence markers at 2715, 2813 and 2415 are not shown at the end.

#### Supply

- Steppe hexes count double for supply distance and desert hexes treble. A motorized grouping more than three
  hexes from its railhead needs the fuel airlift and loses an activation on a roll of 1 to 3. The railhead
  is Choibalsan 2204, at the end of the supply road; Tamsag Bulag is four hexes from it and Lubei nine.
  Measured from Choibalsan, `sov-6gta` and `sov-39a` are beyond the limit before they move. In the figure
  `sov-6gta` passes the rolls on the way to Lubei and fails there (12 to 14 August); `sov-39a` passes. A
  test must fix whether the distance counts from Choibalsan or from the forward depot at Tamsag Bulag.
- Pliyev's group must end each move at a water point or make the same roll. Its drawn waypoints, Erenhot and
  Sonid, are water points.
- In a desert hex a motorized grouping moves and traces supply only along a track. Pliyev's route from Sain
  Shand to Sonid lies on the Kalgan-Urga road and the Erenhot-Sonid-Dolonnor trail.
- The eastern groupings follow the Chinese Eastern Railway's eastern line.

#### Stacking

All stacks are within R7: 2605 (`sov-39a`, `sov-6gta`, `sov-fuel`), 4309 and 4109 (`sov-5a`, `sov-1rb`),
3613 (`kw-30a`, `kw-5a`).

#### Final position, 30 August

| Side | Hex | Pieces |
|---|---|---|
| Soviet | 3405 Qiqihar | `sov-36a` |
| | 3009 Lubei | `sov-6gta`, `mk-halted`, `sov-fuel` |
| | 3211 Tongliao | `sov-39a` |
| | 2513 Dolonnor | `sov-pliyev` |
| | 3603 Beian | `sov-2rb` |
| | 4106 Jiamusi | `sov-15a` |
| | 4109 Mudanjiang | `sov-5a`, `sov-1rb` |
| | 3916 Hamhung | `sov-25a` |
| | 3708 Harbin; 3610 Changchun | `sov-airborne`, `sov-occupied` in each |
| | 3118 Dalian | `sov-airborne`, port denied |
| | 3314 Mukden | `sov-occupied`, `jp-surrendered` |
| Japanese | 2903 Hailar | `kw-fz-hailar`, `kw-4a-2` |
| | 4209 Dongning | `kw-fz-dongning`, `kw-3a` |
| | 4506 Iman/Hutou | `kw-fz-hutou` |
| | 3613 Tonghua | `kw-30a`, `kw-5a` |
| | 2416 Peiping | `jp-d63`, `jp-surrendered` |
| CCP | 2215 Kalgan | `ccp-jcj`, `jp-depot-kalgan`, CCP control |
| | 2917 Shanhaiguan | `ccp-jrl`, CCP control |

#### Results a rule set must reproduce

- T5.1 Each grouping enters at its star on 9 August and follows its axis: `sov-36a` eight hexes to Qiqihar;
  `sov-39a` ten to Tongliao; Pliyev ten to Dolonnor by Erenhot and Sonid, and fourteen to Kalgan by 21
  August; `sov-2rb` three to Beian; `sov-15a` three to Jiamusi; `sov-5a` and `sov-1rb` two to Mudanjiang;
  `sov-25a` six to Hamhung.
- T5.2 Hailar is invested from 10 to 18 August, and `sov-36a` passes it on the way to the Boketu pass.
- T5.3 The bypassed zones at Dongning and Hutou keep their hexes until 26 August.
- T5.4 `sov-6gta` halts for fuel at Lubei from 12 to 14 August; Pliyev's group reaches Dolonnor by way of the
  water points.
- T5.5 Airborne detachments occupy Harbin, Changchun and Mukden (18 to 20 August) and Dalian (22 August), and
  deny Dalian's port.
- T5.6 Kwantung formations may withdraw toward the Tonghua redoubt (`kw-30a`, `kw-5a`).
- T5.7 The surrender of 15 August changes the directive to Hold for the KMT and marks `jp-d63` surrendered;
  `pup-mengjiang` dissolves.
- T5.8 The CCP occupies Kalgan on 23 August, taking the depot, and Shanhaiguan on 30 August.

#### Discrepancies to resolve

- D5.1 The setup puts `kw-fz-sunwu` and `kw-4a` in 3703 (mountain), while the sheet's Sunwu fortified-zone
  mark is in 3702 (Sunwu, clear).
- D5.2 The sheet marks a fortified zone at Heihe 3701 (Aihui), the 2nd Red Banner Army's entry hex; the setup
  does not place `kw-fz-aihui`, which the counter set has.
- D5.3 `kw-fz-hutou` stands at 4506, the Iman entry star; the sheet has no fortified-zone mark there. The map
  notes (section 10) record that Hutou and Iman share a hex.
- D5.4 `sov-36a`'s arrow passes through Hailar 2903, which the Kwantung piece and the zone hold at the end.
- D5.5 Several groupings are drawn at intermediate points with their arrows continuing: `sov-6gta` at its
  halt (12 to 14 August), with the arrow on to Changchun and Mukden; `sov-2rb`, `sov-15a`, `sov-5a` and
  `sov-1rb` short of Harbin; Pliyev at Dolonnor, with the arrow on to Kalgan, which the CCP holds at the end.
  The Hailar zone, invested until 18 August, is still drawn with `kw-4a-2` at the end.
- D5.6 The CCP arrow to Kalgan starts at Chengde 2615, `ccp-jrl`'s hex, but the piece that ends at Kalgan is
  `ccp-jcj`, from 2118. The Shanhaiguan arrow also starts at 2615: in the 100 km table it started at the
  presence hex that the setup figure moves to 2715 by an override, which the play figure lacks.
- D5.7 `kr-34a`, `kw-44a`, `kw-4a` and the puppet pieces are not shown at the end; the figure gives no result
  for them.
- D5.8 Sain Shand is a water point in the rules but has no water-point mark on the sheet.

## 4. The race

### Test case 6: the race, mid-September 1945 to January 1946

#### Figures and claim

Figures `4-race-setup` ("The race: the position in mid-September 1945") and `4-race-play` ("one way it plays,
October 1945 to January 1946"); memorandum, "The race, September 1945 to the spring of 1946". Title-box
markers: on the setup, `us-lift`, `kmt-legit` and `ccp-legit`; on the play figure, `ccp-position`,
`kmt-position` and `sov-withdrawal`.

The KMT depends on US lift, by air to Peiping and by sea to Qinhuangdao, to reach the North China cities,
which the surrendered garrisons hold for it. The Marines hold Tientsin, Qingdao and Qinhuangdao. The KMT
forces Shanhaiguan (15 and 16 November) and takes Jinzhou (26 November). The CCP crosses the Bohai by junk
and marches the Hebei and Jehol columns into Manchuria, where Northeast columns form. The Soviets deny Dairen
and hold the great cities while the treaty holds, then begin to withdraw. CCP and KMT fight at Shangdang and
Handan at a cost in Legitimacy, and the CCP breaks the Jinpu railway.

#### Rules exercised

- "Japanese surrender as a change of phase": surrendered garrisons hold for the KMT.
- "Strategic transport and outside powers": Strategic Lift; the Marines as port markers.
- "The Sino-Soviet treaty", taken as accepted: the Soviet-held cities are denied to the CCP; the KMT may not
  use Dairen; presence beside Soviet hexes does not concentrate at reduced cost.
- The Soviet occupation and the withdrawal track.
- "Chinese-on-Chinese conflict": the Legitimacy track.
- Presence concentrating into Northeast columns; rail-state, port and control markers; the Postwar Position.

#### Initial position, mid-September

| Side | Hex | Pieces |
|---|---|---|
| Japanese | 2416 Peiping | `jp-d63`, `jp-surrendered` |
| | 2518 Tientsin | `jp-d110`, `jp-surrendered` |
| | 2522 Jinan | `jp-d59`, `jp-surrendered` |
| | 2525 Xuzhou | `jp-d65`, `pup-nanjing2` (1-2, 1 step) |
| | 2523 (broken); 2219 Shijiazhuang; 2024 Zhengzhou | `jp-g-jinpu`; `jp-g-pinghan`; `jp-g-longhai` |
| KMT | 1920 Taiyuan | `kmt-ga6` (Shanxi, 1-3, 2 steps) |
| | 1815 Guisui | `kmt-fu` (Fu Zuoyi, 1-3, 2 steps) |
| | 1624 Tongguan | `kmt-ga34`, `kmt-hq1` |
| | 2124 Xinxiang/Kaifeng | `kmt-ga28` |
| | 2325 (marsh) | `kmt-ga33` |
| Soviet | 3314 Mukden | `sov-6gta`, `sov-occupied`, `jp-depot-mukden` |
| | 3610 Changchun | `sov-39a`, `sov-occupied`, `jp-depot-changchun` |
| | 3708 Harbin | `sov-15a`, `sov-occupied`, `jp-depot-harbin` |
| | 3118 Dalian | `sov-occupied`, port denied |
| | 2715 (mountain); 3916 Hamhung | `sov-pliyev`; `sov-25a` |
| CCP | 2215 Kalgan | `ccp-jcj`, `jp-depot-kalgan`, CCP control |
| | 2917 Shanhaiguan | `ccp-jrl`, CCP control |
| | 2722; 2021 | `ccp-sd`, `ccp-jjly`, each with its base area |
| | 3120 Chefoo | `ccp-junks` |
| | 1717 (broken); 2726; 2425 (marsh) | `ccp-js`; `ccp-n4a3`; `ccp-n4a4` |
| | 2118 | base area |
| | 2320, 2421, 2821, 2019, 2813 Chifeng, 2615 Chengde | presence, one in each |
| US | 2718 (sea); 3121 (clear); 3116 (sea) | Marines markers: Tianjin; Qingdao; Qinhuangdao |

#### Sequence

US lift and the KMT:

1. October: armies flown from Chongqing 1031 to Peiping 2416 (dashed US arrow, 22 hexes; "October: armies
   flown to Peiping"). `kmt-94a` (4-4, 3 steps) and `kmt-ga22` end at Peiping with a KMT control marker.
   `jp-d63` and its surrendered marker are not shown at the end.
2. 30 September to October: the Marines land and hold the ports (dashed arrows 2718 → Tientsin 2518, two
   hexes, and 3121 → Qingdao 3022, two hexes; "Marines hold Tientsin, Qingdao, Qinhuangdao"). KMT control
   markers at Tientsin and Qingdao. `jp-d110` is not shown at the end.
3. November: `kmt-13a` and `kmt-52a` (each 4-4, 3 steps) by sea to Qinhuangdao (dashed arrow from the sea at
   3522 by 3116 to 2917, ten hexes). On the 75 km sheet Qinhuangdao and Shanhaiguan share hex 2917, which
   `ccp-jrl` holds at the start (D6.1). The Qinhuangdao Marines marker ends in 2917 with `kmt-13a`,
   `kmt-52a` and a KMT control marker.
4. 15 and 16 November: the KMT forces Shanhaiguan. `ccp-jrl` withdraws to Chengde 2615 (inferred, three
   hexes) and ends there with a CCP control marker.
5. KMT arrow 2917 → Jinzhou 3115, three hexes ("Shanhaiguan 15-16 Nov, Jinzhou 26 Nov"). Its straight line
   crosses the sea hex 3016; the land route along the Peiping-Mukden railway is 2917 2916 3015 3115. Battle
   at Jinzhou (burst). At the end the figure draws `ccp-ne1` in Jinzhou and `kmt-13a` and `kmt-52a` still in
   2917 (D6.3).
6. KMT arrow Zhengzhou 2024 → Handan 2221, four hexes, crossing the Yellow River at 2023:s ("KMT armies north
   along the Pinghan"). `kmt-ga15` ends at Zhengzhou (new to this case; `jp-g-longhai` is not shown at the
   end). `kmt-ga33` ends in 2221, four hexes from 2325, with a `kmt-fort` marker. Battle at Handan (burst;
   "Handan, Oct-Nov: Legitimacy cost").
7. `kmt-ga28` Xinxiang/Kaifeng 2124 → Jinan 2522 (inferred, four hexes), sharing the hex with `jp-d59` and
   its surrendered marker.
8. `kmt-ga6` stays at Taiyuan. The Shangdang battle (burst in 2022, a mountain hex with CCP presence;
   "Shangdang, Sept-Oct") draws no loss on `kmt-ga6`. Not shown at the end: `kmt-fu`, `kmt-ga34`, `kmt-hq1`.

The CCP:

9. The Bohai crossing, September to November, from Chefoo 3120 by junk. Arrow 3120 → 3317 → Antung 3516, six
   hexes, e.g., by the sea hexes 3219 and 3218 to landfall at 3217 on the Liaodong peninsula, then 3317 and
   3416, crossing the Yalu (3416:ne); `ccp-ne3` ends at Antung. Arrow 3120 → 3216 → Yingkou 3215, five
   hexes; its straight line crosses Dalian 3118 (Soviet-occupied), and a route by 3119, 3218 and 3217 avoids
   it. `ccp-ne2` ends at Yingkou with `mk-port` on its denied face ("Yingkou: landing blocked").
10. The march from Jehol with the cadres: arrow Chengde 2615 → Chifeng 2813 → 3113 → 3412, nine hexes, crossing
    the Liao (3313:ne); `ccp-ne4` ends in 3412 (clear). Arrow 2615 → Jinzhou 3115, five hexes; `ccp-ne1` ends
    at Jinzhou.
11. The four Northeast columns (`ccp-ne1` to `ccp-ne4`, each 2-4 with 2 steps, "formed in play") appear in
    Manchuria. The figure does not show which presence markers or field forces formed them.
12. Rail markers: interdicted at 2320 and 2421 on the Pinghan; broken at 2523, where `jp-g-jinpu` stood
    ("Jinpu railway campaign"). `jp-g-jinpu` and `jp-g-pinghan` are not shown at the end.
13. `ccp-jcj` stays at Kalgan with CCP control; `ccp-sd`, `ccp-jjly` and `ccp-junks` stay. Not shown at the
    end: `ccp-js`, `ccp-n4a3`, `ccp-n4a4`, the base at 2118, the six presence markers of the setup (a new
    one stands in 2022), and `jp-depot-kalgan`.

The Soviets:

14. Arrow Changchun 3610 → Harbin 3708, three hexes, crossing the Sungari (3609:ne) ("Soviet withdrawal begins
    in stages (track)"). `sov-39a` is still at Changchun at the end, and the occupation markers remain at
    Mukden, Changchun, Harbin and Dalian. `sov-pliyev`, `sov-25a` and the Manchurian depot markers are not
    shown at the end.

#### Supply

The figures make no supply claim. The questions for a rule set are the port markers (a Marines marker as the
holder of a port, a CCP column denying Yingkou) and the treaty's rule on which ports Strategic Lift may use.

#### Stacking

All stacks are within R7 if a surrendered Japanese division counts as one unit piece: 2416 (`kmt-94a`,
`kmt-ga22`), 2917 (`kmt-13a`, `kmt-52a`, Marines marker), 2522 (`jp-d59` surrendered, `kmt-ga28`).

#### Final position, January 1946

| Side | Hex | Pieces |
|---|---|---|
| KMT and US | 2416 Peiping | `kmt-94a`, `kmt-ga22`, KMT control |
| | 2518 Tientsin | Marines (Tianjin), KMT control |
| | 3022 Qingdao | Marines (Qingdao), KMT control |
| | 2917 Shanhaiguan | Marines (Qinhuangdao), `kmt-13a`, `kmt-52a`, KMT control |
| | 2522 Jinan | `jp-d59`, `jp-surrendered`, `kmt-ga28` |
| | 2221 | `kmt-ga33`, `kmt-fort` |
| | 2024 Zhengzhou | `kmt-ga15` |
| | 1920 Taiyuan | `kmt-ga6` |
| Soviet | 3314 Mukden | `sov-6gta`, `sov-occupied` |
| | 3610 Changchun | `sov-39a`, `sov-occupied` |
| | 3708 Harbin | `sov-15a`, `sov-occupied` |
| | 3118 Dalian | `sov-occupied`, port denied |
| CCP | 3115 Jinzhou | `ccp-ne1` |
| | 3215 Yingkou | `ccp-ne2`, port denied |
| | 3516 Antung | `ccp-ne3` |
| | 3412 | `ccp-ne4` |
| | 2615 Chengde | `ccp-jrl`, CCP control |
| | 2215 Kalgan | `ccp-jcj`, CCP control |
| | 2722; 2021 | `ccp-sd`, `ccp-jjly`, each with its base area |
| | 3120 Chefoo | `ccp-junks` |
| | 2022 | presence |
| Markers | 2320, 2421; 2523 | rail interdicted; rail broken |

#### Results a rule set must reproduce

- T6.1 Strategic Lift carries two KMT armies 22 hexes by air to Peiping in October and two by sea to
  Qinhuangdao in November.
- T6.2 A surrendered Japanese garrison holds its city for the KMT until a KMT formation arrives (Peiping,
  Tientsin, Jinan).
- T6.3 A Marines marker takes a port and gives the KMT control of it.
- T6.4 With the treaty accepted, the KMT may not land at Dalian, and the CCP may not enter the Soviet-held
  cities.
- T6.5 The CCP crosses the Bohai by junk from Chefoo (five or six hexes) to the Liaodong peninsula, and a CCP
  column can block a KMT landing at Yingkou.
- T6.6 Northeast columns form in Manchuria, and columns march five to nine hexes from Jehol.
- T6.7 KMT-CCP battles at Shangdang, Handan and Jinzhou cost Legitimacy.
- T6.8 The CCP interdicts the Pinghan (2320, 2421) and breaks the Jinpu (2523).
- T6.9 The Soviet withdrawal begins and releases hexes to the adjacent faction.

#### Discrepancies to resolve

- D6.1 Qinhuangdao and Shanhaiguan share hex 2917, which `ccp-jrl` holds at the start. The Marines marker and
  the sealift of `kmt-13a` and `kmt-52a` enter a CCP-held hex. The rule set must say whether a lift may
  deliver into a hex the rival holds, and the order of events must be fixed (the Marines and the KMT armies
  landed at Qinhuangdao, then forced Shanhaiguan).
- D6.2 The Qingdao Marines marker starts in 3121, a clear land hex on the Shandong peninsula next to
  Chefoo, although the setup's note puts the Marines "off the ports". The conversion from the 100 km table
  placed it on land.
- D6.3 The KMT arrow's label says Jinzhou fell on 26 November, but the figure leaves `ccp-ne1` in Jinzhou and
  the KMT armies in 2917. The result of the Jinzhou battle must be stated.
- D6.4 Jinan 2522 holds a surrendered Japanese division and a KMT group army. The rules must say whether a
  surrendered piece counts for stacking and when it leaves the map.
- D6.5 `kmt-fort` at Handan 2221. Handan is not a fortified city, and no rule says how a field position
  receives the marker.
- D6.6 The KMT arrow to Handan starts at Zhengzhou, where `kmt-ga15` ends; the piece at Handan,
  `kmt-ga33`, came from 2325.
- D6.7 The straight lines of the junk arrow to Yingkou and of the KMT arrow to Jinzhou cross Dalian 3118 and
  the sea hex 3016.
- D6.8 The figure does not show how the Northeast columns formed, or the pieces not shown at the end (listed
  in the sequence).

Copyright Ben Paul Wise. All Rights Reserved.
