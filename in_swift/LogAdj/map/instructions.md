# CLAUDE.md — project standing context

This file is read automatically by Claude Code at the start of every session
in this project. It documents the design decisions, code-style requirements,
and known tradeoffs that should not be re-litigated without explicit user
direction.

## Project purpose

Render simplified SVG maps of the Caribbean — specifically Cuba, Jamaica,
Hispaniola (Haiti + Dominican Republic), Puerto Rico, the Lesser Antilles
chain between PR and Martinique (Guadeloupe, Dominica, etc.), and Saint
Vincent — from Natural Earth 1:50m admin-0 country polygons. The simplification
amount is user-controlled via a Douglas-Peucker epsilon parameter so the user
can iterate visually.

## Data source (settled)

- Natural Earth 1:50m Admin 0 Countries, public domain.
- Direct GeoJSON URL the project expects:
  `https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_50m_admin_0_countries.geojson`
- Download into `data/ne_50m_admin_0_countries.geojson`. Do not check it in.
- Do not switch sources without asking. Wikimedia SVG sources were
  considered and rejected because they ship pre-flattened Bezier curves that
  make polyline simplification awkward.

## Pipeline (settled)

1. Parse GeoJSON with the in-project `Json` parser (no external JSON deps).
2. Split MultiPolygons into individual polygons, then clip by per-polygon
   lon/lat bounding box. The default bbox is `lon ∈ [-85.5, -60.5],
   lat ∈ [10.0, 23.5]`. This split-then-clip ordering is intentional: it
   keeps the French overseas departments (Martinique, Guadeloupe) without
   dragging in mainland France.
3. Project to spherical Web Mercator (`Mercator.java`).
4. Simplify each ring with Ramer-Douglas-Peucker (`Simplify.java`, iterative
   stack so deep rings don't overflow). Epsilon is in Mercator radians.
5. Normalize into pixel space with uniform scale, flipping y for SVG.
6. Emit one `<path>` per polygon, `fill-rule: evenodd` for holes.

## Projection choice

Spherical Web Mercator was chosen for simplicity and ubiquity. The user
explicitly selected Mercator from a menu of {plate carrée, Mercator, Lambert
conformal conic centered on the Caribbean}. If they later ask to switch to
LCC for better conformal accuracy at this latitude band, the swap point is
`Main.projectRing` — add a new class implementing the same lon/lat → x/y
contract.

## Simplification: Douglas-Peucker is the default; Visvalingam-Whyatt is the escape hatch

DP works on maximum perpendicular distance and can erase entire small
features at aggressive epsilon. If the user reports that small named islands
(St Vincent, the Grenadines, parts of Guadeloupe) are disappearing, the
right response is to add a Visvalingam-Whyatt implementation keyed on
minimum triangle area, which preserves small features better. Do not
silently switch algorithms.

Reference values for ε in Mercator radians:

| epsilon  | character                                        |
|----------|--------------------------------------------------|
| 0        | no simplification; raw 1:50m detail              |
| 1.0e-5   | imperceptible                                    |
| 1.0e-4   | noticeable on Cuba's coast, small islands intact |
| 5.0e-4   | cartoon outlines, smallest islands may collapse  |
| 2.0e-3   | near-caricature                                  |

## User code-style requirements (strict)

These are not suggestions. Violating them is a defect.

- **Java 17** target. Use sealed interfaces and records where they fit.
- **Curly braces around every `if`/`else`**, even single-line bodies.
- **No `Map.merge` or `Map.getOrDefault`** except for well-established
  initial-population patterns. Prefer explicit absence handling.
- **No silent defaults for missing inputs.** When a configuration value or
  required field is absent, fail loudly. Bugs that surface later from
  silent fallbacks are exactly what the user wants to avoid.
- **No "programming by side-effect"** beyond obvious I/O. Prefer
  referential transparency. Functions that mutate hidden state are the
  wrong shape.
- **Files under ~300 lines.** If a file is growing past that, split it.
  All current files are well under that bar; keep it that way.
- **Cross-platform.** Code must compile and run identically on Debian
  Linux and Windows. Don't introduce anything platform-specific.
- **Build: Ant + Netbeans-compatible.** IntelliJ also fine; do not switch
  to Maven or Gradle.
- **No external runtime JAR dependencies.** The hand-rolled `Json` parser
  exists because we deliberately avoided pulling in Jackson/Gson. Don't
  reverse that decision.

## File layout

```
caribbean-map/
├── build.xml                # Ant: clean, compile, jar, run
├── README.md
├── CLAUDE.md                # this file
├── data/
│   ├── test_islands.geojson # tiny synthetic test fixture
│   └── ne_50m_admin_0_countries.geojson  # download separately
├── src/org/example/caribbean/
│   ├── Json.java            # sealed-interface JSON AST + recursive-descent parser
│   ├── Geom.java            # LonLat, XY, Ring, Poly, Feature, Bbox records
│   ├── Features.java        # GeoJSON → Feature, per-polygon bbox clip
│   ├── Mercator.java        # spherical Web Mercator
│   ├── Simplify.java        # iterative Douglas-Peucker
│   ├── SvgWriter.java       # SVG emitter
│   └── Main.java            # CLI, pipeline glue
└── tools/
    └── reference_pipeline.py # Python mirror of the Java pipeline; useful
                              # for sanity checks and algorithm verification
```

## CLI contract

```
java -jar dist/caribbean-map.jar \
     --input  data/ne_50m_admin_0_countries.geojson \
     --output caribbean.svg \
     [--min-lon -85.5] [--max-lon -60.5] \
     [--min-lat  10.0] [--max-lat  23.5] \
     [--epsilon 1.0e-4] \
     [--width 1200] [--height 600] \
     [--precision 2]
```

Stderr already reports `features retained`, raw vs simplified point count,
and percent retained. Don't reformat that without asking; the user uses it
to tune epsilon.

## Common tasks

### Building
```
ant clean jar
```

### Producing the canonical three-version comparison
```
mkdir -p out
java -jar dist/caribbean-map.jar --input data/ne_50m_admin_0_countries.geojson \
     --output out/caribbean-fine.svg   --epsilon 1.0e-4
java -jar dist/caribbean-map.jar --input data/ne_50m_admin_0_countries.geojson \
     --output out/caribbean-medium.svg --epsilon 5.0e-4
java -jar dist/caribbean-map.jar --input data/ne_50m_admin_0_countries.geojson \
     --output out/caribbean-coarse.svg --epsilon 2.0e-3
```

### Sanity-checking the algorithm without the JDK
```
python3 tools/reference_pipeline.py data/test_islands.geojson out.svg 5.0e-4
```
The Python reference mirrors the Java pipeline one-for-one. If the two
disagree on the same input, the Java is wrong (the Python is the spec).

## Things to ask before doing

- Switching projections (Mercator is settled).
- Switching simplification algorithms (DP is settled; VW is the named escape hatch).
- Adding external dependencies (we deliberately have none).
- Changing the bounding box default (the user picked the current one).
- Changing the SVG styling (`#e8e0c8` land, `#cfe2f0` sea, evenodd fill).
- Restructuring the file layout or splitting/merging files.

## Things you can do without asking

- Run the build.
- Run the pipeline at any epsilon the user requests.
- Produce comparison HTML pages, side-by-side renderings, etc.
- Add unit tests under `src/test/` (mirror the package layout).
- Improve error messages.
- Fix bugs the user reports, with the code-style rules above.
