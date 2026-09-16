Copyright Ben Paul Wise. All Rights Reserved.

# Task 13: Stalin Moves West map sheet by image2sheet (milestone M6i)

status: assigned
worker: W6 (sonnet)         started: -
resume: not started. Read map_graphics/xml/tools/image2sheet/README.md and work it stage by stage.
inputs:
  map_graphics/xml/tools/image2sheet/README.md          the process; follow it stage by stage
  map_graphics/xml/tools/image2sheet/maps/template.json  copy to maps/smw.json; check_config.py smw
  map_graphics/xml/tools/image2sheet/examples/pgg/       the pilot's prompts and two worked records
  C:/Library/War-Games/Stalin Moves West/Stalin Moves West map expanded.png   PRIMARY (2475 x 3825)
  C:/Library/War-Games/Stalin Moves West/Stalin Moves West map.jpg    the same image at 1650 x 2550
    NOTE: the expanded PNG is the JPG resampled 1.5x, so it carries no detail the JPG lacks; it is primary
    only because crops read better at tile scale. Ben may add a true scan of his physical copy tomorrow
    (2026-09-16). Swapping sources is a path change in maps/smw.json plus a re-run of stage 1 and after, so
    do not hand-tune anything to this particular raster.
  C:/Library/War-Games/Stalin Moves West/Stalin_Moves_West_Rules_V8F-ERULES.pdf   terrain and map rules
  map_graphics/xml/hexsheet.xsd, README.md, hexsheet2svg.py
  hexgames/PLAN.md "Terms" and RESUME HERE; CLAUDE.md
outputs:
  map_graphics/xml/tools/image2sheet/maps/smw.json      the configuration
  map_graphics/xml/stalin-moves-west.xml                the sheet, valid against hexsheet.xsd
  map_graphics/xml/stalin-moves-west.{svg,png}          rendered with hexsheet2svg.py, 0 warnings
  map_graphics/xml/tools/network_check.py               an SMW profile (acceptance rules, named exceptions)
  CMakeLists.txt                                        hygiene_map_networks also runs on the new sheet
  this file                                             the stage log, the decisions list, the questions log
acceptance:
  every stage gate in the README passes; ctest labels xsd and hygiene green (foreground, after checking that
  no ninja, cl, link, ctest or cmake process runs); no existing golden changes (this map has no game module);
  the render matches the scan under the stage 9 gate.

## Why this task exists: it tests the process, not only the map

image2sheet has run once, on PGG, on Opus, with the coordinator fixing a renderer bug and restarting
stalled workers. The README claims a Sonnet worker can run a new map from it with little guidance. This task
is that test. So:
- Work from the README. Do not ask the coordinator to make decisions the README already covers.
- KEEP A QUESTIONS LOG below: every time you must ask a person, or the README leaves you guessing, write the
  question, what you did, and which stage you were in. The log is the deliverable that matters most; a
  question you had to ask is a hole in the document, not a failure on your part.
- When a stage does not fit this map, record the decision (principle 9); never bend a gate.

## Decisions from Ben (stage 0 decisions list, settled before the work starts)

1. TWO KINDS OF RED DASHED LINE, one sheet element. The long-long dashes are the start-of-game Soviet front
   line, which is also the USSR border, printed "Front Line (USSR border)"; an example is the hexside just
   west of Brest-Litovsk, between 1838 and 1839 (the city hex). The long-short dashes are national borders.
   BOTH become the same `border` hexside line. The sheet does not record where the front line was; dropping
   that distinction is deliberate.
2. PORT CITY HEXES ARE LAND. Stettin 2030, Danzig 2234 and Konigsberg 2235 are city hexes that the print
   draws partly in water. Their terrain is the city's land terrain with a port glyph, never sea or lake.
   Check every other coastal city the same way.
3. The map carries a Terrain Effects Chart, combat tables and turn tracks around the playing area. They are
   furniture: clip the grid to the printed map and record none of it.

## Questions log (the worker fills this in as it goes; one entry per question)

| stage | question | what the README said, or did not | what you did |
|---|---|---|---|
| config/7 | PGG's `sheet.keep` copies palette/terrains/lines/labels/panels from an EXISTING sheet XML. SMW has none: `assemble.py` unconditionally does `etree.parse(sheet_cfg["path"])` and would throw FileNotFoundError. `<sheet>` also requires `palette` and `terrains` children (hexsheet.xsd, non-optional) that only `assemble.py`'s `kept()` writes. The README's "Requirements and layout" and stage 7 assume the sheet being rebuilt already exists; it never says what a map with no prior sheet does. | Hand-authored a minimal seed `stalin-moves-west.xml` (root attrs + `<grid>` stub + `<palette>` + `<terrains>` + `<lines>`, no hex content) before stage 1, and set `sheet.keep: ["palette","terrains","lines"]` in `maps/smw.json` so `assemble.py` carries them through unchanged. |

## Stage log (the worker fills in: gates, counts, timings, pitfalls)

log:
- 2026-09-15 written by the coordinator; not yet launched

Copyright Ben Paul Wise. All Rights Reserved.
