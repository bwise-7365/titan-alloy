Copyright Ben Paul Wise. All Rights Reserved.

# Circling Dragons: rulings needed

This list collects the decisions that the six test cases (`circling-dragons-test-cases.md`, 7 October 2026)
found open. Each item states the question, the options and a recommendation. The ids in brackets (e.g., D3.1)
point to the test case that gives the evidence. Items A and B are rules; item C covers figure and counter
corrections, which wait for a yes. "R1" to "R7" are the provisional rules of the memorandum's subsection
"Rules the maneuvers assume". The HexKrieg-style rules proposal (`circling_dragons_hexkrieg_rules.tex`,
section 6) answers B1, B3, B5 to B8 and B12 in its own way and changes the recommendation of B4.

## A. Done on 7 October, to confirm

**A1. The Yellow River between Tongguan and Zhengzhou [D1.1].** As ruled, the river now passes north of the
Luoyang hex 1924, as at Yueyang. The same fault one hex west was corrected with it: the river also passes
north of the Sanmenxia hex 1724, so the Longhai railway crosses the Yellow River only on the 1938 course
between Zhengzhou and Kaifeng (it crossed four times before). Two side effects: the Tongpu railway now crosses
the river from Shanxi into 1724 instead of into Tongguan, still one crossing; and the Yuncheng basin of
southern Shanxi, which shares hex 1724, counts as south bank. The sheet, its SVG and PNG, the figures and
both PDFs are rebuilt. Keeping the correction at Luoyang only means removing `1724:nw 1724:n 1724:ne` from
the add list of the `yellow` ruling in `cd_data.py` and `1624:ne 1724:s 1724:se` from its drop list.

## B. Rules to decide

**B1. Length of the local supply path [D2.3, T4.2].** The rule says "a short local path". Options: (a) a
limit in hexes; (b) also close off-network mountain and marsh hexes to supply paths. Recommendation: both,
with a limit of 3 hexes. Then the Changsha east wing (shortest path 9 hexes) and the Xuefeng thrust (11 hexes
without the mountain hex) are out of supply as the figures say, and Dushan (3 hexes) stays in supply.

**B2. The Hengyang ring [D3.1].** Under R1 the ring is open at Lingling 1737, because both Japanese
neighbors of 1737 are across the Xiang; the city keeps a railway supply line to Guilin. Options: (a) ZOC
extends across major rivers; (b) a Japanese division stands in 1737 in figure b2, as the study's brief had
the 68th and 116th Divisions come round from the west and southwest. Recommendation: (b). Option (a) would
take from every major river its value as a screen.

**B3. The withdrawal trigger [D2.1].** R3 lets a formation withdraw when an enemy moves adjacent, but the
Changsha screens withdraw when attacked by an enemy that was already adjacent. Recommendation: a formation
may also withdraw when an attack on it is declared, before the attack is resolved.

**B4. Zones of control and movement [cases 2 and 3].** R1 gives ZOC no effect on movement, and several drawn
moves pass from one enemy ZOC hex to another. Options: (a) ZOC affects supply only; (b) the HexKrieg rule, under
which entering an enemy ZOC ends a move. Recommendation, revised on 7 October: (b), as in the HexKrieg-style
proposal. The Changsha screens delayed the advance, which is the effect of the stop rule; under it four drawn
moves (a2 move 3, a5 move 4, b2 moves 3 and 5) take an extra activation.

**B5. Movement costs [introduction of the test cases].** The figures need clear 1, broken 1, mountain 2, a
minor river 1 extra and a major river at least 1 extra. With broken at 2, four drawn moves exceed the mover's
allowance. Recommendation: adopt the table, or say which moves to redraw.

**B6. The fortified-city wording [D3.6].** R4's "the defender may convert one step loss into a retreat
refused" reads two ways. Recommendation: "A defender in a fortified city may refuse a retreat by losing one
step instead."

**B7. Air supply away from fortified cities [D4.7].** R5 covers fortified cities only, but Alpha Force at
Chihchiang is supplied by air in the reflux. Recommendation: a formation on a friendly airfield hex within
range of a friendly air-support piece is in limited supply, like a fortified city. Chihchiang and Hengyang
are both airfield hexes.

**B8. Stacking exceptions [cases 1, 4, 6].** Two figures put three divisions in Wuhan, and Jinan holds a
surrendered Japanese division beside a KMT group army. Recommendation: no exception for cities (redraw
Wuhan); a surrendered piece does not count for stacking and leaves the map when a formation of the faction it
holds for enters its hex.

**B9. Lift into a hex the rival holds [D6.1].** Qinhuangdao and Shanhaiguan share hex 2917, which the CCP
holds when the KMT lands there. Options: (a) a lift may not deliver into a rival-held hex; (b) it may, and
the landing is an attack with Legitimacy cost; (c) move the Qinhuangdao port to a neighboring hex.
Recommendation: (a), with the Marines marker able to hold the port alone, so that the CCP must leave 2917 or
be attacked overland.

**B10. Airfields retaken [D4.3].** No rule says what happens to a captured-airfield marker when its owner
retakes the hex. Recommendation: remove a captured marker when the original owner retakes the hex; a
destroyed marker stays until repaired.

**B11. Soviet entry stars in fortified-zone hexes [D5.2, D5.3].** Heihe 3701 carries both the 2nd Red Banner
Army's star and the Aihui zone; the Iman star 4506 holds the Hutou zone counter. Recommendation: a grouping
whose star lies in a zone hex starts off the map and enters by attacking the zone across the border; if the
zone holds it, the grouping may enter an adjacent border hex instead, leaving the zone bypassed.

**B12. Step faces [D2.4, D3.4].** A 3-step piece has only a 1-step back, so a piece that loses one step is
drawn as if it had lost two. Options: (a) a step-loss marker for the middle step; (b) treat the flip as two
steps lost. Recommendation: (a).

## C. Figure and counter corrections, waiting for a yes

- C1. Renumber Changsha figures a2 and a4 so the KMT withdrawal from 2033 comes before the Japanese move
  through it [D2.2].
- C2. Redraw Hengyang figure b2 with a Japanese division in 1737, if B2(b) is chosen; draw the step losses of
  the 68th and 116th Divisions in b3; draw the `us-14af` piece that supplies the city [D3.1, D3.3, D3.5].
- C3. Wuhan: move one division out of 2131 in the Ichi-Go setup and the reflux play figure [B8].
- C4. Raise the airfield markers from three to five, one for each Fourteenth Air Force field the memorandum
  names (Hengyang, Lingling, Guilin, Liuzhou, Nanning) [D1.7].
- C5. Move the Sunwu zone counter and `kw-4a` from 3703 to Sunwu 3702, and place the Aihui zone at Heihe
  3701 under B11 [D5.1, D5.2].
- C6. Make the Hengyang garrison the same piece in the Ichi-Go figures and the study, and let it surrender in
  both [D1.5, D3.7].
- C7. Ichi-Go play figure: replace `kmt-ga11` (a Y-Force army that returns in 1945) at Guiyang with another
  piece, or change the counter's note [D1.6].
- C8. Race play figure: show the result at Jinzhou (the label says the KMT took it on 26 November, but
  `ccp-ne1` is drawn there); drop the fortified-city marker at Handan; put the Qingdao Marines marker at sea
  [D6.2, D6.3, D6.5].
- C9. Changsha figure a5: add the eighth division (`jp-d27`) or change "eight divisions" to seven [D2.6].
- C10. Counter set: the shared-pool marker is named "Operationalsupport"; the name should read "Operational
  support".

Copyright Ben Paul Wise. All Rights Reserved.
