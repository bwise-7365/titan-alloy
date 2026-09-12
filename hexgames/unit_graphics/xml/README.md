# hexcounters — an XML language for wargame counters and counter sheets

`hexcounters.xsd` defines the language; `counters2svg.py` validates a set and
writes its sheets as SVG (and PNG through Inkscape). Four instance documents
approximate the counter sheets in `..\*.md`:

| File | What it transcribes | Counters |
|---|---|---|
| `the-russian-campaign.xml` | Compass Games 2020 remake, the complete sheet, **both faces** (`TRC-74-remake\trc_units_sep14_page_1_front.jpg`, `page_2_back_1200px.jpg`) | 214 |
| `panzergruppe-guderian.xml` | the two "new" PDF counter sections, both faces (`PGG_Countersheet_1-2_new.pdf`, `2-2_new.pdf`) | 220 |
| `d-day-at-tarawa.xml` | every US unit on the USMC OOB chart, the Japanese unit types from the rules samples, depth markers, the marker set | 90 |
| `dai-senso.xml` | a representative set: every counter family and every presentation device in the inventory | 67 |

Each renders to `<set>-<sheet>-front.svg/png`, `-back`, and `-both` (front and
back side by side, as the PGG PDFs print them). Validated with libxml2 via
`lxml`, the engine XML Copy Editor uses.

## Design

A counter is a record with two faces. A face is a flat ground with content in
named slots on a unit square:

```
+---------------------------------+
| UL          TC             UR   |    LCOL  a column of step pips down the left
| ML        CENTRE           MR   |    BAND  a full-width strip behind BOTTOM
| LL        BOTTOM           LR   |    free  x, y and any angle (diagonal back text)
+---------------------------------+
```

- **Affiliation is never a frame shape.** It is a `style`: ground, text, box
  stroke and box fill colours, keyed by nationality, formation, side or country.
  A quality flag (Guards, elite, colonial) is a `fill`/`stroke` override on the
  symbol box.
- **CENTRE** holds one of: `symbol` (a plain rectangle with a branch `icon` and
  `modifiers`), `silhouette`, `emblem`, or a date `tile`. Symbol box and
  silhouette are two renderings of one field, as TRC 5th edition says outright.
- **BOTTOM** is the `value` line, the largest text: any hyphenated tokens
  (`8-7`, `3-3-1`, `4-U`, `Hero RD`, `(3)★10`).
- `echelon` draws the APP-6 amplifier above the box; `steps` draws pips whose
  shape (`dot`/`square`) carries meaning; `glyph` covers target shapes, position
  discs, range circles, DRM squares, arrows, earmarks and hexagon outlines;
  `band` is the delay stripe / caption band / type label; `split` paints the
  upper-right triangle a second colour.
- **The back is a variant**: a full face, `derived="reduced|concealed|same"`
  from the front, or `ref` to another counter (Dai Senso's Air Force / Troop
  Convoy, CV Fleet / CV Strike, airborne / Airdrop).
- **Sheets**: a grid of `place` entries; the back sheet is the same grid
  `mirror`ed (horizontal or vertical, to suit the duplex flip).

## The two points to watch

**Corners.** Icon geometry is drawn with the right primitives: the armour icon is
a *stadium* (a rectangle with fully rounded ends, `rx = height/2`), never a
rectangle; the symbol box always has square corners; the tile corner radius is
one uniform `corner` fraction per set (0 for square counters). Nothing about
the sources' bevels, textures or fonts is copied.

**Registration.** Front and back come from one grid function in the renderer:
the back places each counter at the mirrored cell of its front, the crop marks
are drawn at every grid line in both margins, and a registration cross sits in
each corner — all at identical coordinates on both faces, in millimetres, so a
duplex print cuts true. `mirror="horizontal"` is for a sheet turned about its
vertical edge (the usual long-edge flip on a landscape page); use `vertical`
for the other flip.

## Element map

```
counters  @id @title @source @size(mm) @corner @font
  palette  > color*  (@id @value)
  styles   > style*  (@id @ground @text @box-stroke @box-fill)
  counter* (@id @family=unit|support|leader|marker @name @count)
     front (Face)          back (Face | @derived | @ref)
  sheet*   (@cols @rows @gutter @margin @mirror @crop-marks) > place* (@counter | @blank, @repeat)

Face  @style @ground @text @split ; children in any order:
  symbol     @icon @modifiers @fill @stroke @partial @text @scale
  silhouette @kind @color @href @scale         emblem @kind @color @color2 @slot @scale
  echelon    @level                            value  @color @size @anchor  (text)
  text       @slot @x @y @rotate @size @color @weight @italic  (text, "|" = new line)
  steps      @count @max @mark @slot @color    glyph  @kind @slot @color @text @x @y
  band       @color @text-color @height (text) tile   @fill @color (text, "|" = new line)
```

All ids are `xs:ID` and share one namespace — a colour, a style and a counter
may not reuse a name (`white` the colour vs `plain` the style).

## Honest limits

- PGG's PDFs are raster; both sections were transcribed by eye, so a few tiny
  designations and corps codes may be off. TRC's designations and the
  front-to-back pairing were checked by the mirror (Stalin's back reads Moscow,
  the Rumanian corps' backs read Rumania).
- Tarawa's reduced-face values and weapon lists are illustrative (the rules
  give the mechanism, not every number); Japanese counters are types, not the
  full set.
- Silhouettes are simple built-in shapes; a real project supplies SVG via
  `silhouette/@href`.

## Build and render

```bash
python counters2svg.py the-russian-campaign.xml --png --dpi 300
```

`tools\build_*.py` regenerate the XML from compact tables.
