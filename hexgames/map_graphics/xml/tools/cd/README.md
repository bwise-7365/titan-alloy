Copyright Ben Paul Wise. All Rights Reserved.

# Circling Dragons sheet builder

`build_cd.py` writes `circling-dragons.xml` (beside itself, to be moved to `map_graphics/xml/`), a designed sheet (no scan exists) for the
China 1944-1945 game described in `Circling Dragons/circling_dragons_map_and_rules_V3.tex`. The design
notes, rulings and open doubts are in `Circling Dragons/circling-dragons-map-notes.md`.

## Files

- `lattice.py` -- the projection (Albers equal-area conic, standard parallels 27 N and 45 N, origin
  35 N 118 E) and the lattice: flat-top, odd columns half a hex down, 47 columns by 46 rows, 75 km
  between hex centres (the standard since 2026-10-06; it was 35 by 34 at 100 km), ids `{col:02}{row:02}`
  from the north-west corner. The centre, corner and
  neighbour formulas are those of `hexsheet2svg.py`, so a point placed here lands in the hex the
  renderer draws there.
- `cd_data.py` -- the hand data: places (with the few moved one hex), railways, strategic roads, named
  ranges as hex lists, lakes, area labels, the Manchukuo polygon, and the rulings that belong to one scale
  (`PLACE_HEX_BY_SCALE`, `LAND_HEXES_BY_SCALE`, `RIVER_EDITS_BY_SCALE`, `MINOR_RIVERS_BY_SCALE`,
  `LABELS_BY_SCALE`).
- `stage1_geo.py` -- rasterises Natural Earth land, lakes and countries onto the lattice; writes
  `geo_hex.json` (per hex: land fraction, lake fraction, country).
- `stage2_elev.py` -- samples ETOPO1 at 19 points per land hex through api.opentopodata.org; writes
  `elev_hex.json`. Resumable; the cached file is in the repository so the build needs no network.
- `stage3_rivers.py` -- snaps the chosen rivers' centrelines to hexside chains, then applies the scale's
  river rulings (at 75 km the Yangtze moved north of the Yueyang hex, and the Yellow River north of the
  Sanmenxia and Luoyang hexes so that the Longhai stays on its south bank) and adds the minor rivers as explicit
  hexsides; writes `rivers.json`. The builder draws the minor rivers (the Xinqiang and the Miluo) with the
  thin `minor-river` line, which both renderers round like the rivers because its id contains `river`.
- `stage4_terrain.py` -- one terrain class per hex from the elevation statistics, the steppe and marsh
  polygons, the named ranges and the place rulings; writes `terrain_hex.json` and a diagnostic PNG.
- `build_cd.py` -- assembles the sheet from the three JSON files and `cd_data.py`. `RIVER_FORM` at its
  top writes the rivers as path chains with ABC vertex junctions at the confluences (the default) or as
  plain edge elements like the PGG sheet. `VERSION` 3 is the standard sheet (the Mongolian operations
  features and the political layer); `build_cd.py 1` and `build_cd.py 2` write the earlier forms as
  `circling-dragons-v1.xml` and `-v2.xml` for comparison. The feature data are the `_V2` and `_V3` lists
  at the end of `cd_data.py`.
- `vectorize_logo.py` -- traces the simplified logo (`Circling Dragons/circling-dragons-logo.png`) into
  `Circling Dragons/circling-dragons-logo.svg`; its docstring says how.
- `illustrate.py` -- the memorandum's seventeen figures, all on the standard sheet: the eight of the "four
  major actions" (an example setup and one play-out each for Ichi-Go, the reflux of 1945, August 1945 and
  the race; their tables are written in 100 km hex ids and converted, a place's hex to the same place's
  hex, and their notes are a separate layer placed in pixels of the exported image) and the nine of the
  maneuver studies (`maneuvers.py`, in 75 km ids: Xue Yue's defence of Changsha, the envelopment of
  Hengyang). Each is a crop of the sheet's reference render with counters drawn through
  `unit_graphics/xml/counters2svg.py` from `unit_graphics/xml/circling-dragons.xml` (built by
  `unit_graphics/xml/tools/build_cd.py`), plus arrows, bursts and notes. Writes
  `Circling Dragons/illustrations/*.svg` and `.png`; the memorandum and the standalone maneuver study
  include the PNGs.

## Rebuilding

With the cached JSON files only `build_cd.py` is needed (it writes beside itself; move the sheet up):

```bash
python map_graphics/xml/tools/cd/build_cd.py
mv map_graphics/xml/tools/cd/circling-dragons.xml map_graphics/xml/
python tools/validate-xml.py map_graphics/xml
python map_graphics/xml/tools/network_check.py map_graphics/xml/circling-dragons.xml --quiet
python map_graphics/xml/hexsheet2svg.py map_graphics/xml/circling-dragons.xml --png
```

### Another scale

`CD_HEX_KM` in the environment builds the same area at another distance between hex centres (75 is the
default). The north-west corner and the pixel size of a hex stay, so the sheet grows or shrinks; every
stage then reads and writes its own caches (`geo_hex-100.json` and so on, from `lattice.cache()`), and the
builder writes `circling-dragons-100.xml`. Hand data ruled as 100 km hex lists (the named ranges, the
redoubt) are mapped by `lattice.rescale_ids()`: every hex whose centre falls in a listed 100 km hex. The
100 km caches are kept, so the first sheet rebuilds without the network:

```bash
CD_HEX_KM=100 python map_graphics/xml/tools/cd/build_cd.py
python map_graphics/xml/tools/network_check.py map_graphics/xml/tools/cd/circling-dragons-100.xml --quiet
```

A new scale reruns all five stages with the variable set; the elevation stage takes about fifteen minutes
at 75 km.

To rerun the geography stages, download these Natural Earth GeoJSON files into `tools/cd/ne/` (not
kept in the repository; about 17 MB) from
`https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/`:
`ne_50m_land`, `ne_50m_lakes`, `ne_50m_admin_0_countries`, `ne_10m_rivers_lake_centerlines`. Then run
`stage1_geo.py`, `stage2_elev.py` (network), `stage3_rivers.py`, `stage4_terrain.py`, `build_cd.py`.

Copyright Ben Paul Wise. All Rights Reserved.
