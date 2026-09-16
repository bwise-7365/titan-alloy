# Copyright Ben Paul Wise. All Rights Reserved.
"""network_check.py -- report and check the networks and hexside lines of hexsheet map sheets.

    python network_check.py SHEET.xml [SHEET.xml ...] [--quiet]

Prints every piece of every hexside line (river, border ...) and of every link kind (rail, road ...)
with its size and ends, then every acceptance rule the sheet breaks. Exits 1 when a sheet breaks a
rule, 0 otherwise. --quiet prints only the per-kind totals and the broken rules.

Each sheet id has a rule profile (PROFILES): a rule for each of its line and link kinds (None: reported,
not checked), a rule for link steps, and rules for the sheet as a whole. A sheet id, line kind or link kind
without an entry is an error, never a silent pass.

TRC and PGG (tasks/08-map-networks.md):
  links  every step joins neighbouring hexes; no link enters a sea or lake hex, except a city hex
         (a port the sheet classifies as water); no step crosses a blocked hexside
  road   exactly one piece, reaching every named place (city-major, city-minor, city, capital, town)
  rail   every major city (city-major, capital) on the network; one piece, except further pieces of
         at least 8 hexes that run off a map edge
  river  no piece of 5 or fewer hexsides; every piece drains (a vertex touches sea or lake or lies on
         the map edge); no river hexside with sea or lake on both sides or on the map edge
  border no hexside in water; a continuous chain whose loose ends lie on sea, a lake or the map edge
Dai Senso (tasks/09-ds-map.md). The map edge includes the hexes outside the printed map (DS_OFFMAP); the
west and east grids are neighbours across their seam.
  links     every step joins neighbouring hexes; no hex in sea or off the map
  rail      no piece under 3 hexes (DS_SHORT excepted); each piece, with the connected straits DS_STRAITS as
            joins, holds exactly one hex of DS_RAIL_SYSTEMS (the print's separate rail systems, named)
  road      no piece under 3 hexes (DS_SHORT excepted)
  network   rail, road and connected straits form the print's networks, each piece holding exactly one hex of
            DS_NETWORKS, and reach every capital and city (DS_UNLINKED excepted); the sheet has rail. Networks
            are named, not derived from land: at hex scale coastal hexes touch across straits the print shows
            as open water
  border, zone, region  no hexside on the map edge; no piece under 3 hexsides; every loose end lies on the
            map edge, on another of these boundary kinds, or on a coast (a corner shared by land and sea)
  mountain  between two land hexes; no piece under 3 hexsides (DS_SPECKS excepted)
  river     between two land hexes; every piece touches a coast, a lake or the map edge
  lake      between two land hexes
  places    capitals, cities and towns on land; ports on land with a sea neighbour
"""
import collections
import os
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HERE)
import hexsheet2svg as H  # noqa: E402
from lxml import etree  # noqa: E402

WATER = ("sea", "lake")
MAJOR = ("city-major", "capital")
PLACES = ("city-major", "capital", "city", "city-minor", "town")
RAIL_MIN = 8
RIVER_MAX_SHORT = 5
BORDER_MIN = 3
BLOCKING_LINES = ("blocked",)
RANGE_MIN = 3
LINK_MIN = 3
BOUNDARIES = ("border", "zone", "region")
NAMED = ("capital", "city")

# ---------------------------------------------------------------- Dai Senso data
# Hexes outside the printed map (paper margin and off-map furniture), per grid row, west to east. Read off the
# game-map scan: paper or white round the hex centre, or the centre inside an off-map panel of the sheet.
DS_OFFMAP = """
w6101-w6127 w6001-w6010 w5901-w5911 w5801-w5809 w5701-w5708 w5601-w5609 w5501-w5508 w5401-w5410 w5301-w5311
w5201-w5209 w5101-w5109 w5001-w5008 w4901-w4907 w4801-w4806 w4705 w4601-w4604 w4401-w4402 w4301-w4302 w4201
w4101 w3901 w3701 w3601 w3501-w3505 w3401-w3404 w3301-w3305 w3201-w3204 w3101-w3105 w2901-w2905 w2801-w2804
w2701-w2713 w2601-w2613 w2501-w2514 w2401-w2413 w2301-w2304 w2306-w2314 w2201-w2215 w2101-w2116 w2001-w2015
w1901-w1916 w1801-w1815 w1701-w1716 w1601-w1615 w1501-w1515 w1517-w1520 w1401-w1420 w1301-w1322 w1201-w1219
w1101-w1122 w1001-w1021 w1024-w1027 w0901-w0927
e6101-e6129 e6012-e6029 e5915-e5929 e5814-e5821 e5829 e5717-e5719 e5725-e5729 e5625-e5629 e5525 e5425-e5429
e5325-e5329 e5225-e5229 e5126-e5129 e5025-e5029 e4925-e4929 e4829 e4729 e4629 e4529 e4429 e4329 e4229 e4129
e4029 e3929 e3829 e3729 e3629 e3529 e3429 e3325-e3329 e3225-e3229 e3124 e3126-e3129 e3023 e3025-e3029
e2923-e2924 e2926-e2929 e2822 e2829 e2722-e2729 e2622-e2629 e2522-e2529 e2422-e2429 e2322-e2329 e2222-e2229
e2122-e2129 e2022-e2029 e1922-e1929 e1828-e1829 e1723-e1729 e1622-e1629 e1523-e1529 e1422-e1429 e1320 e1329
e1220-e1229 e1121-e1129 e1001-e1029 e0901-e0929
"""
# Hex pairs joined by a connected strait arrow; they count as joined for the rail and network rules.
DS_STRAITS = "e4801-e4902 e4802-e4803 e4802-e4902 e5105-e5205 e1317-e1318"
# Capitals and cities the print leaves off every rail and road line: Lhasa, Kyzyl Khoto, Yenan, Petropavlovsk,
# Colombo; Borneo (Kuching, Brunei, Balikpapan, Sandakan, Tarakan); Celebes (Menado, Kendari, Makassar); New Guinea
# (Hollandia, Wewak, Lae, Sorong, Port Moresby); cities alone on their islands: Davao (Mindanao), Rabaul (New
# Britain), Honolulu (Oahu).
DS_UNLINKED = ("w4715 w5818 w5021 e5810 w3509 w3220 w3322 w3022 w3423 w3223 w3126 w2925 w2824 e3004 e2906 e2707 "
               "e3001 e2606 w3426 e2808 e4226")
# Printed mountain hexsides shorter than a range (canonical hexside refs, all of a piece's sides listed).
DS_SPECKS = ""
# Printed rail or road pieces shorter than 3 hexes (all of a piece's hexes listed): rail Palembang-Telukbetung,
# Taihoku-Tainan, the South Island line; road Ledo-Myitkyina, Kweilin-Nanning.
DS_SHORT = "w2919 w2818 w4325 w4224 e1217 e1317 w4516 w4416 w4420 w4320"
# One hex of each printed network (rail, road and connected straits together): Asia, Sumatra, Java, Japan, Formosa,
# Luzon, Australia, New Zealand, Sakhalin. Hex adjacency joins islands the print keeps apart (Malacca, Sunda, Korea,
# Formosa, Tatar, La Perouse and Palk straits), so the networks are named rather than derived from land.
DS_NETWORKS = "w6012 w2919 w2819 e4905 w4325 w3824 w1423 e1517 e5806"
# One hex of each printed rail system: Russia-Manchuria-China-Korea, India, Burma, Indochina-Siam-Malaya, Sumatra,
# Java, Japan, Formosa, Luzon, Australia, New Zealand.
DS_RAIL_SYSTEMS = "w6012 w4405 w4215 w3817 w2919 w2819 e4905 w4325 w3824 w1423 e1517"
# Printed ports whose water lies inside their own hex (a delta), with no all-sea neighbour: pid:slot facing the water.
DS_DELTA_PORTS = "w4314:sw"


class Corners:
    """Hex corners as hashable keys. The same corner computed from two hexes differs by float noise,
    so a plain round() can split it in two; a corner within a small tolerance of one already seen
    takes that one's key. A key is the first point seen, in half pixels."""

    def __init__(self, size):
        self.cell = size / 4.0
        self.tol = size * 0.01
        self.grid = collections.defaultdict(list)

    def key(self, p):
        cx, cy = int(p[0] // self.cell), int(p[1] // self.cell)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for q, k in self.grid[(cx + dx, cy + dy)]:
                    if abs(q[0] - p[0]) <= self.tol and abs(q[1] - p[1]) <= self.tol:
                        return k
        k = (round(p[0] * 2), round(p[1] * 2))
        self.grid[(cx, cy)].append((p, k))
        return k


class Sheet:
    """The geometry, terrain, places, hexside lines and links of one sheet."""

    def __init__(self, path):
        self.path = path
        self.root = etree.parse(path).getroot()
        self.id = self.root.get("id")
        self.grids = [H.Grid(g) for g in self.root.findall("grid")]
        self.cell = {}
        for gi, g in enumerate(self.grids):
            for pid, (c, r) in g.ids.items():
                self.cell[pid] = (gi, c, r)
        self.terrain = self.read_terrain()
        self.places = self.read_places()
        self.glyphs = self.read_glyphs()
        self.build_sides()
        self.offmap = self.id_ranges(profile(self)["offmap"])
        self.lines = self.read_edge_lines()
        self.links = self.read_links()

    # ---- reading
    def read_terrain(self):
        out = {}
        for g in self.grids:
            for pid in g.ids:
                out[pid] = g.terrain
        for h in self.root.findall("hexes"):
            for pid in h.get("ids").split():
                if pid in self.cell:          # the renderer skips ids outside the grid (a warning there)
                    out[pid] = h.get("terrain")
        for h in self.root.findall("hex"):
            if h.get("terrain"):
                out[self.known(h.get("id"))] = h.get("terrain")
        return out

    def read_places(self):
        out = {}
        for h in self.root.findall("hex"):
            for gl in h.findall("glyph"):
                if gl.get("symbol") in PLACES:
                    out[self.known(h.get("id"))] = gl.get("symbol")
        return out

    def read_glyphs(self):
        """pid -> every glyph symbol in the hex."""
        out = collections.defaultdict(set)
        for h in self.root.findall("hex"):
            for gl in h.findall("glyph"):
                out[self.known(h.get("id"))].add(gl.get("symbol"))
        return out

    def id_ranges(self, text):
        """Hex ids from tokens 'w6101' or 'w6101-w6110' (a run along one grid row, west to east)."""
        out = set()
        for tok in text.split():
            a, _, b = tok.partition("-")
            ga, ca, ra = self.cell[self.known(a)]
            gb, cb, rb = self.cell[self.known(b or a)]
            if (ga, ra) != (gb, rb) or cb < ca:
                raise ValueError("%s: bad hex range %s" % (self.path, tok))
            out.update(self.grids[ga].cells[(c, ra)] for c in range(ca, cb + 1))
        return out

    def read_edge_lines(self):
        out = collections.defaultdict(dict)
        for e in self.root.findall("edge"):
            if e.get("line"):
                pid, d = e.get("at").split(":")
                out[e.get("line")][self.side_of(self.known(pid), d)] = e.get("at")
        return out

    def read_links(self):
        out = collections.defaultdict(list)
        for lk in self.root.findall("link"):
            out[lk.get("kind")].append([self.known(p) for p in lk.get("hexes").split()])
        return out

    def known(self, pid):
        if pid not in self.cell:
            raise ValueError("%s: unknown hex %s" % (self.path, pid))
        return pid

    # ---- geometry
    def build_sides(self):
        self.side_hexes = collections.defaultdict(list)   # side -> [pid] (1 or 2)
        self.side_ends = {}                                # side -> (vertex, vertex)
        self.side_ref = {}                                 # side -> canonical "pid:dir"
        self.vertex_hexes = collections.defaultdict(set)   # vertex -> {pid}
        self.vertex_sides = collections.defaultdict(set)   # vertex -> {side}
        self.hex_sides = {}                                # pid -> [side] in ring order
        self.corners = Corners(min(g.size for g in self.grids))
        for pid in sorted(self.cell, key=self.order):
            g, c, r = self.grid_of(pid)
            ring = []
            for d in g.geom["edges"]:
                a, b = g.edge_ends(c, r, d)
                va, vb = self.corners.key(a), self.corners.key(b)
                side = tuple(sorted((va, vb)))
                ring.append(side)
                self.side_hexes[side].append(pid)
                self.side_ends[side] = (va, vb)
                self.side_ref.setdefault(side, "%s:%s" % (pid, d))
                for v in (va, vb):
                    self.vertex_hexes[v].add(pid)
                    self.vertex_sides[v].add(side)
            self.hex_sides[pid] = ring

    def order(self, pid):
        return self.cell[pid]

    def grid_of(self, pid):
        gi, c, r = self.cell[pid]
        return self.grids[gi], c, r

    def side_of(self, pid, d):
        g, c, r = self.grid_of(pid)
        if d not in g.geom["edges"]:
            raise ValueError("%s: direction %s not valid at %s" % (self.path, d, pid))
        a, b = g.edge_ends(c, r, d)
        return tuple(sorted((self.corners.key(a), self.corners.key(b))))

    def neighbours(self, pid):
        """[(dir index, neighbour pid)] in ring order: the hexes sharing a hexside, so grids that sit
        on one lattice (Dai Senso's west and east maps) are neighbours across their seam."""
        return [(i, other) for i, side in enumerate(self.hex_sides[pid])
                for other in self.side_hexes[side] if other != pid]

    def step_side(self, a, b):
        for i, n in self.neighbours(a):
            if n == b:
                return self.hex_sides[a][i]
        return None

    def centre(self, pid):
        g, c, r = self.grid_of(pid)
        return g.centre(c, r)

    def waterP(self, pid):
        return self.terrain[pid] in WATER

    def landP(self, pid):
        return not self.waterP(pid) and pid not in self.offmap

    def boundary_hexP(self, pid):
        return len(self.neighbours(pid)) < 6

    def edge_vertexP(self, v):
        return len(self.vertex_hexes[v]) < 3

    def map_edge_vertexP(self, v):
        return self.edge_vertexP(v) or any(p in self.offmap for p in self.vertex_hexes[v])

    def coast_vertexP(self, v):
        hexes = self.vertex_hexes[v]
        return any(self.landP(p) for p in hexes) and any(self.waterP(p) for p in hexes)

    def outletP(self, v):
        return self.edge_vertexP(v) or any(self.waterP(p) for p in self.vertex_hexes[v])

    def river_sideP(self, side):
        hexes = self.side_hexes[side]
        return len(hexes) == 2 and not any(self.waterP(p) for p in hexes)

    def inner_sideP(self, side):
        hexes = self.side_hexes[side]
        return len(hexes) == 2 and not any(p in self.offmap for p in hexes)

    def land_sideP(self, side):
        hexes = self.side_hexes[side]
        return len(hexes) == 2 and all(self.landP(p) for p in hexes)

    def blocked_sides(self):
        out = set()
        for line in BLOCKING_LINES:
            out.update(self.lines.get(line, {}))
        return out


# ---------------------------------------------------------------- pieces
def side_pieces(sheet, sides):
    """Connected pieces of a set of hexsides, largest first: [(sides, ends)]."""
    adj = collections.defaultdict(set)
    for s in sides:
        va, vb = sheet.side_ends[s]
        adj[va].add(s)
        adj[vb].add(s)
    seen = set()
    out = []
    for s0 in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if s0 in seen:
            continue
        piece, stack = set(), [s0]
        while stack:
            s = stack.pop()
            if s in piece:
                continue
            piece.add(s)
            for v in sheet.side_ends[s]:
                stack.extend(adj[v] - piece)
        seen |= piece
        ends = sorted(v for v in {v for s in piece for v in sheet.side_ends[s]} if len(adj[v]) == 1)
        out.append((piece, ends))
    out.sort(key=lambda pe: (-len(pe[0]), min(sheet.side_ref[s] for s in pe[0])))
    return out


def link_graph(chains):
    adj = collections.defaultdict(set)
    for hexes in chains:
        for a, b in zip(hexes, hexes[1:]):
            adj[a].add(b)
            adj[b].add(a)
    return adj


def link_pieces(sheet, adj):
    """Connected pieces of a hex graph, largest first: [(hexes, ends)]."""
    seen = set()
    out = []
    for h0 in sorted(adj, key=sheet.order):
        if h0 in seen:
            continue
        piece, stack = set(), [h0]
        while stack:
            h = stack.pop()
            if h in piece:
                continue
            piece.add(h)
            stack.extend(adj[h] - piece)
        seen |= piece
        ends = sorted((h for h in piece if len(adj[h]) == 1), key=sheet.order)
        out.append((piece, ends))
    out.sort(key=lambda pe: (-len(pe[0]), sheet.order(min(pe[0], key=sheet.order))))
    return out


def vertex_name(sheet, v, piece):
    """A vertex named by one of the piece's hexsides that ends there."""
    return min(sheet.side_ref[s] for s in piece if v in sheet.side_ends[s])


def piece_name(sheet, piece, ends):
    return vertex_name(sheet, ends[0], piece) if ends else min(sheet.side_ref[s] for s in piece)


# ---------------------------------------------------------------- rules: TRC and PGG
def link_problems(sheet, kind, chains):
    out = []
    blocked = sheet.blocked_sides()
    for hexes in chains:
        for a, b in zip(hexes, hexes[1:]):
            side = sheet.step_side(a, b)
            if side is None:
                out.append("%s step %s-%s does not join neighbouring hexes" % (kind, a, b))
            elif side in blocked:
                out.append("%s step %s-%s crosses a blocked hexside" % (kind, a, b))
        for h in hexes:
            if sheet.waterP(h) and h not in sheet.places:
                out.append("%s enters %s hex %s" % (kind, sheet.terrain[h], h))
    return sorted(set(out))


def rail_problems(sheet, kind, pieces, adj):
    out = []
    for pid, sym in sorted(sheet.places.items(), key=lambda kv: sheet.order(kv[0])):
        if sym in MAJOR and pid not in adj:
            out.append("rail misses major city %s" % pid)
    for hexes, ends in pieces[1:]:
        if len(hexes) < RAIL_MIN or not any(sheet.boundary_hexP(h) for h in hexes):
            out.append("rail piece of %d hexes (%s) is separate and short or inland"
                       % (len(hexes), " ".join(ends[:4]) or min(hexes, key=sheet.order)))
    return out


def road_problems(sheet, kind, pieces, adj):
    out = []
    if len(pieces) != 1:
        out.append("road has %d pieces, not one" % len(pieces))
    for pid in sorted(sheet.places, key=sheet.order):
        if pid not in adj:
            out.append("road misses named place %s" % pid)
    return out


def river_problems(sheet, line, sides, pieces):
    out = []
    for s in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if not sheet.river_sideP(s):
            out.append("river hexside %s lies in water or on the map edge" % sheet.side_ref[s])
    for piece, ends in pieces:
        name = piece_name(sheet, piece, ends)
        if len(piece) <= RIVER_MAX_SHORT:
            out.append("river piece of %d hexsides at %s is short" % (len(piece), name))
        verts = {v for s in piece for v in sheet.side_ends[s]}
        if not any(sheet.outletP(v) for v in verts):
            out.append("river piece of %d hexsides at %s does not drain" % (len(piece), name))
    return out


def border_problems(sheet, line, sides, pieces):
    """A country border is a continuous chain (or ring) whose loose ends lie on sea, a lake or the map
    edge; where borders meet they share a vertex and so are one piece."""
    out = []
    for s in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if not sheet.river_sideP(s):
            out.append("border hexside %s lies in water or on the map edge" % sheet.side_ref[s])
    for piece, ends in pieces:
        name = piece_name(sheet, piece, ends)
        if len(piece) < BORDER_MIN:
            out.append("border piece of %d hexsides at %s is a stub" % (len(piece), name))
        for v in ends:
            if not sheet.outletP(v):
                out.append("border piece of %d hexsides ends inland at %s" % (len(piece), vertex_name(sheet, v, piece)))
    return out


# ---------------------------------------------------------------- rules: Dai Senso
def ds_boundary_problems(sheet, line, sides, pieces):
    """A boundary (country or dependent border, naval zone border, region border) is a continuous chain,
    over land or open water, whose loose ends lie on the map edge, on another boundary kind or on a coast."""
    out = []
    others = {v for kind in BOUNDARIES if kind != line for s in sheet.lines.get(kind, {}) for v in sheet.side_ends[s]}
    for s in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if not sheet.inner_sideP(s):
            out.append("%s hexside %s lies on the map edge" % (line, sheet.side_ref[s]))
    for piece, ends in pieces:
        if len(piece) < BORDER_MIN:
            out.append("%s piece of %d hexsides at %s is a stub" % (line, len(piece), piece_name(sheet, piece, ends)))
        for v in ends:
            if not (sheet.map_edge_vertexP(v) or v in others or sheet.coast_vertexP(v)):
                where = "sea" if all(sheet.waterP(p) for p in sheet.vertex_hexes[v]) else "land"
                out.append("%s piece of %d hexsides ends in open %s at %s" % (
                    line, len(piece), where, vertex_name(sheet, v, piece)))
    return out


def ds_range_problems(sheet, line, sides, pieces):
    out = []
    specks = set(DS_SPECKS.split())
    for s in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if not sheet.land_sideP(s):
            out.append("%s hexside %s is not between two land hexes" % (line, sheet.side_ref[s]))
    for piece, ends in pieces:
        if len(piece) < RANGE_MIN and not {sheet.side_ref[s] for s in piece} <= specks:
            out.append("%s piece of %d hexsides at %s is a speck" % (line, len(piece), piece_name(sheet, piece, ends)))
    return out


def ds_river_problems(sheet, line, sides, pieces):
    out = []
    for s in sorted(sides, key=lambda s: sheet.side_ref[s]):
        if not sheet.land_sideP(s):
            out.append("%s hexside %s is not between two land hexes" % (line, sheet.side_ref[s]))
    if "river" != line:
        return out
    lakes = {v for s in sheet.lines.get("lake", {}) for v in sheet.side_ends[s]}
    for piece, ends in pieces:
        verts = {v for s in piece for v in sheet.side_ends[s]}
        if not any(sheet.coast_vertexP(v) or sheet.map_edge_vertexP(v) or v in lakes for v in verts):
            out.append("river piece of %d hexsides at %s does not drain" % (len(piece), piece_name(sheet, piece, ends)))
    return out


def ds_link_problems(sheet, kind, chains):
    out = []
    for hexes in chains:
        for a, b in zip(hexes, hexes[1:]):
            if sheet.step_side(a, b) is None:
                out.append("%s step %s-%s does not join neighbouring hexes" % (kind, a, b))
        for h in hexes:
            if not sheet.landP(h):
                out.append("%s enters %s hex %s" % (kind, "off-map" if h in sheet.offmap else sheet.terrain[h], h))
    return sorted(set(out))


def strait_pairs(sheet):
    out = []
    for tok in DS_STRAITS.split():
        a, b = tok.split("-")
        if sheet.step_side(sheet.known(a), sheet.known(b)) is None:
            raise ValueError("%s: strait %s does not join neighbouring hexes" % (sheet.path, tok))
        out.append((a, b))
    return out


def joined(sheet, adj, places=False):
    """A copy of a link graph with the connected straits added: between hexes it holds, or with places=True
    between any strait's two hexes (DS counts a connected strait as a road or rail link)."""
    out = collections.defaultdict(set, {h: set(ns) for h, ns in adj.items()})
    for a, b in strait_pairs(sheet):
        if places or (a in adj and b in adj):
            out[a].add(b)
            out[b].add(a)
    return out


def named_pieces(sheet, head, adj, names):
    """Each piece of adj holds exactly one of the named hexes (one per printed network or rail system), and each
    named hex is on one."""
    out = []
    listed = [sheet.known(p) for p in names.split()]
    placed = set()
    for hexes, _ in link_pieces(sheet, adj):
        held = sorted((p for p in listed if p in hexes), key=sheet.order)
        placed.update(held)
        if not held:
            out.append("%s piece of %d hexes at %s is none of the named printed systems"
                       % (head, len(hexes), min(hexes, key=sheet.order)))
        elif len(held) > 1:
            out.append("%s piece at %s joins named systems %s" % (head, held[0], " ".join(held)))
    out += ["%s misses its named hex %s" % (head, p) for p in listed if p not in placed]
    return out


def short_pieces(sheet, kind, pieces):
    excepted = set(DS_SHORT.split())
    return ["%s piece of %d hexes at %s is short" % (kind, len(hexes), " ".join(ends) or min(hexes, key=sheet.order))
            for hexes, ends in pieces if len(hexes) < LINK_MIN and not hexes <= excepted]


def ds_rail_problems(sheet, kind, pieces, adj):
    return short_pieces(sheet, kind, pieces) + named_pieces(sheet, kind, joined(sheet, adj), DS_RAIL_SYSTEMS)


def ds_road_problems(sheet, kind, pieces, adj):
    return short_pieces(sheet, kind, pieces)


def ds_required_problems(sheet):
    return [] if sheet.links.get("rail") else ["rail links are missing"]


def ds_network_problems(sheet):
    """Rail, road and connected straits form the print's named networks, one piece each, reaching every capital
    and city except the listed unlinked ones (and no listed one is in fact linked)."""
    adj = joined(sheet, link_graph([c for kind in ("rail", "road") for c in sheet.links.get(kind, [])]), places=True)
    out = named_pieces(sheet, "network", adj, DS_NETWORKS)
    unlinked = {sheet.known(p) for p in DS_UNLINKED.split()}
    for pid in sorted(sheet.glyphs, key=sheet.order):
        named = sorted(sheet.glyphs[pid] & set(NAMED))
        if named and pid not in unlinked and pid not in adj:
            out.append("network misses %s %s" % ("/".join(named), pid))
        if pid in unlinked and pid in adj:
            out.append("network reaches %s, listed as unlinked" % pid)
    return out


def ds_place_problems(sheet):
    out = []
    delta = {tok.split(":")[0] for tok in DS_DELTA_PORTS.split()}
    for pid in sorted(sheet.glyphs, key=sheet.order):
        for sym in sorted(sheet.glyphs[pid] & set(PLACES + ("port",))):
            if not sheet.landP(pid):
                out.append("%s at %s is on a %s hex" % (sym, pid, "off-map" if pid in sheet.offmap else sheet.terrain[pid]))
            elif "port" == sym and pid not in delta and not any(sheet.waterP(n) for _, n in sheet.neighbours(pid)):
                out.append("port at %s has no sea neighbour" % pid)
    return out


# ---------------------------------------------------------------- PGG printed exceptions
# The PGG sheet is built from the print by map_graphics/xml/tools/image2sheet (tasks/12-pgg-map.md). A broken
# rule that the print itself shows is named here with its reason, and reported as EXCEPTED; an exception that
# no longer occurs is itself a broken rule, so the list stays exact.
PGG_EXCEPTED = {
    "river hexside 5802:se lies in water or on the map edge":
        "printed: the river loops round 5903 from the east edge along the lake hex 5802's se hexside, 45 px from "
        "the lake outline (look/res/r-5802.jpg)",
    "river piece of 4 hexsides at 5902:s is short": "printed: the short loop round 5903 at the east edge",
    "river hexside 1601:s lies in water or on the map edge":
        "printed: the river leaves the lake down the shore of 1602, lake by Ben's label (fill 0.34)",
    "river hexside 1602:ne lies in water or on the map edge":
        "printed: the river leaves the lake down the shore of 1602, lake by Ben's label (fill 0.34)",
    "river hexside 1602:se lies in water or on the map edge":
        "printed: the river leaves the lake down the shore of 1602, lake by Ben's label (fill 0.34)",
    "rail misses major city 0424": "printed: the railway reaches Mogilev's other hex 0525, not 0424",
    "road has 11 pieces, not one": "printed: roads end at a dot beside rivers and cities; no road crosses a river",
    "road misses named place 0412": "printed: Vitebsk's road reaches its other hex 0513",
    "road misses named place 0424": "printed: no road reaches Mogilev",
    "road misses named place 0525": "printed: no road reaches Mogilev",
    "road misses named place 1427": "printed: no road reaches the town Кричев",
    "road misses named place 1524": "printed: no road reaches the town Мстиславль",
    "road misses named place 3005": "printed: no road reaches the town Белый",
    "road misses named place 3901": "printed: no road reaches Rzhev (railway only)",
    "road misses named place 4607": "printed: Gzhatsk's road ends at a dot short of its blocks",
    "road misses named place 5921": "printed: no road reaches Kaluga (railway only)",
}


def pgg_excepted(problems, emit):
    """Drop the named printed exceptions, and report any named exception that no longer occurs."""
    out = []
    for p in problems:
        if p in PGG_EXCEPTED:
            emit("EXCEPTED: %s (%s)" % (p, PGG_EXCEPTED[p]))
        else:
            out.append(p)
    out += ["named exception no longer occurs: %s" % p for p in PGG_EXCEPTED if p not in problems]
    return out


# ---------------------------------------------------------------- profiles
PROFILES = {
    "trc": dict(offmap="", steps=link_problems, whole=(),
                lines={"river": river_problems, "border": border_problems, "blocked": None},
                links={"rail": rail_problems}),
    "pgg": dict(offmap="", steps=link_problems, whole=(),
                lines={"river": river_problems},
                links={"rail": rail_problems, "road": road_problems},
                excepted=pgg_excepted),
    "dai-senso": dict(offmap=DS_OFFMAP, steps=ds_link_problems,
                      whole=(ds_required_problems, ds_network_problems, ds_place_problems),
                      lines={"border": ds_boundary_problems, "zone": ds_boundary_problems,
                             "region": ds_boundary_problems, "mountain": ds_range_problems,
                             "river": ds_river_problems, "lake": ds_river_problems},
                      links={"rail": ds_rail_problems, "road": ds_road_problems}),
}


def profile(sheet):
    if sheet.id not in PROFILES:
        raise ValueError("%s: no network rules for sheet id %s" % (sheet.path, sheet.id))
    return PROFILES[sheet.id]


def kind_rule(sheet, table, kind):
    if kind not in table:
        raise ValueError("%s: no network rule for %s on sheet %s" % (sheet.path, kind, sheet.id))
    return table[kind]


def check(sheet, quiet=False, emit=print):
    """Report the sheet's networks; return the list of broken rules."""
    rules = profile(sheet)
    problems = []
    emit("== %s" % os.path.basename(sheet.path))
    for line in sorted(sheet.lines):
        rule = kind_rule(sheet, rules["lines"], line)
        sides = set(sheet.lines[line])
        pieces = side_pieces(sheet, sides)
        emit("edge line %-8s hexsides %4d  pieces %3d  largest %s" % (
            line, len(sides), len(pieces), [len(p) for p, _ in pieces[:6]]))
        if not quiet:
            for piece, ends in pieces:
                emit("    %4d hexsides  ends %s" % (len(piece), " ".join(vertex_name(sheet, v, piece) for v in ends)))
        if rule is not None:
            problems += rule(sheet, line, sides, pieces)
    for kind in sorted(sheet.links):
        rule = kind_rule(sheet, rules["links"], kind)
        adj = link_graph(sheet.links[kind])
        pieces = link_pieces(sheet, adj)
        emit("link kind %-8s hexes %4d  pieces %3d  largest %s" % (
            kind, len(adj), len(pieces), [len(p) for p, _ in pieces[:6]]))
        if not quiet:
            for hexes, ends in pieces:
                emit("    %4d hexes  ends %s" % (len(hexes), " ".join(ends)))
        problems += rules["steps"](sheet, kind, sheet.links[kind])
        if rule is not None:
            problems += rule(sheet, kind, pieces, adj)
    for rule in rules["whole"]:
        problems += rule(sheet)
    if "excepted" in rules:
        problems = rules["excepted"](problems, emit)
    for p in problems:
        emit("BROKEN: " + p)
    emit("%s: %d broken rules" % (os.path.basename(sheet.path), len(problems)))
    return problems


def utf8_output():
    """Say our output is UTF-8. ctest takes it through a pipe, and Windows then encodes it as the console
    code page, which has no Cyrillic: the PGG sheet's place names threw UnicodeEncodeError."""
    sys.stdout.reconfigure(encoding="utf-8")
    return


def main(argv):
    utf8_output()
    paths = [a for a in argv[1:] if not a.startswith("--")]
    if not paths:
        print(__doc__)
        return 2
    broken = 0
    for path in paths:
        broken += len(check(Sheet(path), "--quiet" in argv))
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
