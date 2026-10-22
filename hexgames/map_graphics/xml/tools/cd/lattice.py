# Copyright Ben Paul Wise. All Rights Reserved.
"""lattice.py -- the Circling Dragons sheet: projection, grid and hex geometry.

One flat-top lattice, odd columns shifted half a hex down (hexsheet grid offset="odd"), ids
{col:02}{row:02} from the north-west corner.  Centre-to-centre distance is 75 km in every
direction (the standard since 2026-10-06; CD_HEX_KM=100 builds the first standard, 100 km).  The geography is an Albers equal-area conic (standard parallels 27 N and 45 N, origin
35 N 118 E), so every hex is 8660 km^2 wherever it lies.  The centre, corner and neighbour formulas
are those of map_graphics/xml/hexsheet2svg.py (class Grid, FLAT), so a point placed here lands in the
hex the renderer draws there.
"""
import math
import os

R_EARTH = 6371.0
PHI1, PHI2, PHI0, LAM0 = 27.0, 45.0, 35.0, 118.0
SQRT3 = math.sqrt(3)

# ---- Albers equal-area conic -------------------------------------------------------------------
_n = (math.sin(math.radians(PHI1)) + math.sin(math.radians(PHI2))) / 2
_C = math.cos(math.radians(PHI1)) ** 2 + 2 * _n * math.sin(math.radians(PHI1))
_rho0 = R_EARTH * math.sqrt(_C - 2 * _n * math.sin(math.radians(PHI0))) / _n


def fwd(lat, lon):
    """(lat, lon) degrees -> (x, y) km, y northward."""
    theta = _n * math.radians(lon - LAM0)
    rho = R_EARTH * math.sqrt(_C - 2 * _n * math.sin(math.radians(lat))) / _n
    return rho * math.sin(theta), _rho0 - rho * math.cos(theta)


def inv(x, y):
    """(x, y) km -> (lat, lon) degrees."""
    rho = math.hypot(x, _rho0 - y)
    theta = math.atan2(x, _rho0 - y)
    s = (_C - (rho * _n / R_EARTH) ** 2) / (2 * _n)
    s = max(-1.0, min(1.0, s))
    return math.degrees(math.asin(s)), LAM0 + math.degrees(theta / _n)


# ---- sheet layout ------------------------------------------------------------------------------
# The standard sheet is 75 km between centres (Ben, 2026-10-06; it was 100 km, and the hand data are still
# ruled on that lattice).  CD_HEX_KM in the environment builds the same area at another scale: the
# north-west corner and SIZE in px stay, so the
# pixel lattice is the same and only the km-to-px factor, the column and row counts and the cache
# names change.  Hand data keyed by 100 km hex ids are mapped onto the lattice by rescale_ids().
REF_KM, REF_COLS, REF_ROWS = 100.0, 35, 34
DEFAULT_KM = 75.0                                     # the standard sheet
HEX_KM = float(os.environ.get("CD_HEX_KM", DEFAULT_KM))   # centre to centre
SIZE = 46.0                    # circumradius in px
MARGIN_X, MARGIN_Y = 60.0, 60.0
# km extents the lattice covers: x from XMIN eastward, y from YMAX southward
XMIN_KM, YMAX_KM = -1700.0, 1770.0


def dims(hex_km):
    """(cols, rows) covering at least the standard sheet's area at hex_km between centres."""
    if hex_km == REF_KM:
        return REF_COLS, REF_ROWS
    width = (1.5 * (REF_COLS - 1) + 2) * REF_KM / SQRT3     # km; the column pitch is 1.5 / sqrt(3) of hex_km
    height = (REF_ROWS + 0.5) * REF_KM
    cols = int(math.ceil((width * SQRT3 / hex_km - 2) / 1.5 - 1e-9)) + 1
    rows = int(math.ceil(height / hex_km - 0.5 - 1e-9))
    return cols, rows


PX_PER_KM = SQRT3 * SIZE / HEX_KM
COLS, ROWS = dims(HEX_KM)
SUFFIX = "" if HEX_KM == DEFAULT_KM else "-%g" % HEX_KM     # cache and sheet names: "" or "-100"


def cache(name):
    """A cache or diagnostic file name for this scale: geo_hex.json becomes geo_hex-100.json."""
    base, ext = os.path.splitext(name)
    return base + SUFFIX + ext

OX = MARGIN_X + SIZE                 # centre of hex (0, 0) in px
OY = MARGIN_Y + SQRT3 / 2 * SIZE
MAP_W = 1.5 * SIZE * (COLS - 1) + 2 * SIZE
MAP_H = SQRT3 * SIZE * ROWS + SQRT3 / 2 * SIZE
SHEET_W = int(math.ceil(MAP_W + 2 * MARGIN_X))
SHEET_H = int(math.ceil(MAP_H + 2 * MARGIN_Y))

FLAT = dict(
    corners={"e": 0, "se": 60, "sw": 120, "w": 180, "nw": 240, "ne": 300},
    edges={"se": 30, "s": 90, "sw": 150, "nw": 210, "n": 270, "ne": 330},
    edge_corners={"n": ("nw", "ne"), "ne": ("ne", "e"), "se": ("e", "se"),
                  "s": ("se", "sw"), "sw": ("sw", "w"), "nw": ("w", "nw")},
    nbr_shift={"n": (0, -1), "s": (0, 1), "ne": (1, 0), "se": (1, 1), "nw": (-1, 0), "sw": (-1, 1)},
    nbr_plain={"n": (0, -1), "s": (0, 1), "ne": (1, -1), "se": (1, 0), "nw": (-1, -1), "sw": (-1, 0)},
)
DIRS = ("n", "ne", "se", "s", "sw", "nw")
OPPOSITE = {"n": "s", "s": "n", "ne": "sw", "sw": "ne", "nw": "se", "se": "nw"}


def km_to_px(x, y):
    return MARGIN_X + (x - XMIN_KM) * PX_PER_KM, MARGIN_Y + (YMAX_KM - y) * PX_PER_KM


def px_to_km(px, py):
    return XMIN_KM + (px - MARGIN_X) / PX_PER_KM, YMAX_KM - (py - MARGIN_Y) / PX_PER_KM


def ll_to_px(lat, lon):
    return km_to_px(*fwd(lat, lon))


def px_to_ll(px, py):
    return inv(*px_to_km(px, py))


def shifted(c):
    return c % 2 == 1


def hex_id(c, r):
    return "%02d%02d" % (c + 1, r + 1)


def parse_id(h):
    return int(h[:2]) - 1, int(h[2:]) - 1


def in_grid(c, r):
    return 0 <= c < COLS and 0 <= r < ROWS


def centre(c, r):
    x = OX + c * 1.5 * SIZE
    y = OY + r * SQRT3 * SIZE + (SQRT3 / 2 * SIZE if shifted(c) else 0)
    return x, y


def corner(c, r, name, inset=0.0):
    cx, cy = centre(c, r)
    a = math.radians(FLAT["corners"][name])
    d = SIZE * (1 - inset)
    return cx + d * math.cos(a), cy + d * math.sin(a)


def polygon(c, r, inset=0.0):
    return [corner(c, r, n, inset) for n in FLAT["corners"]]


def neighbour(c, r, d):
    tbl = FLAT["nbr_shift"] if shifted(c) else FLAT["nbr_plain"]
    dc, dr = tbl[d]
    return c + dc, r + dr


def neighbours(c, r):
    for d in DIRS:
        n = neighbour(c, r, d)
        if in_grid(*n):
            yield d, n


def direction(a, b):
    """The direction from hex a to adjacent hex b, or None."""
    for d in DIRS:
        if neighbour(a[0], a[1], d) == tuple(b):
            return d
    return None


def adjacent(a, b):
    return direction(a, b) is not None


def hex_at(px, py, cols=None, rows=None):
    """The hex containing a pixel point: the nearest centre among the candidates around it.  cols and rows
    bound the search on the lattice of another scale (the pixel lattice is the same at every scale)."""
    cols = COLS if cols is None else cols
    rows = ROWS if rows is None else rows
    c0 = int(round((px - OX) / (1.5 * SIZE)))
    best, bd = None, 1e18
    for c in (c0 - 1, c0, c0 + 1):
        if c < 0 or c >= cols:
            continue
        y0 = OY + (SQRT3 / 2 * SIZE if shifted(c) else 0)
        r0 = int(round((py - y0) / (SQRT3 * SIZE)))
        for r in (r0 - 1, r0, r0 + 1):
            if r < 0 or r >= rows:
                continue
            cx, cy = centre(c, r)
            d = (cx - px) ** 2 + (cy - py) ** 2
            if d < bd:
                best, bd = (c, r), d
    return best


def hex_of_ll(lat, lon):
    return hex_at(*ll_to_px(lat, lon))


def hex_of_km(x, y, hex_km):
    """(col, row) of the hex containing a km point on the lattice of hex_km between centres."""
    k = SQRT3 * SIZE / hex_km
    cols, rows = dims(hex_km)
    return hex_at(MARGIN_X + (x - XMIN_KM) * k, MARGIN_Y + (YMAX_KM - y) * k, cols, rows)


_PARENT = {}


def rescale_ids(ids, from_km=REF_KM):
    """The hex ids of this lattice whose centres fall in the listed hexes of the from_km lattice, space
    separated; the identity at the same scale.  A ruling keeps its area and gains no gaps."""
    if from_km == HEX_KM:
        return ids if isinstance(ids, str) else " ".join(ids)
    if from_km not in _PARENT:
        par = {}
        for c in range(COLS):
            for r in range(ROWS):
                p = hex_of_km(*px_to_km(*centre(c, r)), from_km)
                if p is not None:
                    par[hex_id(c, r)] = hex_id(*p)
        _PARENT[from_km] = par
    want = set(ids.split() if isinstance(ids, str) else ids)
    return " ".join(h for h, p in sorted(_PARENT[from_km].items()) if p in want)


# ---- vertices and edges ------------------------------------------------------------------------
def vkey(p):
    """Canonical key of a vertex from its pixel position (corners from different hexes differ by
    float noise; the lattice spacing is far above 0.5 px)."""
    return (int(round(p[0] * 2)), int(round(p[1] * 2)))


def vpos(k):
    return k[0] / 2.0, k[1] / 2.0


def edge_vertices(c, r, d):
    a, b = FLAT["edge_corners"][d]
    return vkey(corner(c, r, a)), vkey(corner(c, r, b))


def canonical_edge(c, r, d):
    """(hex, dir) naming the hexside from the hex that is first in (col, row) order; the other hex
    may lie outside the grid, in which case this hex names it."""
    n = neighbour(c, r, d)
    if in_grid(*n) and (n[0], n[1]) < (c, r):
        return n[0], n[1], OPPOSITE[d]
    return c, r, d


def edge_name(c, r, d):
    c, r, d = canonical_edge(c, r, d)
    return "%s:%s" % (hex_id(c, r), d)


def build_vertex_graph():
    """vertex key -> {neighbour vertex key: (c, r, d) canonical edge} over the whole grid."""
    g = {}
    e2v = {}
    for c in range(COLS):
        for r in range(ROWS):
            for d in DIRS:
                cc, rr, dd = canonical_edge(c, r, d)
                if (cc, rr, dd) in e2v:
                    continue
                a, b = edge_vertices(cc, rr, dd)
                e2v[(cc, rr, dd)] = (a, b)
                g.setdefault(a, {})[b] = (cc, rr, dd)
                g.setdefault(b, {})[a] = (cc, rr, dd)
    return g, e2v


def edge_hexes(c, r, d):
    """The two hexes sharing a hexside (the second may be outside the grid)."""
    return (c, r), neighbour(c, r, d)


def hex_line(a, b):
    """The hexes on the straight line between the centres of a and b, inclusive, by sampling."""
    ax, ay = centre(*a)
    bx, by = centre(*b)
    n = max(1, int(math.ceil(math.hypot(bx - ax, by - ay) / (SIZE * 0.5))))
    out = [tuple(a)]
    for i in range(1, n + 1):
        t = i / n
        h = hex_at(ax + (bx - ax) * t, ay + (by - ay) * t)
        if h is not None and h != out[-1]:
            # a sampled jump across two hexes at once would break adjacency; refine
            if not adjacent(out[-1], h):
                for k in range(1, 8):
                    tt = (i - 1 + k / 8) / n
                    hh = hex_at(ax + (bx - ax) * tt, ay + (by - ay) * tt)
                    if hh is not None and hh != out[-1] and adjacent(out[-1], hh):
                        out.append(hh)
                        if hh == h:
                            break
                if out[-1] != h and adjacent(out[-1], h):
                    out.append(h)
                elif out[-1] != h:
                    raise ValueError("hex_line could not step from %s to %s" % (hex_id(*out[-1]), hex_id(*h)))
            else:
                out.append(h)
    return out


if __name__ == "__main__":
    print("sheet %d x %d px, map %.0f x %.0f px, %d cols x %d rows = %d hexes" %
          (SHEET_W, SHEET_H, MAP_W, MAP_H, COLS, ROWS, COLS * ROWS))
    print("km covered: x %.0f..%.0f  y %.0f..%.0f" %
          (XMIN_KM, XMIN_KM + MAP_W / PX_PER_KM, YMAX_KM - MAP_H / PX_PER_KM, YMAX_KM))
    for name, lat, lon in [("Peiping", 39.90, 116.39), ("Wuhan", 30.58, 114.27), ("Chongqing", 29.57, 106.59),
                           ("Harbin", 45.75, 126.65), ("Kunming", 25.04, 102.70), ("Hanoi", 21.04, 105.85),
                           ("Khabarovsk", 48.45, 135.12), ("Blagoveshchensk", 50.27, 127.53),
                           ("Manzhouli", 49.60, 117.43), ("Shanghai", 31.22, 121.43), ("Lanzhou", 36.06, 103.79)]:
        px = ll_to_px(lat, lon)
        h = hex_at(*px)
        print("%-16s px (%.0f, %.0f) hex %s" % (name, px[0], px[1], hex_id(*h) if h else None))
    # corner check: the four lattice corners in lat/lon
    for (c, r) in [(0, 0), (COLS - 1, 0), (0, ROWS - 1), (COLS - 1, ROWS - 1)]:
        lat, lon = px_to_ll(*centre(c, r))
        print("hex %s centre at %.2f N %.2f E" % (hex_id(c, r), lat, lon))
# Copyright Ben Paul Wise. All Rights Reserved.
