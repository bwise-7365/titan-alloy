Copyright Ben Paul Wise. All Rights Reserved.

# Proposal: explicit junctions for link networks in hexsheet.xsd

Status: APPLIED 2026-09-23 on Ben's instruction (his decisions on the three open points are in
section 7; his slide-14 rule, that connected intersections drawn inside one hex form ONE junction, is
in the XSD documentation). Section 8 lists what was changed. Source of the design:
Ben's briefing `doc/Linear Features trimmed.pptx`, slides 5-8 and 13-22; the motivating cases are on
the Velikiye Luki sheet (`map_graphics/xml/velikiye-luki.xml`) and beside Leningrad on The Russian
Campaign sheet.

## 1. The problem

A `link` is an ordered chain of hex centres. Two links that pass through one hex therefore share that
hex as a node: a unit may switch from one to the other there, and the renderer draws them meeting.
The maps disagree:

- VL 0705: two roads cross the hex (0605-0705-0805 and 0604-0705-0805) and do not meet.
- VL 0805 (Dokukino): three separate intersections in one hex.
- VL 0403: two arcs through the hex that never meet.
- TRC: two hexes beside Leningrad each carry two parallel, non-intersecting roads.

## 2. Ben's data structure (from the briefing)

- A network line (his RRLine, our `link`) is a named ordered list of cells (slide 8: "list cells in
  order, to simplify recording and validation").
- Each cell holds the SET of lines in it (derivable from the lists) and the SET of junctions in it; a
  junction is a set of line names between which a unit may switch freely (slides 5, 6).
- Several junctions may lie in one cell: {A,B,C} and {X,Y,Z} in "3,3" are two junctions; switching
  A to X is not allowed (slide 6). No line may be in two junctions of the same cell (slide 14).
- Movement H1 -> H2 is along the network iff some line appears in both cells and the two cells are
  consecutive on it (slide 5, 15-17). Switching lines happens only at a junction (slide 5).
- The readable file and the runtime structure differ: file = lists of cells per line plus junctions
  per cell; runtime = `Map<GridCell, Set<Line>>` (arcs) and `Map<GridCell, Set<Junction>>` (nodes)
  (slides 8, 13). Names are strings to avoid circular references (slide 8).
- Consistency checks (slide 8): consecutive cells adjacent; every line at least two cells; the
  junction in each end cell of a line must include that line (when the line ends at a junction).

## 3. Proposed XSD change (hexsheet.xsd)

Three small additions; every existing instance keeps validating once one attribute is added to it.

```xml
<!-- sheet: the document chooses (as with @urban), no default -->
<xs:attribute name="junctions" use="required">
  <xs:annotation><xs:documentation>implicit: two links that share a hex meet there (the rule so far;
  the five older sheets). explicit: links meet only where a junction element says so; a shared hex
  without a junction is a crossing without connection.</xs:documentation></xs:annotation>
  <xs:simpleType><xs:restriction base="xs:string">
    <xs:enumeration value="implicit"/><xs:enumeration value="explicit"/>
  </xs:restriction></xs:simpleType>
</xs:attribute>

<!-- Link gains an id, so junctions can name it (Path already has one) -->
<xs:attribute name="id" type="xs:ID"/>   <!-- required by the loader when junctions="explicit" -->

<!-- new element in the sheet's content choice: a node of ONE network -->
<xs:complexType name="Junction">
  <xs:annotation><xs:documentation>A set of chains of one kind between which movement may switch
  freely. For links (road, rail) the junction sits in a hex (at = HexRef, links = the link ids); for
  paths (rivers) it sits at a hex corner where the hexsides meet (at = VertexRef, paths = the path ids).
  Exactly one of links and paths is given. All members have the same kind: a road and a railway never
  share a node (a unit that changes from road to rail movement in a hex does so by a rule about the
  hex, not by a node of the map). A hex or vertex may hold several junctions; a chain is in at most one
  junction at a given hex or vertex.</xs:documentation></xs:annotation>
  <xs:attribute name="at" use="required">
    <xs:simpleType><xs:union memberTypes="HexRef VertexRef"/></xs:simpleType>
  </xs:attribute>
  <xs:attribute name="links" type="xs:IDREFS"/>
  <xs:attribute name="paths" type="xs:IDREFS"/>
  <xs:attribute name="name" type="xs:string"/>
</xs:complexType>
```

Paths (rivers) get the same treatment as links: in `explicit` mode two paths that share a vertex join
only where a junction at that vertex says so; otherwise they pass (Ben: there are maps where two minor
rivers pass through one hex without meeting). A path junction is what a rules writer would call a
confluence.

Loader and checker rules (XSD 1.0 cannot say them):
1. `junctions="explicit"` requires every `link` and every `path` to carry `@id`.
2. Every member of a junction passes through `@at` (a link through the hex; a path along a hexside
   ending at the vertex); a junction names at least two distinct members, all of one `kind`.
3. A chain appears in at most one junction at a given hex or vertex.
4. `@ends` "junction" at an end requires a junction at that end (hex for a link, an end vertex for a
   path) that includes the chain.
5. Consecutive hexes of a link are adjacent; consecutive hexsides of a path share a vertex; every
   chain has at least two members (already checked).
6. In `implicit` mode the document must contain no `junction` element.
7. A junction never mixes kinds (road with rail, river with border); the loader rejects it.

## 4. Movement semantics for the engine (slides 15-22, restated)

Carry a SET of candidate lines instead of one current line; this replaces the "one-hex look ahead"
of slides 18-21 and gives slide 22's "ambiguous final state" for free.

- State: (hex H, candidates C). At the start of a move C = all lines of the network that pass
  through H (slide 22: a new turn starts with no current line).
- Step H -> H': let S = { L in C : H and H' consecutive on L }. If S is non-empty the step is along
  the network and C' = S.
- Switching: before the step, C may be enlarged: for each junction J in H with J meets C non-empty,
  add all lines of J to C. (Slide 5: free switch only at a junction; slide 6: only within one junction.)
- If S is empty the step is off the network (slides 16, 17, 21 "4-5 has intersection {}").
- The move ends with C possibly holding several lines (slide 22); nothing more is decided.

Runtime structures as in slide 8: arcs `Map<HexIndex, Set<LinkId>>`, nodes `Map<HexIndex,
Set<Junction>>`, both built once from the sheet by `hexmodel::BoardBuilder`; `hexsearch::NetworkGraph`
gets a state (hex, candidate set) instead of a hex.

## 5. Rendering (hexsheet2svg.py, hexview later)

- `implicit`: as today; chains of one kind are merged at shared hexes.
- `explicit`: each link is its own chain, drawn midpoint to midpoint through its hexes and to the
  centre at its ends; the members of a junction are joined at the hex centre. Two links crossing a hex
  without a junction cross on the drawing as they do on the printed map (the map draws them apart
  inside the hex; the language does not record sub-hex geometry, slide 14: nothing smaller than
  centre to centre).

## 6. The first instance (Velikiye Luki, sketch)

```xml
<sheet ... junctions="explicit">
  <link id="rd-east-bank" kind="road" hexes="0604 0605 0606 0607 0508 0509" .../>
  <link id="rd-vl-dokukino" kind="road" hexes="0604 0705 0805 0906 R05" .../>
  <link id="rd-0605-0705" kind="road" hexes="0605 0705" .../>
  <link id="rd-velikopole" kind="road" hexes="0705 0804 0803 0802 0801 0800" .../>
  <link id="rd-dokukino-lipets" kind="road" hexes="0805 0706" .../>
  <link id="rd-velikopole-dokukino" kind="road" hexes="0804 0805" .../>
  <link id="rd-kunya" kind="road" hexes="0805 0905 R04" .../>
  <!-- 0705: three roads pass, none meet: no junction element -->
  <junction at="0604" name="Velikiye Luki" links="rd-east-bank rd-vl-dokukino rd-rusanovo rd-demya rl-... "/>
  <junction at="0805" links="rd-vl-dokukino rd-dokukino-lipets"/>
  <junction at="0805" links="rd-velikopole-dokukino rd-kunya"/>
  <!-- the third Dokukino intersection to be read from the map -->
</sheet>
```

The exact membership of the 0805 junctions and every other junction on the sheet is to be read from
the map (Ben) before the instance is converted; the sketch shows the shape only.

## 7. Decisions (Ben, 2026-09-23)

1. A junction never joins road and rail: trains do not run on roads nor trucks on tracks. A unit may
   change from road to rail movement where both are in its hex, but that is a rule about the hex; the
   two networks have no common node. (Check 7 above.)
2. `junctions` is a required sheet attribute; the five older sheets get `junctions="implicit"`.
3. Rivers (paths) need the same mechanism: some maps have two minor rivers through one hex that do not
   meet. Path junctions sit at vertices (section 3).

## 8. What was applied (2026-09-23)

- `hexsheet.xsd`: `Junction` type and `junction` element, `Link/@id`, required `sheet/@junctions`.
- Fifteen sheet instances (the six in map_graphics/xml, five reader/image2sheet work sheets, four C++
  test fixtures) and six generators (`build_*.py`, `assemble.py`, `reader/sheet.py`) say `implicit`.
- `hexsheet2svg.py`: on an explicit sheet each link is its own chain and only junction members meet
  (`link_strokes` takes a node function); implicit sheets render byte-identically to before.
- `tools/validate-xml.py`: `sheet_checks` implements rules 1-7 (the vertex geometry of a path junction
  is left to the loader).
- `HexXml::SheetDoc`: `junctions`, `SheetLinkDoc::id`, `SheetJunctionDoc`, `junctionElements`; an
  implicit sheet with a junction element throws. `HexModel::LinkNetwork`: arcs remember their chain
  (`Link::chain`, `chains()`), nodes are `junctionsAt(hex)`, `switchP(hex, from, to)` answers the
  movement question of section 4; `BoardBuilder` builds both and enforces rules 1-3 and 7 for links.
  Path junctions are parsed and written but not modelled in Board (no river network there yet).
- HexMapEd's `SheetWriter` writes `junctions`, `link/@id` and the junction elements, so an edit
  round-trips them.
- `velikiye-luki.xml` is the first explicit sheet: every link has an id, 17 junctions (13 road, 3 rail),
  no junction at 0705 or 0403. Not yet done: `hexsearch::NetworkGraph` still searches over hexes; a
  search that respects junctions needs the (hex, candidate set) state and is a later engine task.

Copyright Ben Paul Wise. All Rights Reserved.
