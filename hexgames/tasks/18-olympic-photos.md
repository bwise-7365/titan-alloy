Copyright Ben Paul Wise. All Rights Reserved.
# Task 18: Olympic map, re-read from Ben's three flat detail photos
status: review
worker: W18a..W18f (sonnet), one sub-task each; coordinator merges into the sheet
started: 2026-09-29
resume: ALL sub-tasks A-G done and merged into operation-olympic.xml (validates, renders); open questions for Ben in example maps/Olympic/reading-notes.md (2026-09-29 section)
inputs: `example maps/Olympic/work/` (everything a worker needs is there); `map_graphics/xml/operation-olympic.xml` (read only)
outputs: `example maps/Olympic/work/out/<subtask>.txt` (one file per sub-task, written by its worker only)
acceptance: coordinator merges, validates (`tools/validate-xml.py`), renders, and compares; Ben reviews doubts

## Shared brief (every worker reads this section)

The map: Operation Olympic (Decision Games). Flat-topped hexes, printed ids `CCRR` (column 01-61 west to east,
row 00-34 north to south); EVEN columns sit half a hex lower. Neighbours of an EVEN column hex (c, r):
n (c, r-1), s (c, r+1), ne (c+1, r), se (c+1, r+1), nw (c-1, r), sw (c-1, r+1). Of an ODD column hex:
n (c, r-1), s (c, r+1), ne (c+1, r-1), se (c+1, r), nw (c-1, r-1), sw (c-1, r). Never work neighbours out
in your head; `check.py` does it.

Photos (quarter scale, upright): `left_d4.png` (about cols 01-27), `middle_d4.png` (about 20-48),
`right_d4.png` (about 38-61). They overlap. The sheet's lattice has been fitted to each photo; the fit
was checked on printed ids and is good to a few pixels.

Tools (run from `example maps/Olympic/work/`, Python):
- `python crop.py PHOTO C0 C1 R0 R1 crops/NAME.jpg [--no-lines]`: crop of columns C0..C1, rows R0..R1
  (printed numbers, inclusive) with the lattice drawn in thin magenta and each hex's id in magenta near
  its top (the printed id is just below it, in grey). `--no-lines` shows ids only. Keep crops to about
  5 columns x 5 rows. `python crop.py where HEXID` says which photos show a hex.
- `python check.py out/FILE.txt`: checks every `road:` line for adjacency and valid ids.

Rules of work (they come from earlier runs that stalled or went wrong):
1. Look at at most FOUR images per request. Never write a reading for an image you did not see (a
   "media removed" result means: request it again, fewer at once).
2. Write your output file as you go, after every crop, never at the end. It is the only record.
3. At most three looks at any one spot; then record it as a doubt (`# doubt: ...`) and move on.
4. Record doubts; never guess silently. A doubt names the hex(es) and what is unclear.
5. Do not edit the sheet XML, any .xsd, any code, or anything outside `work/out/<your file>` and
   `work/crops/`. No git commands. Do not build.
6. Finish with a short report at the end of your output file: counts, doubts, anything surprising.
   If you are blocked (a tool fails, the fit looks wrong somewhere), write `BLOCKED: <reason>` at the end
   of your file and stop; the coordinator will clarify.

## Sub-tasks

### A. Water hexes (W18a) -> out/water.txt
The printed map has exactly TWO water colours: all-sea (saturated blue) and amphibious assault hexes (pale
blue-grey). Any darker patches are shadows of the photograph, not a third colour. For every hex that is
wholly or almost wholly water, decide sea or assault. You may measure (mean colour inside each hex, e.g.
saturation) but decide by eye; shadow lowers brightness, not the blue/grey character.
Output lines: `assault: ID [letter X]` for each pale hex (note any red letter printed in it), then
`# doubt:` lines. Check every pale hex on all three photos, and the current sheet's list
(`<hexes terrain="assault" ...>` in the XML) against what you see: list `# was assault, is sea: ID` and
`# was sea, is assault: ID`.

### B, C, D. Roads, one strip each -> out/roads-west.txt (cols 01-21, left photo), out/roads-middle.txt
(cols 20-41, middle photo), out/roads-east.txt (cols 40-61, right photo)
Roads are thin dark brown/grey lines that wander across hexes; rivers are blue; grid lines are straight
and follow hexsides. Read EVERY road in your strip, fresh (ignore the sheet's draft roads; they are often a
column off). A road is the chain of hexes whose interior it passes through, in order, each step to a
neighbour. Where a road runs along a hexside or through a vertex, pick the more plausible hex and add a
doubt. One line per road piece:
`road: 1003 1104 1204 1305 | ends: place junction`
End reasons: `place` (a town dot or city in that hex), `junction` (it meets another road; the meeting hex
must be in both chains), `strip` (it leaves your column range; continue one column past your range so the
strips overlap), `coast`, `edge` (map edge), `unexplained` (it just stops; a doubt). Run check.py after
every few roads. Also list every town dot / city you see in your strip: `place: ID Name (town|city)`,
and flag any that differ from the sheet's `<hex id=.. name=..>` entries.

### E. All-mountain hexsides (W18e) -> out/mountain-sides.txt
TEC: "All-Mountain Hexside: entry prohibited", drawn as fingers of the darkest brown shading running along
a hexside. The sheet holds 73 measured candidates (`<edge ... line="mountain-side">`), most probably wrong.
Check each candidate and look for real ones: `side: HEX:DIR real|not` (DIR one of n ne se s sw nw), plus
`# doubt:` lines. Ben believes only a few are real.

### E2. All-mountain hexsides, second pass (W18e2) -> out/mountain-sides-2.txt
Pass E was too strict (Ben, 2026-09-29). The criterion: dark all-mountain shading lying along a hexside counts,
even when the hexes on both sides are almost entirely not mountain; the TEC swatch only suggests this. Ben
confirmed 13 (calibration set): 4518:ne 4518:se 4020:se 4124:n 4223:ne 4222:se 4023:ne 4023:se 4025:ne 3925:se
4020:ne 4319:se 4321:ne. Ben expects the true count nearer the 73 measured candidates than 5.
First view the calibration set with `crop.py ... --mark` (yellow segments). Then re-judge every candidate
of pass E (out/mountain-sides.txt, `side:` lines) and search all land for more of the same look.
Output `side: HEX:DIR yes|no` (+ `# doubt:`), one line per hexside judged.

### F. Dashed boundary lines (W18f) -> out/boundaries.txt
The American Objective Line (grey dashed, near cols 23-25) and the North-South Boundary Line (red dashed,
near cols 32-33) run north-south. For each, list the hexsides it follows from end to end
(`side: HEX:DIR` in order), or, where it cuts through hex interiors, the hexes and a doubt.

### G. Japanese deployment letters (W18g) -> out/letters.txt
Every red letter group printed on the map (C, L, 2L, CL, 36C, 36L, R, ...): `letter: ID TEXT`, the hex that
holds the letter's centre; a doubt when it sits on a hexside. Compare with the sheet's `glyph symbol="text"`
entries and list differences (`# sheet has X at ID, photo has Y`).

## Coordinator decisions
- 2026-09-29 A done (54 pale hexes). Ben's ruling 3 (2026-09-25) applies: a pale hex holding a red letter is a
  Japanese deployment hex, not assault, so the 15 lettered ones are dropped: 39 assault hexes. 1705 (sheet has a
  C there, the water worker saw none) waits for G.
- 2026-09-29 Ben: 4705 is Saesbo (was Sonogi). Saeki is at 3532 (missing place). Printed spellings Yokamaechi
  (2214), Tsyuzaki (5517): Ben confirmed; applied to the sheet with Saeki and Saesbo.

## Log
- 2026-09-29 created by the coordinator; registration: reg.py (homography from landmarks, refined on the
  printed grid lines), left photo re-anchored on printed ids (landmarks_left.json).

Copyright Ben Paul Wise. All Rights Reserved.
