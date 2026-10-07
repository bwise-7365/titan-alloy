# Copyright Ben Paul Wise. All Rights Reserved.
"""illustrate.py -- the figures of the design memorandum, drawn on the standard (75 km) sheet.

Two sets.  The four major actions (Ichi-Go, the reflux of 1945, August 1945, the race), an example setup
and one way it might play out for each: first drawn on the 100 km sheet, so their tables are written in
100 km hex ids (km=100) and converted here, a place's hex to the same place's hex, any other hex to the
hex under its centre.  The maneuver studies (Xue Yue's defence of Changsha, the envelopment of Hengyang),
written in 75 km ids in maneuvers.py.  Each figure is a crop of map_graphics/xml/circling-dragons.svg (the
reference render) with counters from unit_graphics/xml/circling-dragons.xml drawn through the counter
renderer, plus arrows, battle bursts and notes.  The positions are examples for the prototype, not a
scenario; the counts and values are the memorandum's preliminary ones.

    python illustrate.py [--only NAME] [--no-export] [--boxes]

Writes Circling Dragons/illustrations/<name>.svg and, through Inkscape, <name>.png (1800 px wide), and
Circling Dragons/sections/locator-boxes.tex: for each section of figures, the area its figures show, as a
TikZ rectangle in fractions of the sheet, for the full-page locator maps of the memorandum.  --boxes
writes only that file.
"""
import math
import os
import re
import subprocess
import sys
from xml.sax.saxutils import escape

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, HERE)
import lattice as L  # noqa: E402
import cd_data as D  # noqa: E402

UG = os.path.join(REPO, "unit_graphics", "xml")
sys.path.insert(0, UG)
from counters2svg import Renderer  # noqa: E402

MAP_SVG = os.path.join(REPO, "map_graphics", "xml", "circling-dragons%s.svg" % L.SUFFIX)   # the sheet of this scale
COUNTERS = os.path.join(UG, "circling-dragons.xml")
OUT = os.path.join(REPO, "Circling Dragons", "illustrations")
INKSCAPE = r"C:\Program Files\Inkscape\bin\inkscape.exe"
R = L.SIZE
JP, KMT, CCP, SOV, US = "#9c2a0e", "#1f4e9c", "#c81e1e", "#5a3410", "#2e7d32"
SERIF = "Georgia, Times New Roman, serif"
SANS = "Arial Narrow, Liberation Sans Narrow, Arial, sans-serif"


# ------------------------------------------------------------------------------------------------ geometry
SRC_KM = [None]    # the scale a picture's hex ids are written in; None: this sheet's
MOVE = {}          # per picture: a hex id written at SRC_KM -> the hex of this sheet to use instead (two old
                   # hexes that fall into one new hex: the second goes one hex further on)


def place_hexes(hex_km):
    """place id -> hex id on the lattice of hex_km, after that scale's pins."""
    pins = D.PLACE_HEX_BY_SCALE.get(hex_km, {})
    out = {}
    for pid, label, lat, lon, kind, flags, terr in list(D.PLACES) + list(D.PLACES_V2) + list(D.PLACES_V3):
        out[pid] = (kind, pins.get(pid) or L.hex_id(*L.hex_of_km(*L.fwd(lat, lon), hex_km)))
    return out


RANK = {"major": 0, "city": 1, "town": 2, "mark": 3, "wp": 4}
_CONVERT = {}


def M(h, from_km):
    """The hex of this sheet for a hex id written on the from_km lattice: the hex of the place the old hex
    held (the most important one), or else the hex under the old hex's centre."""
    key = (h, from_km)
    if key not in _CONVERT:
        if ("places", from_km) not in _CONVERT:
            old, new = place_hexes(from_km), place_hexes(L.HEX_KM)
            best = {}
            for pid, (kind, oh) in old.items():
                if oh not in best or RANK.get(kind, 9) < RANK.get(old[best[oh]][0], 9):
                    best[oh] = pid
            _CONVERT["places", from_km] = {oh: new[pid][1] for oh, pid in best.items()}
        hit = _CONVERT["places", from_km].get(h)
        if hit is None:
            c, r = L.parse_id(h)
            px, py = L.centre(c, r)                  # the pixel lattice is the same at every scale
            k = L.SQRT3 * L.SIZE / from_km
            hit = L.hex_id(*L.hex_of_km(L.XMIN_KM + (px - L.MARGIN_X) / k, L.YMAX_KM - (py - L.MARGIN_Y) / k, L.HEX_KM))
        _CONVERT[key] = hit
    return _CONVERT[key]


def C(h):
    """Centre of a hex by printed id (written at the picture's scale)."""
    if SRC_KM[0] and SRC_KM[0] != L.HEX_KM:
        h = MOVE.get(h) or M(h, SRC_KM[0])
    return L.centre(*L.parse_id(h))


def H(lat, lon):
    """The centre of the hex of a point, for places the data file does not name (a point, so it is not
    converted again)."""
    return L.centre(*L.hex_of_ll(lat, lon))


def P(p):
    return C(p) if isinstance(p, str) else p


def crop_box(tl, br, pad=12, pad_top=0):
    (ax, ay), (bx, by) = C(tl), C(br)
    return ax - R - pad, ay - R - pad - pad_top, bx + R + pad, by + R + pad


# ------------------------------------------------------------------------------------------------ drawing
def map_inner():
    s = open(MAP_SVG, encoding="utf-8").read()
    i = s.index(">", s.index("<svg")) + 1
    j = s.rindex("</svg>")
    return re.sub(r"<title>.*?</title>", "", s[i:j], count=1, flags=re.S)


MAP = map_inner()
REN = Renderer(COUNTERS)
SIZE = [54.0]      # counter side in map px; set per picture
CROP = [0.0, 0.0, 1.0, 1.0]   # the picture's crop: x0, y0, width, height in map px
QUIET = [False]    # True: the item tables' own labels are left out (the picture has a notes layer)
EXPORT_W = 1800.0


def E(x, y):
    """A point given in pixels of the exported image (1800 wide) -> map px, for placing notes."""
    k = CROP[2] / EXPORT_W
    return (CROP[0] + x * k, CROP[1] + y * k)
TEXT = [1.0]       # text and panel scale; set per picture (tight crops are enlarged more on export)


# counters merged into double-sided markers (2026-10-06): the old id -> the counter and the face that shows it
ALIAS = {"ccp-base": "ccp-presence:b", "kmt-control": "mk-control", "ccp-control": "mk-control:b",
         "mk-rail-interdicted": "mk-rail", "mk-rail-broken": "mk-rail:b", "mk-airfield-captured": "mk-airfield",
         "mk-airfield-destroyed": "mk-airfield:b", "mk-port-denied": "mk-port", "mk-port-open": "mk-port:b",
         "jp-front-kmt": "jp-front", "jp-front-ccp": "jp-front:b", "kw-fort": "kw-4a-2"}


def counter(cid, x, y, size=None):
    """A counter face at (x, y); "id:r" draws the reduced back face (a step lost), "id:b" the back of a
    double-sided marker."""
    size = size or SIZE[0]
    cid = ALIAS.get(cid, cid)
    cid, _, side = cid.partition(":")
    c = REN.counters[cid]
    face = REN.back_face(c) if side in ("r", "b") else c.find("front")
    k = size / 100.0
    return ('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="#000000" opacity="0.3" rx="2"/>'
            '<g font-family="%s" transform="translate(%.1f,%.1f) scale(%.4f)">%s</g>'
            % (x - size / 2 + 3, y - size / 2 + 3, size, size, SANS, x - size / 2, y - size / 2, k,
               REN.face_svg(face, c)))


def stack(h, cids):
    x, y = P(h)
    n = len(cids)
    return "".join(counter(cid, x + (i - (n - 1) / 2) * 7, y - (i - (n - 1) / 2) * 7) for i, cid in enumerate(cids))


def text(x, y, s, size=20, bold=True, anchor="middle", color="#111111", halo="#ffffff", italic=False):
    size = size * TEXT[0]
    lines = s.split("|")
    o = []
    for i, ln in enumerate(lines):
        dy = (i - (len(lines) - 1) / 2) * size * 1.15
        o.append('<text x="%.1f" y="%.1f" font-size="%d" text-anchor="%s" fill="%s"%s%s paint-order="stroke" stroke="%s" '
                 'stroke-width="%d" stroke-linejoin="round" dominant-baseline="central">%s</text>'
                 % (x, y + dy, size, anchor, color, ' font-weight="bold"' if bold else "", ' font-style="italic"' if italic else "",
                    halo, max(3, size // 4), escape(ln)))
    return "".join(o)


def shift_line(pts, d):
    """A polyline moved sideways by d px (positive: to the left of the direction of travel)."""
    out = []
    for i, p in enumerate(pts):
        nx = ny = 0.0
        for a, b in [(pts[j], pts[j + 1]) for j in (i - 1, i) if 0 <= j < len(pts) - 1]:
            n = math.hypot(b[0] - a[0], b[1] - a[1]) or 1.0
            nx += (b[1] - a[1]) / n
            ny += -(b[0] - a[0]) / n
        k = math.hypot(nx, ny) or 1.0
        out.append((p[0] + nx / k * d, p[1] + ny / k * d))
    return out


def arrow(points, color, width=9, dash=False, label=None, at=None, dy=-16, dx=0, anchor="middle", offset=0.0):
    pts = [P(p) for p in points]
    if offset:
        pts = shift_line(pts, offset)
    d = "M%.1f,%.1f " % pts[0] + " ".join("L%.1f,%.1f" % p for p in pts[1:])
    o = ['<path d="%s" fill="none" stroke="#ffffff" stroke-width="%.1f" stroke-opacity="0.7" stroke-linejoin="round" stroke-linecap="round"/>'
         % (d, width + 7),
         '<path d="%s" fill="none" stroke="%s" stroke-width="%.1f" stroke-opacity="0.92" stroke-linejoin="round" stroke-linecap="round"%s '
         'marker-end="url(#arrow-%s)"/>' % (d, color, width, ' stroke-dasharray="20,13"' if dash else "", color.lstrip("#"))]
    if label and not QUIET[0]:
        lx, ly = P(at) if at else pts[-1]
        o.append(text(lx + dx, ly + dy, label, 19, True, anchor, color))
    return "".join(o)


def burst(h, label=None, r=34, dx=0, dy=None, anchor="middle"):
    x, y = P(h)
    pts = []
    for i in range(16):
        a = math.radians(i * 22.5 - 90)
        rr = r if i % 2 == 0 else r * 0.55
        pts.append("%.1f,%.1f" % (x + rr * math.cos(a), y + rr * math.sin(a)))
    o = ['<polygon points="%s" fill="#ffd21f" stroke="#c0392b" stroke-width="3" opacity="0.92"/>' % " ".join(pts)]
    if label and not QUIET[0]:
        o.append(text(x + dx, y + (dy if dy is not None else r + 16), label, 19, True, anchor, "#7a1f12"))
    return "".join(o)


def note(h, s, dx=0, dy=0, size=19, anchor="middle", color="#111111"):
    if QUIET[0]:
        return ""
    x, y = P(h)
    return text(x + dx, y + dy, s, size, True, anchor, color)


def LL(lat, lon):
    """The map pixel of a point, for drawing between hex centres."""
    return L.ll_to_px(lat, lon)


def N(h, d):
    """The neighbouring hex id in direction d (n ne se s sw nw)."""
    return L.hex_id(*L.neighbour(*L.parse_id(h), d))


def zone(hexes, fill, opacity=0.25, stroke=None, width=4, dash=None):
    """Hexes washed in a colour: a ring, a cut supply line, an assembly area."""
    o = []
    for h in hexes:
        pts = " ".join("%.1f,%.1f" % q for q in L.polygon(*L.parse_id(h), inset=0.05))
        st = ""
        if stroke:
            st = ' stroke="%s" stroke-width="%.1f" stroke-linejoin="round"%s' % (stroke, width, ' stroke-dasharray="%s"' % dash if dash else "")
        o.append('<polygon points="%s" fill="%s" fill-opacity="%.2f"%s/>' % (pts, fill, opacity, st))
    return "".join(o)


def minor_river(points, label=None, at=0, dx=0, dy=-14, anchor="middle"):
    """A minor river the sheet does not print, drawn schematically through (lat, lon) points."""
    pts = [LL(*q) for q in points]
    d = "M%.1f,%.1f " % pts[0] + " ".join("L%.1f,%.1f" % q for q in pts[1:])
    o = ['<path d="%s" fill="none" stroke="#2f6fd0" stroke-width="4" stroke-dasharray="10,7" stroke-linecap="round" '
         'stroke-linejoin="round" opacity="0.9"/>' % d]
    if label:
        x, y = pts[at]
        o.append(text(x + dx, y + dy, label, 15, False, anchor, "#1f4e9c", italic=True))
    return "".join(o)


def tag(p, n, color="#1f4e9c"):
    """A small numbered tag (white disc, coloured ring) keyed in the legend: the minor rivers."""
    x, y = P(p)
    r = 9 * TEXT[0]
    return ('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#ffffff" stroke="%s" stroke-width="2.2"/>' % (x, y, r, color)
            + '<text x="%.1f" y="%.1f" font-size="%.1f" text-anchor="middle" dominant-baseline="central" fill="%s" '
              'font-weight="bold">%s</text>' % (x, y, r * 1.35, color, n))


def callout(at, targets, s, color, size=16, anchor="middle", gap=None):
    """A note at a point with thin leader lines to one or more hexes (or points): the line runs from the
    edge of the text block to the edge of the counter at the target."""
    if targets is None:
        targets = []
    elif isinstance(targets, (str, tuple)):
        targets = [targets]
    x, y = P(at)
    px = size * TEXT[0]
    lines = s.split("|")
    w = 0.56 * px * max(len(ln) for ln in lines) + 6
    h = 1.15 * px * len(lines) + 4
    cx = x + {"start": w / 2, "end": -w / 2}.get(anchor, 0)
    o = []
    for t in targets:
        tx, ty = P(t)
        g = (SIZE[0] * 0.55 if isinstance(t, str) else 4) if gap is None else gap
        dx, dy = tx - cx, ty - y
        dist = math.hypot(dx, dy)
        if dist < 1:
            continue
        sx = (w / 2) / abs(dx) if dx else 1e9
        sy = (h / 2) / abs(dy) if dy else 1e9
        s0 = min(sx, sy)
        s1 = max(0.0, 1 - g / dist)
        if s1 <= s0:
            continue
        o.append('<path d="M%.1f,%.1f L%.1f,%.1f" stroke="#ffffff" stroke-width="4.5" stroke-opacity="0.75" stroke-linecap="round"/>'
                 '<path d="M%.1f,%.1f L%.1f,%.1f" stroke="%s" stroke-width="2" stroke-linecap="round"/>'
                 '<circle cx="%.1f" cy="%.1f" r="2.6" fill="%s"/>'
                 % (cx + dx * s0, y + dy * s0, cx + dx * s1, y + dy * s1,
                    cx + dx * s0, y + dy * s0, cx + dx * s1, y + dy * s1, color, cx + dx * s1, y + dy * s1, color))
    o.append(text(x, y, s, size, True, anchor, color))
    return "".join(o)


def step(p, n, color, dx=0, dy=0):
    """A numbered disc giving the order of the moves in a figure."""
    x, y = P(p)
    r = 13 * TEXT[0]
    return ('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s" stroke="#ffffff" stroke-width="2.5"/>' % (x + dx, y + dy, r, color)
            + '<text x="%.1f" y="%.1f" font-size="%.0f" text-anchor="middle" dominant-baseline="central" fill="#ffffff" '
              'font-weight="bold">%s</text>' % (x + dx, y + dy, r * 1.3, n))


def box(x, y, maxw, title, sub=None, counters_=()):
    """The title box: width from the text, counters at its right end."""
    k = TEXT[0]
    w = min(maxw, k * (max(15.5 * len(title), 8.8 * len(sub or "")) + 40 + 60 * len(counters_)))
    h = k * (86 if sub else 56)
    o = ['<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="10" fill="#ffffff" opacity="0.93" stroke="#333333" stroke-width="2"/>' % (x, y, w, h)]
    o.append(text(x + 18 * k, y + 30 * k, title, 30, True, "start"))
    if sub:
        o.append(text(x + 18 * k, y + 64 * k, sub, 20, False, "start", "#222222", italic=True))
    cx = x + w - 40 * k
    for cid in counters_:
        o.append(counter(cid, cx, y + h / 2, 50 * k))
        cx -= 60 * k
    return "".join(o)


def legend(x, y, items, w=330):
    k = TEXT[0]
    h = k * (26 + 30 * len(items))
    o = ['<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="8" fill="#ffffff" opacity="0.93" stroke="#333333" stroke-width="2"/>' % (x, y, w, h)]
    for i, (color, dash, s) in enumerate(items):
        yy = y + k * (24 + 30 * i)
        if color is None:
            pass                                   # a line of text only
        elif color == "burst":
            o.append(burst((x + 32 * k, yy), None, 12 * k))
        elif color.startswith("tag:"):             # a numbered tag, as on the map
            o.append(tag((x + 33 * k, yy), color[4:]))
        elif color.startswith("zone:"):
            c = color[5:]
            o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s" fill-opacity="0.3" stroke="%s" stroke-width="2"/>'
                     % (x + 14 * k, yy - 9 * k, 38 * k, 18 * k, c, c))
        else:
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f" stroke="%s" stroke-width="%.1f" stroke-linecap="round"%s/>'
                     % (x + 14 * k, yy, x + 52 * k, yy, color, 8 * k, ' stroke-dasharray="12,8"' if dash else ""))
        o.append(text(x + (66 if color is not None else 18) * k, yy, s, 18, False, "start"))
    return "".join(o)


def markers():
    return "".join('<marker id="arrow-%s" viewBox="0 0 10 10" refX="7" refY="5" markerWidth="3.2" markerHeight="3.2" orient="auto-start-reverse">'
                   '<path d="M0,0 L10,5 L0,10 Z" fill="%s"/></marker>' % (c.lstrip("#"), c) for c in (JP, KMT, CCP, SOV, US))


def picture(p):
    SIZE[0] = p.get("size", 54.0)
    TEXT[0] = p.get("text", 1.0)
    SRC_KM[0] = p.get("km")
    MOVE.clear()
    MOVE.update(p.get("move", {}))
    x0, y0, x1, y1 = crop_box(*p["crop"], pad_top=p.get("pad_top", 0))
    W, Hh = x1 - x0, y1 - y0
    CROP[:] = [x0, y0, W, Hh]
    QUIET[0] = "notes" in p
    o = ['<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="%d" height="%d" viewBox="%.1f %.1f %.1f %.1f" font-family="%s">'
         % (W, Hh, x0, y0, W, Hh, SERIF),
         '<title>%s</title>' % escape(p["title"]),
         '<defs>%s</defs>' % markers(),
         '<g class="map">%s</g>' % MAP,
         '<g class="overlay">']
    o.extend(p["items"]())
    if "notes" in p:
        o.extend(p["notes"]())
    o.append(box(x0 + 18, y0 + 18, W - 36, p["title"], p.get("sub"), p.get("title_counters", ())))
    if p.get("legend"):
        lx, ly = p.get("legend_at", ("right", "bottom"))
        lw = p.get("legend_w", 340) * TEXT[0]
        lh = TEXT[0] * (26 + 30 * len(p["legend"]))
        X = x1 - lw - 18 if lx == "right" else x0 + 18
        Y = y1 - lh - 18 if ly == "bottom" else y0 + 122
        o.append(legend(X, Y, p["legend"], lw))
    o.append('</g></svg>')
    return "\n".join(o)


# ------------------------------------------------------------------------------------------------ the pictures
LEG_ALL = [(JP, False, "Japanese operation"), (KMT, False, "KMT movement"), (CCP, False, "CCP movement or expansion"),
           (SOV, False, "Soviet grouping"), (US, True, "US air and sea lift"), ("burst", False, "battle or siege")]
SOUTH = ("0616", "2132")       # the Ichi-Go country, Chongqing to the coast
NORTHEAST = ("1300", "3514")   # Mongolia, Manchuria, Korea
NORTH = ("1004", "3019")       # North China and southern Manchuria


def ichigo_setup():
    return [
        # Japanese: 12th Army north of the Yellow River, 11th Army at Wuhan and Yichang, 13th and 23rd Armies on the coast
        stack("1617", ["jp-d110", "jp-d37"]), stack("1718", ["jp-d62", "jp-tk3"]), stack("1618", ["mk-bridge-yellow"]),
        stack("1614", ["jp-g-pinghan"]), stack("1916", ["jp-g-jinpu"]), stack("1919", ["jp-d65", "jp-g-longhai"]),
        stack("1623", ["jp-d3", "jp-d68", "jp-d116", "jp-air"]), stack("1323", ["jp-d13", "jp-d39"]), stack("1424", ["jp-d34", "jp-d40"]),
        stack("1824", ["jp-d58"]), stack("1621", ["jp-d27"]), stack("2422", ["jp-d60"]), stack("2223", ["jp-d70"]), stack("1431", ["jp-d104", "jp-d22"]),
        # KMT war areas
        stack("1619", ["kmt-ga15", "kmt-ga31"]), stack("1418", ["kmt-ga19", "kmt-hq1"]), stack("1420", ["kmt-ga28"]),
        stack("1321", ["kmt-ga22", "kmt-ga33"]), stack("1123", ["kmt-ga10"]), stack("1325", ["kmt-ga26"]),
        stack("1425", ["kmt-ga24", "kmt-hq9"]), stack("1427", ["kmt-ga27", "kmt-fort", "us-14af"]), stack("1526", ["kmt-ga30"]),
        stack("1129", ["kmt-ga16", "us-14af"]), stack("1029", ["kmt-ga35", "us-cacw"]), stack("1530", ["kmt-ga12"]), stack("2025", ["kmt-ga23"]),
        stack("0724", ["kmt-hqalpha"]),
        # CCP: field forces and presence along the Pinghan and in the New Fourth Army country
        stack("1516", ["ccp-base", "ccp-jjly"]), stack("1522", ["ccp-base", "ccp-n4a5"]), stack("1820", ["ccp-n4a4"]), stack("2021", ["ccp-n4a2"]),
        stack("2320", ["ccp-n4a1"]), stack("2020", ["ccp-n4a3"]), stack("2016", ["ccp-base", "ccp-sd"]),
        stack("1616", ["ccp-presence"]), stack("1716", ["ccp-presence"]), stack("1717", ["ccp-presence"]), stack("1818", ["ccp-presence"]),
        stack("1920", ["ccp-presence"]), stack("2120", ["ccp-presence"]), stack("1515", ["ccp-presence"]), stack("1721", ["ccp-presence"]),
        note("1618", "Yellow River rail bridge:|the only crossing for the Henan offensive", -70, 100, 18, "end", JP),
        note("1427", "Hengyang: fortified city,|14 AF field", -70, 44, 18, "end", KMT),
        note("1623", "11 Army, Wuhan", 0, 56, 18, "middle", JP),
        note("0724", "Chongqing: Alpha Force|forms here and at Kunming", 60, 60, 18, "start", KMT),
    ]


def ichigo_play():
    return [
        stack("1418", ["jp-d110"]), stack("1618", ["jp-d62", "jp-tk3"]), stack("1619", ["jp-d37"]),
        stack("1425", ["jp-d34"]), stack("1427", ["jp-d68", "jp-d116", "mk-airfield-destroyed"]), stack("1328", ["jp-d3", "mk-airfield-destroyed"]),
        stack("1129", ["jp-d58", "mk-airfield-captured"]), stack("1029", ["jp-d13", "mk-airfield-captured"]), stack("0827", ["jp-d40"]),
        stack("0831", ["jp-d22"]), stack("1623", ["jp-d39", "jp-air"]),
        stack("1420", ["kmt-ga19", "kmt-ga28"]), stack("1218", ["kmt-ga15", "kmt-hq1"]), stack("1321", ["kmt-ga22", "kmt-ga33"]),
        stack("1325", ["kmt-ga26", "kmt-ga24"]), stack("1226", ["kmt-ga27", "kmt-hq9"]), stack("1127", ["kmt-ga30"]),
        stack("0829", ["kmt-ga16", "kmt-ga35"]), stack("0727", ["kmt-ga11", "kmt-hqalpha"]), stack("1026", ["us-14af", "kmt-n6a"]),
        stack("1516", ["ccp-base", "ccp-jjly"]), stack("1617", ["ccp-presence"]), stack("1717", ["ccp-presence"]), stack("1816", ["ccp-presence"]),
        stack("1916", ["jp-g-jinpu", "jp-security"]), stack("1614", ["jp-g-pinghan", "jp-security"]),
        # the three phases
        arrow(["1617", "1618", "1418"], JP, label="I. Henan, April-May: the river|crossed, Luoyang besieged", at="1418", dx=-40, dy=-104),
        arrow(["1718", "1619", "1620"], JP),
        arrow(["1623", "1424", "1425", "1427"], JP, label="II. Hunan, May-August:|Changsha falls, Hengyang holds 47 days", at="1425", dx=90, dy=-10, anchor="start"),
        arrow(["1427", "1328", "1129", "1029"], JP, label="III. Guangxi, Sept-Nov:|Guilin and Liuzhou", at="1129", dx=80, dy=20, anchor="start"),
        arrow(["1029", "0929", "0827"], JP, label="December: Dushan,|then the halt", at="0827", dx=50, dy=-44, anchor="start"),
        arrow(["1431", "1331", "0831"], JP, label="23 Army to Nanning", at="0831", dx=0, dy=52),
        # KMT withdrawals and the Alpha Force
        arrow(["1425", "1325"], KMT), arrow(["1427", "1226"], KMT), arrow(["1129", "0829"], KMT),
        arrow(["0228", "0727"], KMT, label="Alpha Force forms at Kunming;|New 6 Army flown back from Burma", at="0727", dx=40, dy=72, anchor="start"),
        arrow(["0228", "1026"], US, dash=True),
        # CCP expansion behind the departing divisions
        arrow(["1516", "1617"], CCP, label="presence spreads along|the Pinghan as garrisons thin", at="1617", dx=-80, dy=64, anchor="end"),
        arrow(["2016", "1816"], CCP),
        burst("1418", "Luoyang, May", dx=70, dy=-30, anchor="start"), burst("1427", None), burst("1129", None), burst("0827", None),
        note("1618", "bridge repaired, bombed, repaired", 0, 58, 17),
        note("1916", "the KMT player raids the bases|with the North China garrisons|while the pools are drawn down", 0, 90, 17, "middle", JP),
    ]


def reflux_setup():
    return [
        stack("1326", ["jp-d116", "jp-d34"]), stack("1427", ["jp-d68", "mk-airfield-destroyed"]), stack("1526", ["jp-d27"]),
        stack("1129", ["jp-d13", "mk-airfield-captured"]), stack("1029", ["jp-d3", "mk-airfield-captured"]), stack("0831", ["jp-d58", "jp-d22"]),
        stack("1623", ["jp-d39", "jp-d40", "jp-air"]), stack("1431", ["jp-d104"]), stack("1824", ["jp-g-yangtze"]), stack("1530", ["jp-g-canton"]),
        stack("1321", ["jp-d110"]), stack("1619", ["jp-d37", "jp-g-longhai"]), stack("1919", ["jp-d65"]), stack("2422", ["jp-d60"]),
        stack("1026", ["kmt-n6a", "kmt-hqalpha", "us-14af"]), stack("0727", ["kmt-ga11"]), stack("0827", ["kmt-ga20"]), stack("1127", ["kmt-ga27"]),
        stack("1325", ["kmt-ga24"]), stack("1227", ["kmt-ga30", "kmt-hq9"]), stack("0829", ["kmt-ga16"]), stack("0928", ["kmt-ga35"]),
        stack("1123", ["kmt-ga10"]), stack("1222", ["kmt-ga26"]), stack("2025", ["kmt-ga23"]), stack("1629", ["kmt-ga12"]), stack("0926", ["us-cacw"]),
        stack("1516", ["ccp-base", "ccp-jjly"]), stack("1522", ["ccp-base", "ccp-n4a5"]), stack("1820", ["ccp-n4a4"]), stack("2021", ["ccp-n4a2"]),
        stack("2320", ["ccp-n4a1"]), stack("2119", ["ccp-n4a3"]), stack("2016", ["ccp-base", "ccp-sd"]), stack("1531", ["ccp-dj"]),
        stack("1517", ["ccp-presence"]), stack("1618", ["ccp-presence"]), stack("1719", ["ccp-presence"]), stack("1921", ["ccp-presence"]),
        stack("2020", ["ccp-presence"]), stack("2121", ["ccp-presence"]), stack("1616", ["ccp-presence"]), stack("1818", ["ccp-presence"]),
        note("1026", "Zhijiang: Alpha Force,|US-equipped, air-supplied", 0, -72, 18, "middle", KMT),
        note("1326", "20 Army at Baoqing,|facing the Xuefeng", -70, 0, 18, "end", JP),
        note("1321", "Laohekou lost in April", 0, -46, 17, "middle", JP),
    ]


def reflux_play():
    return [
        stack("1026", ["kmt-n6a", "kmt-hqalpha", "us-14af"]), stack("1126", ["kmt-ga20"]), stack("1326", ["kmt-ga27"]),
        stack("1029", ["kmt-ga16", "mk-airfield-captured"]), stack("1129", ["kmt-ga35", "kmt-ga11"]), stack("0831", ["kmt-ga12"]),
        stack("1623", ["jp-d39", "jp-d40", "jp-d3", "jp-front-ccp"]), stack("1427", ["jp-d68"]), stack("1526", ["jp-d27", "jp-d116"]),
        stack("1812", ["jp-d13", "jp-front-ccp"]), stack("1919", ["jp-d65", "jp-d110"]), stack("2422", ["jp-d60", "jp-d104"]), stack("1431", ["jp-d22"]),
        stack("1516", ["ccp-base", "ccp-jjly"]), stack("1617", ["ccp-presence", "ccp-concentrate"]), stack("1717", ["ccp-presence"]), stack("1718", ["ccp-presence"]),
        stack("2016", ["ccp-base", "ccp-sd"]), stack("2116", ["ccp-presence"]), stack("1916", ["mk-rail-interdicted"]), stack("1615", ["mk-rail-interdicted"]),
        stack("1522", ["ccp-base", "ccp-n4a5"]), stack("1820", ["ccp-n4a4"]), stack("2021", ["ccp-n4a2"]), stack("2320", ["ccp-n4a1"]),
        arrow(["1326", "1226", "1126"], JP, label="April-June: 20 Army over|the Xuefeng toward Zhijiang", at="1326", dx=60, dy=-56, anchor="start", offset=12),
        arrow(["1126", "1226", "1326"], KMT, label="repulsed and pursued;|Alpha Force's first battle", at="1226", dx=0, dy=68, offset=12),
        arrow(["0831", "1029", "1129", "1427", "1526", "1623"], JP, label="May-August: 6 Area Army|withdraws north; Guangxi abandoned", at="1427", dx=70, dy=36, anchor="start"),
        arrow(["1623", "1621", "1812"], JP, label="divisions to the Peiping axis|and the coast: control passes|to the KMT player", at="1621", dx=64, dy=-30, anchor="start"),
        arrow(["0829", "1029"], KMT), arrow(["0928", "1129"], KMT, label="Nanning, Liuzhou, Guilin|retaken July-August", at="1029", dx=0, dy=72),
        arrow(["1516", "1617"], CCP, label="presence concentrates into|a field force; rails interdicted", at="1617", dx=0, dy=86),
        arrow(["2016", "2116"], CCP),
        burst("1126", "Xuefeng, May", dx=-70, dy=-10, anchor="end"), burst("1321", "Laohekou", dx=0, dy=-44),
        note("2026", "the Pacific War track nears Soviet entry;|the players weigh how soon Japan should lose", 0, 0, 18),
    ]


def august_setup():
    return [
        stack("1902", ["sov-36a"]), stack("2004", ["sov-39a", "sov-6gta", "sov-fuel"]), stack("1307", ["sov-pliyev"]),
        stack("2801", ["sov-2rb"]), stack("3303", ["sov-15a"]), stack("3206", ["sov-5a", "sov-1rb"]), stack("3109", ["sov-25a"]),
        stack("2102", ["kw-fz-hailar", "kw-fort"]), stack("2204", ["kw-fz-arshaan"]), stack("2305", ["kw-44a"]), stack("2802", ["kw-fz-sunwu", "kw-4a"]),
        stack("3107", ["kw-5a"]), stack("3207", ["kw-fz-dongning", "kw-3a"]), stack("3404", ["kw-fz-hutou"]), stack("2708", ["kw-30a", "jp-depot-changchun"]),
        stack("2510", ["pup-manchukuo2", "jp-depot-mukden"]), stack("2805", ["pup-manchukuo1", "jp-depot-harbin"]), stack("1712", ["pup-mengjiang", "jp-depot-kalgan"]),
        stack("2912", ["kr-34a"]), stack("1812", ["jp-d63"]),
        stack("1912", ["ccp-jrl"]), stack("1613", ["ccp-jcj"]), stack("2011", ["ccp-presence"]), stack("2110", ["ccp-presence"]), stack("1811", ["ccp-presence"]),
        note("2004", "Trans-Baikal Front: the tank army|and 39 Army at Tamsag Bulag", 0, 74, 18, "middle", SOV),
        note("1307", "Pliyev's cavalry-mechanized|group at Sain Shand", 44, 64, 18, "start", SOV),
        note("3206", "1st Far Eastern Front|at Suifenhe", 0, 68, 18, "middle", SOV),
        note("2710", "the Tonghua redoubt", 0, 44, 17, "middle", JP),
        note("2801", "2nd Far Eastern Front|on the Amur", 70, 10, 18, "start", SOV),
    ]


def august_play():
    return [
        stack("2504", ["sov-36a"]), stack("2307", ["sov-6gta", "mk-halted", "sov-fuel"]), stack("2408", ["sov-39a"]), stack("1910", ["sov-pliyev"]),
        stack("2703", ["sov-2rb"]), stack("3104", ["sov-15a"]), stack("3107", ["sov-5a", "sov-1rb"]), stack("2912", ["sov-25a"]),
        stack("2805", ["sov-airborne", "sov-occupied"]), stack("2708", ["sov-airborne", "sov-occupied"]), stack("2413", ["sov-airborne", "mk-port-denied"]),
        stack("2510", ["sov-occupied", "jp-surrendered"]),
        stack("2102", ["kw-fz-hailar", "kw-fort"]), stack("3207", ["kw-fz-dongning", "kw-3a"]), stack("3404", ["kw-fz-hutou"]), stack("2710", ["kw-30a", "kw-5a"]),
        stack("1712", ["ccp-jcj", "jp-depot-kalgan", "ccp-control"]), stack("2212", ["ccp-jrl", "ccp-control"]), stack("1812", ["jp-d63", "jp-surrendered"]),
        arrow(["1902", "2102", "2303", "2504"], SOV, label="36 Army: Hailar invested 10-18 Aug,|the Boketu pass, Qiqihar", at="2303", dx=-30, dy=70, anchor="end"),
        arrow(["2004", "2107", "2307", "2408", "2708"], SOV, label="6 Gds Tank Army over the Khingan;|halted for fuel at Lubei 12-14 Aug", at="2307", dx=-50, dy=76, anchor="end"),
        arrow(["2408", "2510"], SOV),
        arrow(["1307", "1408", "1509", "1910", "1712"], SOV, label="Pliyev: Erenhot, Sonid, Dolonnor,|Kalgan by 21 Aug", at="1509", dx=0, dy=-62),
        arrow(["2801", "2802", "2703", "2805"], SOV, label="2 Red Banner: Sunwu, Beian", at="2703", dx=-70, dy=0, anchor="end"),
        arrow(["3303", "3104", "2805"], SOV, label="15 Army and the flotilla|up the Sungari", at="3104", dx=0, dy=-66),
        arrow(["3206", "3107", "2807", "2805"], SOV, label="Mutanchiang 12-16 Aug:|the heaviest fighting", at="3107", dx=-80, dy=10, anchor="end"),
        arrow(["3109", "3110", "2912"], SOV, label="25 Army into Korea", at="2912", dx=-70, dy=0, anchor="end"),
        arrow(["2708", "2710"], JP, label="30 and 5 Armies toward the redoubt", at="2710", dx=0, dy=52),
        arrow(["1912", "1712"], CCP, label="Eighth Route Army: Kalgan 23 Aug", at="1712", dx=0, dy=-58),
        arrow(["2011", "2212"], CCP, label="Shanhaiguan 30 Aug", at="2212", dx=0, dy=54),
        burst("2102", None), burst("3107", None), burst("3207", "Dongning to 26 Aug", dx=70, dy=10, anchor="start"), burst("3404", "Hutou to 26 Aug", dx=0, dy=52),
        burst("2802", None),
        note("2805", "airborne detachments take Harbin,|Changchun, Mukden 18-20 Aug,|Dairen 22 Aug", 100, 40, 18, "start", SOV),
        note("1812", "surrender 15 Aug: directive|Hold for the KMT", 64, 10, 18, "start", JP),
    ]


def race_setup():
    yantai = H(37.46, 121.45)
    return [
        stack("1812", ["jp-d63", "jp-surrendered"]), stack("1914", ["jp-d110", "jp-surrendered"]), stack("1916", ["jp-d59", "jp-surrendered"]),
        stack("1919", ["jp-d65", "pup-nanjing2"]), stack("1917", ["jp-g-jinpu"]), stack("1614", ["jp-g-pinghan"]), stack("1618", ["jp-g-longhai"]),
        stack("1415", ["kmt-ga6"]), stack("1411", ["kmt-fu"]), stack("1218", ["kmt-ga34", "kmt-hq1"]), stack("1718", ["kmt-ga28"]), stack("1719", ["kmt-ga33"]),
        stack("2510", ["sov-occupied", "sov-6gta", "jp-depot-mukden"]), stack("2708", ["sov-occupied", "sov-39a", "jp-depot-changchun"]),
        stack("2805", ["sov-occupied", "sov-15a", "jp-depot-harbin"]), stack("2413", ["sov-occupied", "mk-port-denied"]), stack("2011", ["sov-pliyev"]),
        stack("2912", ["sov-25a"]),
        stack("1712", ["ccp-jcj", "jp-depot-kalgan", "ccp-control"]), stack("2212", ["ccp-jrl", "ccp-control"]), stack("2016", ["ccp-base", "ccp-sd"]),
        stack(yantai, ["ccp-junks"]), stack("1516", ["ccp-base", "ccp-jjly"]), stack("1313", ["ccp-js"]), stack("2019", ["ccp-n4a3"]), stack("1819", ["ccp-n4a4"]),
        stack("1613", ["ccp-base"]), stack("1715", ["ccp-presence"]), stack("1816", ["ccp-presence"]), stack("2116", ["ccp-presence"]), stack("1515", ["ccp-presence"]),
        stack("2110", ["ccp-presence"]), stack("1912", ["ccp-presence"]),
        stack("2013", ["us-marines-tianjin"]), stack("2316", ["us-marines-qingdao"]), stack("2312", ["us-marines-qinhuangdao"]),
        note("1812", "surrendered garrisons hold|Peiping, Tientsin, Jinan for the KMT", 0, 64, 18, "middle", JP),
        note("2510", "Soviet occupation: cities denied|to the CCP while the treaty holds", 0, 68, 18, "middle", SOV),
        note("2013", "US Marines off the ports", 80, 0, 18, "start", US),
        note("1218", "Chiang's armies are far away:|the race depends on US lift", 70, 50, 18, "start", KMT),
    ]


def race_play():
    yantai = H(37.46, 121.45)
    handan = H(36.61, 114.49)
    changzhi = H(36.19, 113.12)
    sea = (C("2212")[0] + 420, C("2212")[1] + 380)
    return [
        stack("1812", ["kmt-94a", "kmt-ga22", "kmt-control"]), stack("1914", ["us-marines-tianjin", "kmt-control"]), stack("2216", ["us-marines-qingdao", "kmt-control"]),
        stack("1916", ["jp-d59", "jp-surrendered", "kmt-ga28"]), stack("2212", ["us-marines-qinhuangdao", "kmt-13a", "kmt-52a", "kmt-control"]),
        stack("2311", ["ccp-ne1"]), stack(handan, ["kmt-ga33", "kmt-fort"]), stack("1618", ["kmt-ga15"]),
        stack("2510", ["sov-occupied", "sov-6gta"]), stack("2708", ["sov-occupied", "sov-39a"]), stack("2805", ["sov-occupied", "sov-15a"]), stack("2413", ["sov-occupied", "mk-port-denied"]),
        stack("2411", ["ccp-ne2", "mk-port-denied"]), stack("2612", ["ccp-ne3"]), stack("2609", ["ccp-ne4"]), stack("2011", ["ccp-jrl", "ccp-control"]),
        stack("1712", ["ccp-jcj", "ccp-control"]), stack("2016", ["ccp-base", "ccp-sd"]), stack(yantai, ["ccp-junks"]), stack("1516", ["ccp-base", "ccp-jjly"]),
        stack("1715", ["mk-rail-interdicted"]), stack("1816", ["mk-rail-interdicted"]), stack("1917", ["mk-rail-broken"]), stack("1415", ["kmt-ga6"]),
        stack(changzhi, ["ccp-presence"]),
        arrow(["0724", "1812"], US, dash=True, label="October: armies flown to Peiping", at="1812", dx=0, dy=74),
        arrow([sea, "2312", "2212"], US, dash=True, label="November: 13 and|52 Armies by sea|to Qinhuangdao", at=(C("2212")[0] + 380, C("2212")[1] + 320), dx=0, dy=0),
        arrow(["2013", "1914"], US, dash=True), arrow(["2316", "2216"], US, dash=True, label="30 Sept-Oct: Marines hold|Tientsin, Qingdao, Qinhuangdao", at="2316", dx=0, dy=72),
        arrow(["2212", "2311"], KMT, label="Shanhaiguan 15-16 Nov,|Jinzhou 26 Nov", at="2311", dx=0, dy=-70),
        arrow(["1618", handan], KMT, label="KMT armies north|along the Pinghan", at=handan, dx=0, dy=-74),
        arrow([yantai, "2513", "2612"], CCP, label="Shandong troops cross the Bohai|by junk, Sept-Nov", at=yantai, dx=80, dy=-60, anchor="start"),
        arrow([yantai, "2412", "2411"], CCP),
        arrow(["2011", "2110", "2310", "2609"], CCP, label="Hebei and Jehol columns march|into Manchuria with the cadres", at="2110", dx=0, dy=-62),
        arrow(["2011", "2311"], CCP),
        arrow(["2708", "2805"], SOV, label="Soviet withdrawal begins|in stages (track)", at="2805", dx=-70, dy=0, anchor="end"),
        burst("2311", None), burst(handan, "Handan, Oct-Nov:|Legitimacy cost", dx=0, dy=74), burst(changzhi, "Shangdang, Sept-Oct", dx=-70, dy=0, anchor="end"),
        burst("2411", "Yingkou: landing blocked", dx=70, dy=-40, anchor="start"),
        note("2413", "Dairen denied to the KMT|by the Soviets", 80, 20, 18, "start", SOV),
        note("1917", "Jinpu railway campaign:|the line broken", 0, 84, 18, "middle", CCP),
    ]


A100 = dict(km=100, size=60.0, text=1.15, pad_top=150, notes=lambda: [])   # the action pictures' tables are in 100 km hex ids
PICTURES = [
    dict(A100, name="1-ichigo-setup", move={"1718": "2224", "1717": "2123"}, crop=SOUTH, title="Ichi-Go: the opening position, April 1944",
         sub="The CCP player commands the Japanese in the south, the KMT player the North China garrisons",
         title_counters=["jp-dir-ichigo1"], items=ichigo_setup),
    dict(A100, name="1-ichigo-play", crop=SOUTH, title="Ichi-Go: one way it plays, April to December 1944",
         sub="Three directives in turn; each operation draws on the shared Japanese pools that the KMT player also needs",
         title_counters=["jp-dir-ichigo3", "jp-pool-ops"], items=ichigo_play, legend=LEG_ALL),
    dict(A100, name="2-reflux-setup", crop=SOUTH, title="The reflux of 1945: the position in April",
         sub="The corridor is open but thinly held; the CCP player commands the Japanese pieces in the south",
         title_counters=["jp-dir-airfields"], items=reflux_setup),
    dict(A100, name="2-reflux-play", move={"1126": "1534", "1718": "2224"}, crop=SOUTH, title="The reflux of 1945: one way it plays, April to August",
         sub="The Japanese withdraw north and to the coast; the CCP expands into the ground they leave",
         title_counters=["jp-dir-coast", "jp-front-ccp"], items=reflux_play, legend=LEG_ALL),
    dict(A100, name="3-august-setup", pad_top=0, move={"2011": "2715"}, crop=NORTHEAST, title="August 1945: the eve of Soviet entry, 8 August",
         sub="Nine scripted groupings at their stars; a Kwantung Army of raw divisions behind its fortified zones",
         title_counters=["sov-directive", "mk-pacific"], items=august_setup),
    dict(A100, name="3-august-play", pad_top=0, crop=NORTHEAST, title="August 1945: one way it plays, 9 to 30 August",
         sub="The Soviet groupings follow their axes; the players choose the directive, the treaty and the start of the race",
         title_counters=["jp-dir-hold", "sov-withdrawal"], items=august_play, legend=LEG_ALL),
    dict(A100, name="4-race-setup", move={"1917": "2523", "2011": "2715"}, crop=NORTH, title="The race: the position in mid-September 1945",
         sub="Japan has surrendered; the factions race to occupy the cities, the railways and the arms dumps",
         title_counters=["us-lift", "kmt-legit", "ccp-legit"], items=race_setup),
    dict(A100, name="4-race-play", move={"1917": "2523"}, crop=NORTH, title="The race: one way it plays, October 1945 to January 1946",
         sub="Lift against march; the treaty, the withdrawal track and the Legitimacy cost of Chinese-on-Chinese fighting",
         title_counters=["ccp-position", "kmt-position", "sov-withdrawal"], items=race_play, legend=LEG_ALL),
]



# ------------------------------------------------------------------------------------------------ notes, on the 75 km sheet
# Each figure of the four major actions carries its notes here: placed in pixels of the exported image (1800 wide,
# E()), with leader lines to the pieces they name (targets as old 100 km hex ids, converted like the stacks).
def ichigo_setup_notes():
    return [
        callout(E(560, 400), "1618", "Yellow River rail bridge:|the only crossing for the Henan offensive", JP),
        callout(E(1340, 1070), "1623", "11 Army at Wuhan:|nine divisions", JP),
        callout(E(560, 1700), "1427", "Hengyang: fortified|city, 14 AF field", KMT),
        callout(E(250, 1330), "0724", "Chongqing: Alpha Force|forms here and at Kunming", KMT),
        callout(E(1790, 1150), "1820", "the New Fourth|Army north of|the Yangtze", CCP, anchor="end"),
    ]


def ichigo_play_notes():
    return [
        callout(E(560, 400), "1418", "I. Henan, April-May: the river|crossed, Luoyang besieged", JP),
        callout(E(1290, 1560), "1427", "II. Hunan, May-August:|Changsha falls, Hengyang|holds 47 days", JP),
        callout(E(560, 2100), "1129", "III. Guangxi, Sept-Nov:|Guilin and Liuzhou", JP),
        callout(E(270, 1460), "0827", "December: Dushan,|then the halt", JP),
        callout(E(560, 2280), "0831", "23 Army to Nanning", JP),
        callout(E(1735, 470), "1916", "the KMT player raids the bases|with the North China garrisons|while the shared pools are drawn down", JP, anchor="end"),
        callout(E(830, 700), "1617", "presence spreads along the|Pinghan as the garrisons thin", CCP),
        callout(E(583, 1295), "0727", "Alpha Force forms;|New 6 Army flown|back from Burma", KMT),
    ]


def reflux_setup_notes():
    return [
        callout(E(760, 1440), "1026", "Zhijiang: Alpha Force,|US-equipped, air-supplied", KMT),
        callout(E(1000, 1740), "1326", "20 Army at Baoqing,|facing the Xuefeng", JP),
        callout(E(560, 760), "1321", "Laohekou lost in April", JP),
        callout(E(30, 1530), ["0727", "0827"], "Y-Force group armies|back from Burma", KMT, anchor="start"),
        callout(E(1790, 1700), None, "the CCP player commands|the Japanese pieces in the south", JP, anchor="end"),
    ]


def reflux_play_notes():
    return [
        callout(E(250, 1510), "1126", "April-June: 20 Army over the Xuefeng|toward Zhijiang, repulsed and|pursued: Alpha Force's first battle", KMT),
        callout(E(1160, 1820), "1427", "May-August: 6 Area Army|withdraws north;|Guangxi abandoned", JP),
        callout(E(930, 650), "1621", "divisions to the Peiping axis|and the coast: control passes|to the KMT player", JP),
        callout(E(570, 2200), "1029", "Nanning, Liuzhou, Guilin|retaken July-August", KMT),
        callout(E(560, 880), "1321", "Laohekou", JP),
        callout(E(1600, 440), "1617", "presence concentrates into|a field force; rails interdicted", CCP),
        callout(E(1790, 1730), None, "the Pacific War track nears|Soviet entry; the players|weigh how soon Japan|should lose", "#111111", anchor="end"),
    ]


def august_setup_notes():
    return [
        callout(E(330, 600), "2004", "Trans-Baikal Front: the tank|army and 39 Army at Tamsag Bulag", SOV),
        callout(E(200, 860), "1307", "Pliyev's cavalry-|mechanized group|at Sain Shand", SOV),
        callout(E(1790, 660), "3206", "1st Far Eastern|Front at Suifenhe", SOV, anchor="end"),
        callout(E(1430, 120), ["2801", "3303"], "2nd Far Eastern|Front on the Amur", SOV),
        callout(E(1280, 1150), "2710", "the Tonghua redoubt", JP),
        callout(E(1000, 400), ["2102", "2204"], "fortified zones:|Hailar, Arshaan", JP),
        callout(E(800, 948), ["1912", "2011"], "Eighth Route Army|along the Great Wall", CCP),
    ]


def august_play_notes():
    return [
        callout(E(940, 150), "2504", "36 Army: Hailar|invested 10-18 Aug,|the Boketu pass, Qiqihar", SOV),
        callout(E(470, 560), "2307", "6 Gds Tank Army over the|Khingan; halted for fuel|at Lubei 12-14 Aug", SOV),
        callout(E(340, 790), "1910", "Pliyev: Erenhot, Sonid,|Dolonnor, Kalgan by 21 Aug", SOV),
        callout(E(1060, 600), ["2805", "2708"], "airborne detachments take|Harbin, Changchun, Mukden|18-20 Aug, Dairen 22 Aug", SOV),
        callout(E(1270, 800), "3107", "Mutanchiang 12-16 Aug:|the heaviest fighting", SOV),
        callout(E(1660, 470), "3404", "Hutou|to 26 Aug", JP),
        callout(E(1300, 1140), "2710", "30 and 5 Armies|toward the redoubt", JP),
        callout(E(610, 1262), ["1712", "2212"], "Eighth Route Army: Kalgan|23 Aug, Shanhaiguan 30 Aug", CCP),
        callout(E(420, 1150), "1812", "surrender 15 Aug: directive|Hold for the KMT", JP),
    ]


def race_setup_notes():
    return [
        callout(E(395, 1090), ["1812", "1914"], "surrendered garrisons hold|Peiping, Tientsin, Jinan|for the KMT", JP),
        callout(E(1530, 640), ["2510", "2708"], "Soviet occupation: cities|denied to the CCP while|the treaty holds", SOV),
        callout(E(1080, 1150), ["2013", "2316", "2312"], "US Marines|off the ports", US),
        callout(E(30, 1400), "1218", "Chiang's armies are far|away: the race|depends on US lift", KMT, anchor="start"),
        callout(E(560, 520), ["1712", "2212"], "the Eighth Route Army holds|Kalgan and Shanhaiguan", CCP),
    ]


def race_play_notes():
    g = SIZE[0] * 0.55
    return [
        callout(E(520, 1100), "1812", "October: armies flown|to Peiping", US),
        callout(E(1500, 1220), "2212", "November: 13 and|52 Armies by sea|to Qinhuangdao", US),
        callout(E(1070, 760), "2311", "Shanhaiguan 15-16 Nov,|Jinzhou 26 Nov", KMT),
        callout(E(570, 1600), "1618", "KMT armies north|along the Pinghan", KMT),
        callout(E(1480, 1130), H(37.46, 121.45), "Shandong troops cross|the Bohai by junk,|Sept-Nov", CCP, gap=g),
        callout(E(1080, 540), "2609", "Hebei and Jehol columns march|into Manchuria with the cadres", CCP),
        callout(E(1640, 760), "2411", "Yingkou:|landing blocked", "#7a1f12"),
        callout(E(990, 1150), "2413", "Dairen denied|by the Soviets", SOV),
        callout(E(250, 1440), H(36.19, 113.12), "Shangdang,|Sept-Oct", "#7a1f12", gap=g),
        callout(E(760, 1255), H(36.61, 114.49), "Handan, Oct-Nov:|Legitimacy cost", "#7a1f12", gap=g),
        callout(E(990, 1440), "1917", "Jinpu railway|campaign", CCP),
        callout(E(1680, 420), "2805", "Soviet withdrawal|begins in stages", SOV),
    ]


NOTES = {"1-ichigo-setup": ichigo_setup_notes, "1-ichigo-play": ichigo_play_notes, "2-reflux-setup": reflux_setup_notes,
         "2-reflux-play": reflux_play_notes, "3-august-setup": august_setup_notes, "3-august-play": august_play_notes,
         "4-race-setup": race_setup_notes, "4-race-play": race_play_notes}
for _p in PICTURES:
    if _p["name"] in NOTES:
        _p["notes"] = NOTES[_p["name"]]


from maneuvers import maneuvers   # noqa: E402  (the studies of Changsha and Hengyang, in 75 km ids)
MANEUVERS = maneuvers(sys.modules[__name__])


# the memorandum's sections of figures: TeX name -> the picture whose crop the section's figures share
LOCATORS = [("Ichigo", "1-ichigo-setup"), ("Reflux", "2-reflux-setup"), ("August", "3-august-setup"),
            ("Race", "4-race-setup"), ("Changsha", "a1-changsha-position"), ("Hengyang", "b1-hengyang-position")]


def figure_box(p):
    """The crop of a picture in map px (x0, y0, x1, y1), as picture() computes it."""
    SRC_KM[0] = p.get("km")
    MOVE.clear()
    MOVE.update(p.get("move", {}))
    return crop_box(*p["crop"], pad_top=p.get("pad_top", 0))


def write_locator_boxes():
    """sections/locator-boxes.tex: one TikZ rectangle per section of figures, in fractions of the sheet image
    (x from the left, y from the bottom), clamped to the sheet; the memorandum draws it over the full map."""
    by_name = {p["name"]: p for p in PICTURES + MANEUVERS}
    o = ["% Copyright Ben Paul Wise. All Rights Reserved.",
         "% Generated by map_graphics/xml/tools/cd/illustrate.py; do not edit.  The area each section's figures show",
         "% on the standard sheet, as a TikZ rectangle in fractions of the sheet (x from the left, y from the bottom)."]
    W, Hh = float(L.SHEET_W), float(L.SHEET_H)
    for tex, name in LOCATORS:
        x0, y0, x1, y1 = figure_box(by_name[name])
        x0, x1 = max(0.0, x0) / W, min(W, x1) / W
        top, bottom = 1.0 - max(0.0, y0) / Hh, 1.0 - min(Hh, y1) / Hh
        o.append("\\def\\cdbox%s{(%.4f,%.4f) rectangle (%.4f,%.4f)}   %% %s" % (tex, x0, bottom, x1, top, name))
    o.append("% Copyright Ben Paul Wise. All Rights Reserved.")
    path = os.path.join(REPO, "Circling Dragons", "sections", "locator-boxes.tex")
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(o) + "\n")
    print("wrote", os.path.relpath(path, REPO))


def main(argv):
    only = argv[argv.index("--only") + 1] if "--only" in argv else None
    export = "--no-export" not in argv
    write_locator_boxes()
    if "--boxes" in argv:
        return 0
    os.makedirs(OUT, exist_ok=True)
    for p in PICTURES + MANEUVERS:
        if only and only not in p["name"]:
            continue
        svg = picture(p)
        path = os.path.join(OUT, p["name"] + ".svg")
        open(path, "w", encoding="utf-8").write(svg)
        print("wrote", os.path.basename(path), len(svg) // 1024, "KB")
        if export:
            png = os.path.join(OUT, p["name"] + ".png")
            subprocess.run([INKSCAPE, path, "--export-type=png", "--export-filename=" + png, "--export-width=1800",
                            "--export-background=#ffffff"], check=True)
            print("wrote", os.path.basename(png))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
