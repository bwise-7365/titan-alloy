Copyright Ben Paul Wise. All Rights Reserved.

# Making the next map readings cheaper: PGG again, D-Day at Tarawa, Dai Senso

Written 2026-09-17 while the SMW chain trial ran. Facts are from the sheets in `map_graphics/xml`, the
task files and today's ledger; estimates are marked as such.

## 1. What today's trial measured

| stage | image reads | tokens | tool calls | wall clock |
|---|---|---|---|---|
| S scripts (no images) | 0 | 169k | 42 | 12 min |
| C-rail, 130 hexes | 28 | 251k | 161 | 28 min |

Two lessons that carry over to every map:
- Image tokens are a small share. Twenty-eight crops cost roughly 40k tokens; the other 210k were
  reasoning, re-reading instructions, and rewriting the chain JSON after every crop (161 tool calls
  for 28 looks). The lever is the worker's bookkeeping, not the pictures.
- The chain trace and the tile reading disagree on about 49 rail hexes, and Ben's eye finds printed
  rail that both missed. Two model readings do not make a truth; one model reading plus a structural
  gate plus Ben's eye on a difference image does, and the last of those costs no tokens.

## 2. The three maps, by what they are made of

| | PGG | DDAT | DS |
|---|---|---|---|
| grid | flat 59 x 31, 1829 hexes | pointy 42 x 25, 1050 hexes | two pointy grids 27 x 53 and 29 x 53, 2968 hexes |
| scan | 5615 x 3727, hex 62 px | 1786 x 1153, hex 25 px | 3990 x 3198, hex 41 px |
| line chains | river 650 hexsides, rail 260 hexes, road 148 | 408 edges, no networks | river 171, mountain 113, rail 218, road 40 |
| area boundaries drawn as lines | none | none | border 361, zone 377, region 129 hexsides |
| point features | 16 hexes, 323 labels | 945 side glyphs, 1143 hex glyphs | 441 hexes, 62 labels |
| state today | sheet exists, networks rebuilt by tidy_networks.py | sheet exists from the old tracer | sheet exists from the old tracer |

The cost driver differs per map. PGG is chains. DDAT is a census of coloured dots. DS is mostly
boundaries of tinted areas, plus two grids.

## 3. Levers, in order of payoff

1. **An existing sheet is the first reading. Never read a map from scratch twice.** Round-trip the
   sheet into chain files (chain2catalogue.py --from-catalogue does this for SMW; a sheet-to-chain
   reverse for the other three is the same code reading the XML), run chain_check.py, and look ONLY
   where it flags: unexplained ends, ends within one hex of each other, non-adjacent pairs. For PGG
   that list is short: rail is one piece already, roads end at printed dots by the profile's
   exception, rivers have 11 pieces to explain. Estimate for PGG: about 40 looks, 300k tokens, 2 h.
2. **Ben's eye first, on things that cost nothing.** A difference image between two renders, and a
   scan/render strip around any hex he names, are script output. Each mark he makes becomes one crop.
   Today that found the Brest-Litovsk line neither reading had. Build the strip tool once
   (today's ad hoc script, made a `--strip` option of crop.py).
3. **Regions by membership, not by tracing their outline.** DS's border, zone and region lines are
   867 hexsides that are all boundaries of tinted hex sets. Membership per hex comes from a colour
   mask at the hex centre by script; the boundary hexsides are then derived, not read. That removes
   the largest single block of DS reading. Rivers, mountains, rail and road stay chains: about 540
   hexes and hexsides, roughly 110 looks.
4. **Point features by census.** DDAT's 945 fire dots and Tarawa's position badges are coloured marks
   at known slots; colour IS the data. A script classifies each side's dot colour against the palette
   and a reader confirms a sample on contact sheets (about 15 sheets). No tile reading at all.
   The same census reads DS's 441 place and marker hexes: mask finds the blob, one contact sheet per
   eight names it.
5. **Cut the worker's bookkeeping.** Today's worker rewrote the whole chain JSON after every crop and
   re-read its instructions often. Change the contract: the worker APPENDS one decision line per look
   to a decisions file; a script builds the chain file from decisions. Fewer tool calls, smaller
   context, and a killed session loses nothing. Expected to halve the non-image tokens.
6. **Lattice by autocorrelation before any new scan is read.** DDAT and DS need fresh calibration and
   DS has two grids with a seam. The SMW tile run lost hours to hand control points and a wrong
   line_dark. Write the period-and-phase finder once (the old tools/calibrate.py is the seed); confirm
   with two printed ids; extent from where the periodic signal exists.
7. **Keep the overlay off the lines.** The tile and chain runs both drew our hex id on the ray from the
   centre to the east midpoint with an opaque halo, which is exactly where every east-west link crosses;
   Ben found the railway through 1831 hidden under its own label. Fixed 2026-09-17: the id sits near a
   corner, translucent. A reader's crop must never cover a midpoint ray or a hexside; check any new
   overlay element against that rule before the first look.
8. **Check the scan before anything.** DDAT's hexes are 25 px across in the only scan. Crops at
   scale 3 will be soft, and every misread costs a look. A better scan of DDAT is worth more than any
   process change on that map; decide that before starting it.

## 4. Estimates per map with the levers above (not measurements)

| map | one-time scripts | looks | tokens | wall clock | review points for Ben |
|---|---|---|---|---|---|
| PGG redo (verification mode) | strip tool, sheet reverse | about 40 | about 300k | 2 h | difference image; final render |
| DDAT | lattice finder, dot census | about 30 | about 250k | 2 h plus scripts | scan quality; palette labels; final render |
| DS | two-grid lattice, region masks | about 110 | about 600k | 5 h plus scripts | zone and region legend meanings; final render |

For comparison the tile process spent about 3M tokens reading PGG's tiles alone.

## 5. What not to do

- No second full model reading as a cross-check. The two SMW readings disagreed on 49 rail hexes and
  neither was right at Brest-Litovsk; the gate and Ben's eye are the cross-check.
- No per-tile verification pass. Verification is structural (chain_check, network_check) and visual
  (Ben, difference image).
- No more than one worker per stage, and no worker edits the task file mid-stage; the coordinator
  does that from the decisions file at the boundary.

Copyright Ben Paul Wise. All Rights Reserved.
