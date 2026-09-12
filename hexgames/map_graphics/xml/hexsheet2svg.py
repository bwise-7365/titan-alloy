#!/usr/bin/env python
"""hexsheet2svg.py -- reference renderer for hexsheet XML map sheets.

    python hexsheet2svg.py sheet.xml [out.svg] [--png] [--scale S]

Validates against hexsheet.xsd (beside this script), writes SVG, and with --png
rasterises through Inkscape.  Layers are emitted in the taxonomy's fixed order:
terrain, regions, grid, edges, links, rings, hex glyphs, side glyphs, labels, panels.
"""
import math
import os
import re
import subprocess
import sys
from xml.sax.saxutils import escape

from lxml import etree

HERE = os.path.dirname(os.path.abspath(__file__))
INKSCAPE = r"C:\Program Files\Inkscape\bin\inkscape.exe"
SQRT3 = math.sqrt(3)

# ---------------------------------------------------------------- geometry
# Screen coordinates, y down.  Angles in degrees, 0 = east, 90 = south.
FLAT = dict(
    corners={"e": 0, "se": 60, "sw": 120, "w": 180, "nw": 240, "ne": 300},
    edges={"se": 30, "s": 90, "sw": 150, "nw": 210, "n": 270, "ne": 330},
    edge_corners={"n": ("nw", "ne"), "ne": ("ne", "e"), "se": ("e", "se"),
                  "s": ("se", "sw"), "sw": ("sw", "w"), "nw": ("w", "nw")},
    # neighbour offsets (dc, dr) for shifted / unshifted columns
    nbr_shift={"n": (0, -1), "s": (0, 1), "ne": (1, 0), "se": (1, 1), "nw": (-1, 0), "sw": (-1, 1)},
    nbr_plain={"n": (0, -1), "s": (0, 1), "ne": (1, -1), "se": (1, 0), "nw": (-1, -1), "sw": (-1, 0)},
)
POINTY = dict(
    corners={"se": 30, "s": 90, "sw": 150, "nw": 210, "n": 270, "ne": 330},
    edges={"e": 0, "se": 60, "sw": 120, "w": 180, "nw": 240, "ne": 300},
    edge_corners={"e": ("ne", "se"), "se": ("se", "s"), "sw": ("s", "sw"),
                  "w": ("sw", "nw"), "nw": ("nw", "n"), "ne": ("n", "ne")},
    nbr_shift={"e": (1, 0), "w": (-1, 0), "ne": (1, -1), "se": (1, 1), "nw": (0, -1), "sw": (0, 1)},
    nbr_plain={"e": (1, 0), "w": (-1, 0), "ne": (0, -1), "se": (0, 1), "nw": (-1, -1), "sw": (-1, 1)},
)
OPPOSITE = {"n": "s", "s": "n", "ne": "sw", "sw": "ne", "nw": "se", "se": "nw", "e": "w", "w": "e"}


def letters(n):
    """1 -> A, 26 -> Z, 27 -> AA, 28 -> BB ... (doubled letters past Z)."""
    if n < 1:
        return "?"
    if n <= 26:
        return chr(64 + n)
    return chr(64 + n - 26) * 2


class Grid:
    def __init__(self, el):
        self.id = el.get("id") or "g"
        self.geom = FLAT if el.get("orientation") == "flat" else POINTY
        self.flat = el.get("orientation") == "flat"
        self.offset = el.get("offset")
        self.cols = int(el.get("cols"))
        self.rows = int(el.get("rows"))
        self.size = float(el.get("size"))
        self.ox = float(el.get("ox"))
        self.oy = float(el.get("oy"))
        self.fmt = el.get("id-format")
        self.c0 = int(el.get("col-start") or 1)
        self.r0 = int(el.get("row-start") or 1)
        self.cstep = int(el.get("col-step") or 1)
        self.rstep = int(el.get("row-step") or 1)
        self.terrain = el.get("terrain")
        self.show_ids = (el.get("show-ids") or "true") == "true"
        self.id_side = el.get("id-side") or "w"
        self.ids = {}     # printed id -> (c, r) zero-based
        self.cells = {}   # (c, r) -> printed id
        clipped = self.parse_clip(el.get("clip"))
        for c in range(self.cols):
            for r in range(self.rows):
                pid = self.make_id(c, r)
                if pid in clipped:
                    continue
                self.ids[pid] = (c, r)
                self.cells[(c, r)] = pid

    def parse_clip(self, s):
        out = set()
        if not s:
            return out
        for tok in s.split():
            if "-" in tok:
                a, b = tok.split("-", 1)
                out.update(self.expand_range(a, b))
            else:
                out.add(tok)
        return out

    def expand_range(self, a, b):
        # ranges are resolved by generating ids and taking those between a and b
        # in (col, row) order along one column or one row.
        ca = self.locate(a)
        cb = self.locate(b)
        out = set()
        if ca is None or cb is None:
            return out
        (c1, r1), (c2, r2) = ca, cb
        for c in range(min(c1, c2), max(c1, c2) + 1):
            for r in range(min(r1, r2), max(r1, r2) + 1):
                out.add(self.make_id(c, r))
        return out

    def locate(self, pid):
        for c in range(self.cols):
            for r in range(self.rows):
                if self.make_id(c, r) == pid:
                    return (c, r)
        return None

    def make_id(self, c, r):
        col = self.c0 + c * self.cstep
        row = self.r0 + r * self.rstep

        def sub(m):
            key, width = m.group(1), m.group(2)
            if key == "col":
                v = str(col)
            elif key == "row":
                v = str(row)
            elif key == "rowletter":
                v = letters(row)
            elif key == "colletter":
                v = letters(col)
            else:
                v = "?"
            if width:
                v = v.zfill(int(width))
            return v
        return re.sub(r"\{(col|row|rowletter|colletter)(?::0?(\d+))?\}", sub, self.fmt)

    def shifted(self, c, r):
        k = c if self.flat else r
        return (k % 2 == 1) if self.offset == "odd" else (k % 2 == 0)

    def centre(self, c, r):
        s = self.size
        if self.flat:
            x = self.ox + c * 1.5 * s
            y = self.oy + r * SQRT3 * s + (SQRT3 / 2 * s if self.shifted(c, r) else 0)
        else:
            x = self.ox + c * SQRT3 * s + (SQRT3 / 2 * s if self.shifted(c, r) else 0)
            y = self.oy + r * 1.5 * s
        return x, y

    def corner(self, c, r, name, inset=0.0):
        cx, cy = self.centre(c, r)
        a = math.radians(self.geom["corners"][name])
        d = self.size * (1 - inset)
        return cx + d * math.cos(a), cy + d * math.sin(a)

    def polygon(self, c, r, inset=0.0):
        return [self.corner(c, r, n, inset) for n in self.geom["corners"]]

    def edge_angle(self, d):
        return self.geom["edges"][d]

    def edge_mid(self, c, r, d):
        cx, cy = self.centre(c, r)
        a = math.radians(self.edge_angle(d))
        h = self.size * SQRT3 / 2
        return cx + h * math.cos(a), cy + h * math.sin(a)

    def edge_ends(self, c, r, d):
        a, b = self.geom["edge_corners"][d]
        return self.corner(c, r, a), self.corner(c, r, b)

    def neighbour(self, c, r, d):
        tbl = self.geom["nbr_shift"] if self.shifted(c, r) else self.geom["nbr_plain"]
        if d not in tbl:
            return None
        dc, dr = tbl[d]
        return c + dc, r + dr

    def slot_point(self, c, r, slot, k=0.5):
        cx, cy = self.centre(c, r)
        if slot == "c":
            return cx, cy
        ang = {"e": 0, "se": 45, "s": 90, "sw": 135, "w": 180, "nw": 225, "n": 270, "ne": 315}[slot]
        a = math.radians(ang)
        return cx + k * self.size * math.cos(a), cy + k * self.size * math.sin(a)


# ---------------------------------------------------------------- symbols
# Unit frame: hex circumradius = 1, centre at origin, y down.  "currentColor"
# takes the instance colour.  Side glyphs are drawn with the hexside along the
# x axis at y=0 and the hex interior toward -y.
SYMBOLS = {
    "position-badge": '<polygon points="0.42,0 0.21,0.364 -0.21,0.364 -0.42,0 -0.21,-0.364 0.21,-0.364" fill="currentColor" stroke="#fff" stroke-width="0.04"/>',
    "fire-intense": '<path d="M-0.2,0 A0.2,0.2 0 0 1 0.2,0 Z" fill="currentColor" stroke="#fff" stroke-width="0.02"/>',
    "fire-steady": '<path d="M-0.2,0 A0.2,0.2 0 0 1 0.2,0 Z" fill="currentColor" stroke="#fff" stroke-width="0.02"/><rect x="-0.045" y="-0.2" width="0.09" height="0.12" fill="#fff"/>',
    "fire-square": '<rect x="-0.09" y="-0.09" width="0.18" height="0.18" fill="currentColor" stroke="#fff" stroke-width="0.02"/>',
    "lvt-wreck": '<polygon points="0,-0.3 0.3,0 0,0.3 -0.3,0" fill="none" stroke="currentColor" stroke-width="0.07"/>',
    "arrival-box": '<rect x="-0.25" y="-0.25" width="0.5" height="0.5" transform="rotate(45)" fill="currentColor" stroke="#fff" stroke-width="0.03"/><path d="M0,-0.35 L0,-0.85 M-0.15,-0.7 L0,-0.85 L0.15,-0.7" fill="none" stroke="currentColor" stroke-width="0.09"/>',
    "artillery": '<rect x="-0.08" y="-0.22" width="0.36" height="0.24" fill="#fff" stroke="#444" stroke-width="0.02"/><circle cx="0.1" cy="-0.1" r="0.08" fill="#d22"/><path d="M-0.08,-0.22 L-0.08,0.25" stroke="#444" stroke-width="0.03"/>',
    "tank": '<rect x="-0.3" y="-0.08" width="0.6" height="0.22" rx="0.08" fill="currentColor"/><rect x="-0.12" y="-0.2" width="0.24" height="0.14" fill="currentColor"/><path d="M0.12,-0.13 L0.36,-0.13" stroke="currentColor" stroke-width="0.05"/>',
    "pier-head": '<circle r="0.26" fill="currentColor" stroke="#fff" stroke-width="0.04"/>',
    "city-major": '<rect x="-0.26" y="-0.26" width="0.52" height="0.52" fill="currentColor" stroke="#e8e8e8" stroke-width="0.06"/>',
    "city-minor": '<circle r="0.22" fill="#fff" stroke="#555" stroke-width="0.04"/>',
    "city": '<circle r="0.2" fill="currentColor" stroke="#222" stroke-width="0.04"/>',
    "capital": '<polygon points="0,-0.36 0.11,-0.12 0.36,-0.11 0.17,0.06 0.22,0.32 0,0.18 -0.22,0.32 -0.17,0.06 -0.36,-0.11 -0.11,-0.12" fill="currentColor" stroke="#222" stroke-width="0.04"/>',
    "town": '<rect x="-0.14" y="-0.14" width="0.28" height="0.28" fill="#fff" stroke="#333" stroke-width="0.04"/>',
    "port": '<circle r="0.22" fill="currentColor" stroke="#124" stroke-width="0.03"/><path d="M0,-0.14 L0,0.12 M-0.12,0 L0.12,0 M-0.13,0.04 A0.13,0.13 0 0 0 0.13,0.04" fill="none" stroke="#124" stroke-width="0.04"/><circle cy="-0.14" r="0.04" fill="none" stroke="#124" stroke-width="0.03"/>',
    "port-multi": '<circle r="0.3" fill="none" stroke="currentColor" stroke-width="0.05"/><circle r="0.22" fill="currentColor" stroke="#124" stroke-width="0.03"/><path d="M0,-0.14 L0,0.12 M-0.12,0 L0.12,0 M-0.13,0.04 A0.13,0.13 0 0 0 0.13,0.04" fill="none" stroke="#124" stroke-width="0.04"/>',
    "port-key": '<circle r="0.22" fill="currentColor" stroke="#124" stroke-width="0.03"/><path d="M0,-0.14 L0,0.12 M-0.12,0 L0.12,0 M-0.13,0.04 A0.13,0.13 0 0 0 0.13,0.04" fill="none" stroke="#124" stroke-width="0.04"/><rect x="-0.3" y="0.2" width="0.6" height="0.08" fill="#124"/>',
    "oil": '<path d="M-0.16,0.3 L-0.05,-0.3 L0.05,-0.3 L0.16,0.3 Z M-0.12,0.1 L0.12,0.1 M-0.09,-0.1 L0.09,-0.1" fill="none" stroke="currentColor" stroke-width="0.04"/>',
    "range-dot": '<circle r="0.06" fill="#fff"/>',
    "stacking": '<rect x="-0.17" y="-0.11" width="0.34" height="0.22" fill="#fff" stroke="#222" stroke-width="0.03"/><circle cx="-0.09" cy="0" r="0.035" fill="#222"/><circle cx="0" cy="0" r="0.035" fill="#222"/><circle cx="0.09" cy="0" r="0.035" fill="#222"/>',
    "star": '<polygon points="0,-0.22 0.065,-0.07 0.22,-0.07 0.1,0.03 0.13,0.2 0,0.11 -0.13,0.2 -0.1,0.03 -0.22,-0.07 -0.065,-0.07" fill="#fff" stroke="#222" stroke-width="0.03"/>',
    "aid": '<rect x="-0.13" y="-0.13" width="0.26" height="0.26" rx="0.04" fill="currentColor" stroke="#fff" stroke-width="0.03"/><path d="M0,-0.08 L0,0.08 M-0.08,0 L0.08,0" stroke="#fff" stroke-width="0.05"/>',
    "ice": '<path d="M0,-0.25 L0,0.25 M-0.22,-0.125 L0.22,0.125 M-0.22,0.125 L0.22,-0.125" stroke="currentColor" stroke-width="0.04"/>',
    "strait": '<path d="M-0.42,0 L0.42,0" stroke="currentColor" stroke-width="0.09"/><polygon points="-0.55,0 -0.35,-0.14 -0.35,0.14" fill="currentColor"/><polygon points="0.55,0 0.35,-0.14 0.35,0.14" fill="currentColor"/>',
    "strait-broken": '<path d="M-0.42,0 L-0.2,-0.1 L-0.05,0.1 L0.1,-0.1 L0.42,0" fill="none" stroke="currentColor" stroke-width="0.09"/><polygon points="-0.55,0 -0.35,-0.14 -0.35,0.14" fill="currentColor"/><polygon points="0.55,0 0.35,-0.14 0.35,0.14" fill="currentColor"/>',
    "arrow": '<path d="M0,0.1 L0,-0.7 M-0.16,-0.52 L0,-0.7 L0.16,-0.52" fill="none" stroke="currentColor" stroke-width="0.09"/>',
    "junction": '<rect x="-0.09" y="-0.09" width="0.18" height="0.18" fill="#fff" stroke="#222" stroke-width="0.03"/>',
    "dot": '<circle r="0.1" fill="currentColor"/>',
    "text": '',
}
DIR_ANGLE = {"e": 0, "se": 45, "s": 90, "sw": 135, "w": 180, "nw": 225, "n": 270, "ne": 315}


# ---------------------------------------------------------------- renderer
class Renderer:
    def __init__(self, xml_path):
        self.doc = etree.parse(xml_path)
        schema = etree.XMLSchema(etree.parse(os.path.join(HERE, "hexsheet.xsd")))
        if not schema.validate(self.doc):
            for e in schema.error_log:
                print("XSD: line %d: %s" % (e.line, e.message), file=sys.stderr)
            raise SystemExit("invalid: " + xml_path)
        self.root = self.doc.getroot()
        self.colors = {c.get("id"): c.get("value") for c in self.root.iter("color")}
        self.terrains = {t.get("id"): t for t in self.root.iter("terrain")}
        self.lines = {l.get("id"): l for l in self.root.iter("line")}
        self.grids = [Grid(g) for g in self.root.findall("grid")]
        self.font = self.root.get("font") or "Arial, Helvetica, sans-serif"
        self.warnings = []
        self.terrain_of = {}
        for g in self.grids:
            for pid in g.ids:
                self.terrain_of[pid] = g.terrain
        for h in self.root.findall("hexes"):
            for pid in h.get("ids").split():
                if self.find(pid):
                    self.terrain_of[pid] = h.get("terrain")
                else:
                    self.warn("unknown hex %s in <hexes>" % pid)
        for h in self.root.findall("hex"):
            if h.get("terrain") and self.find(h.get("id")):
                self.terrain_of[h.get("id")] = h.get("terrain")

    def warn(self, msg):
        self.warnings.append(msg)

    def col(self, cid, default="#000"):
        if cid is None:
            return default
        v = self.colors.get(cid)
        if v is None:
            self.warn("unknown colour %s" % cid)
            return default
        return v

    def find(self, pid):
        for g in self.grids:
            if pid in g.ids:
                return g, g.ids[pid]
        return None

    def find_or_warn(self, pid, ctx):
        f = self.find(pid)
        if f is None:
            self.warn("unknown hex %s in %s" % (pid, ctx))
        return f

    def parse_edge(self, ref, ctx):
        pid, d = ref.split(":")
        f = self.find_or_warn(pid, ctx)
        if f is None:
            return None
        g, (c, r) = f
        if d not in g.geom["edges"]:
            self.warn("edge %s: direction %s not valid for %s grid" % (ref, d, "flat" if g.flat else "pointy"))
            return None
        return g, c, r, d

    # ---- style helpers
    def line_attrs(self, lid):
        l = self.lines.get(lid)
        if l is None:
            self.warn("unknown line %s" % lid)
            return 'stroke="#000" stroke-width="1"', None
        a = 'stroke="%s" stroke-width="%s" fill="none" stroke-linejoin="round" stroke-linecap="round"' % (
            self.col(l.get("stroke")), l.get("width"))
        if l.get("dash"):
            a += ' stroke-dasharray="%s"' % l.get("dash")
        if l.get("opacity"):
            a += ' stroke-opacity="%s"' % l.get("opacity")
        return a, l

    def casing_attrs(self, l):
        if l is None or l.get("casing") is None:
            return None
        return 'stroke="%s" stroke-width="%s" fill="none" stroke-linejoin="round" stroke-linecap="round"' % (
            self.col(l.get("casing")), l.get("casing-width") or (float(l.get("width")) * 2.2))

    @staticmethod
    def pts(points):
        return " ".join("%.2f,%.2f" % p for p in points)

    @staticmethod
    def pathd(points):
        return "M" + " L".join("%.2f,%.2f" % p for p in points)

    # ---- main
    def render(self):
        R = self.root
        W, H = R.get("width"), R.get("height")
        out = []
        out.append('<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
                   'width="%s" height="%s" viewBox="0 0 %s %s" font-family="%s">' % (W, H, W, H, escape(self.font)))
        out.append('<title>%s</title>' % escape(R.get("title")))
        out.append(self.defs())
        out.append('<rect width="%s" height="%s" fill="%s"/>' % (W, H, self.col(R.get("background"), "#fff")))
        out.append(self.layer_terrain())
        out.append(self.layer_regions())
        out.append(self.layer_grid())
        out.append(self.layer_edges())
        out.append(self.layer_links())
        out.append(self.layer_rings())
        out.append(self.layer_hexglyphs())
        out.append(self.layer_sideglyphs())
        out.append(self.layer_labels())
        out.append(self.layer_panels())
        out.append("</svg>")
        return "\n".join(out)

    def defs(self):
        d = ["<defs>"]
        for name, body in SYMBOLS.items():
            d.append('<symbol id="sym-%s" overflow="visible">%s</symbol>' % (name, body))
        d.append('<pattern id="pat-dots" width="6" height="6" patternUnits="userSpaceOnUse"><circle cx="3" cy="3" r="0.9" fill="#000" fill-opacity="0.25"/></pattern>')
        d.append('<pattern id="pat-hatch" width="6" height="6" patternUnits="userSpaceOnUse"><path d="M0,6 L6,0" stroke="#000" stroke-opacity="0.2" stroke-width="1"/></pattern>')
        d.append('<pattern id="pat-mottle" width="9" height="9" patternUnits="userSpaceOnUse"><circle cx="2" cy="3" r="1.6" fill="#000" fill-opacity="0.12"/><circle cx="6.5" cy="7" r="1.2" fill="#000" fill-opacity="0.12"/></pattern>')
        d.append('<pattern id="pat-palms" width="10" height="10" patternUnits="userSpaceOnUse"><path d="M5,8 L5,4 M5,4 L2,2 M5,4 L8,2 M5,4 L3,6 M5,4 L7,6" stroke="#2a7a2a" stroke-opacity="0.6" stroke-width="0.9" fill="none"/></pattern>')
        d.append("</defs>")
        return "\n".join(d)

    def layer_terrain(self):
        o = ['<g class="layer terrain">']
        for g in self.grids:
            for (c, r), pid in g.cells.items():
                t = self.terrains.get(self.terrain_of.get(pid))
                if t is None:
                    self.warn("unknown terrain for %s" % pid)
                    continue
                pts = self.pts(g.polygon(c, r))
                o.append('<polygon class="hex terr-%s" data-hex="%s" points="%s" fill="%s"/>' % (
                    t.get("id"), pid, pts, self.col(t.get("fill"))))
                if (t.get("pattern") or "none") != "none":
                    o.append('<polygon points="%s" fill="url(#pat-%s)"/>' % (pts, t.get("pattern")))
        o.append("</g>")
        return "\n".join(o)

    def layer_regions(self):
        o = ['<g class="layer regions">']
        for reg in self.root.findall("region"):
            hexes = reg.get("hexes").split()
            cls = "region layer-%s" % re.sub(r"\W", "-", reg.get("layer"))
            o.append('<g class="%s" data-name="%s">' % (cls, escape(reg.get("name"))))
            members = set(hexes)
            if reg.get("tint"):
                for pid in hexes:
                    f = self.find_or_warn(pid, "region %s" % reg.get("name"))
                    if f is None:
                        continue
                    g, (c, r) = f
                    o.append('<polygon points="%s" fill="%s" fill-opacity="%s"/>' % (
                        self.pts(g.polygon(c, r)), self.col(reg.get("tint")), reg.get("opacity") or "0.35"))
            if reg.get("outline"):
                attrs, _ = self.line_attrs(reg.get("outline"))
                segs = []
                for pid in hexes:
                    f = self.find(pid)
                    if f is None:
                        continue
                    g, (c, r) = f
                    for d in g.geom["edges"]:
                        n = g.neighbour(c, r, d)
                        npid = g.cells.get(n) if n else None
                        if npid not in members:
                            a, b = g.edge_ends(c, r, d)
                            segs.append("M%.2f,%.2f L%.2f,%.2f" % (a[0], a[1], b[0], b[1]))
                o.append('<path d="%s" %s/>' % (" ".join(segs), attrs))
            o.append("</g>")
        o.append("</g>")
        return "\n".join(o)

    def layer_grid(self):
        o = ['<g class="layer grid" fill="none">']
        for g in self.grids:
            for (c, r), pid in g.cells.items():
                t = self.terrains.get(self.terrain_of.get(pid))
                stroke = self.col(t.get("stroke")) if t is not None else "#888"
                o.append('<polygon points="%s" stroke="%s" stroke-width="%.2f"/>' % (
                    self.pts(g.polygon(c, r)), stroke, max(0.6, g.size * 0.035)))
        o.append("</g>")
        return "\n".join(o)

    def layer_edges(self):
        o = ['<g class="layer edges">']
        for p in self.root.findall("path"):
            attrs, l = self.line_attrs(p.get("line"))
            offset = float(p.get("offset") or 0)
            d = self.chain_path(p.get("edges").split(), offset, "path %s" % (p.get("name") or p.get("kind")))
            pid = ' id="%s"' % p.get("id") if p.get("id") else ""
            cas = self.casing_attrs(l)
            if cas:
                o.append('<path d="%s" %s/>' % (d, cas))
            o.append('<path%s class="path %s" data-name="%s" d="%s" %s/>' % (
                pid, escape(p.get("kind")), escape(p.get("name") or ""), d, attrs))
        for e in self.root.findall("edge"):
            pe = self.parse_edge(e.get("at"), "edge")
            if pe is None:
                continue
            g, c, r, d = pe
            if e.get("line"):
                attrs, l = self.line_attrs(e.get("line"))
                a, b = g.edge_ends(c, r, d)
                o.append('<path class="edge" d="M%.2f,%.2f L%.2f,%.2f" %s/>' % (a[0], a[1], b[0], b[1], attrs))
        o.append("</g>")
        return "\n".join(o)

    def chain_path(self, edges, offset, ctx):
        """Join consecutive hexsides into polylines; break where they do not touch."""
        parts = []
        cur = []
        last = None

        def near(p, q):
            return abs(p[0] - q[0]) < 0.5 and abs(p[1] - q[1]) < 0.5

        for ref in edges:
            pe = self.parse_edge(ref, ctx)
            if pe is None:
                continue
            g, c, r, d = pe
            a, b = g.edge_ends(c, r, d)
            if offset:
                cx, cy = g.centre(c, r)
                mx, my = g.edge_mid(c, r, d)
                L = math.hypot(cx - mx, cy - my)
                ux, uy = (cx - mx) / L, (cy - my) / L
                a = (a[0] + ux * offset, a[1] + uy * offset)
                b = (b[0] + ux * offset, b[1] + uy * offset)
            if last is None:
                cur = [a, b]
            elif near(last, a):
                cur.append(b)
            elif near(last, b):
                cur.append(a)
                a, b = b, a
            else:
                # maybe the previous edge was reversed
                if len(cur) == 2 and (near(cur[0], a) or near(cur[0], b)):
                    cur = [cur[1], cur[0]]
                    if near(cur[-1], a):
                        cur.append(b)
                    else:
                        cur.append(a); a, b = b, a
                else:
                    parts.append(cur)
                    cur = [a, b]
            last = cur[-1]
        if cur:
            parts.append(cur)
        return " ".join(self.pathd(p) for p in parts if len(p) >= 2)

    def layer_links(self):
        o = ['<g class="layer links">']
        for lk in self.root.findall("link"):
            attrs, l = self.line_attrs(lk.get("line"))
            pts = []
            for pid in lk.get("hexes").split():
                f = self.find_or_warn(pid, "link %s" % (lk.get("name") or lk.get("kind")))
                if f:
                    g, (c, r) = f
                    pts.append(g.centre(c, r))
            if len(pts) < 2:
                continue
            d = self.pathd(pts)
            owner = ' data-owner="%s"' % escape(lk.get("owner")) if lk.get("owner") else ""
            cas = self.casing_attrs(l)
            if cas:
                o.append('<path d="%s" %s/>' % (d, cas))
            o.append('<path class="link %s"%s data-name="%s" d="%s" %s/>' % (
                escape(lk.get("kind")), owner, escape(lk.get("name") or ""), d, attrs))
            if l is not None and (l.get("ticks") or "false") == "true":
                w = float(l.get("width"))
                o.append('<path d="%s" stroke="%s" stroke-width="%.2f" fill="none" stroke-dasharray="%.2f %.2f"/>' % (
                    d, self.col(l.get("stroke")), w * 3, w * 0.9, w * 2.6))
        o.append("</g>")
        return "\n".join(o)

    def layer_rings(self):
        o = ['<g class="layer rings" fill="none">']
        for h in self.root.findall("hex"):
            if not h.get("ring"):
                continue
            f = self.find_or_warn(h.get("id"), "hex ring")
            if f is None:
                continue
            g, (c, r) = f
            w = float(h.get("ring-width") or g.size * 0.12)
            o.append('<polygon points="%s" stroke="%s" stroke-width="%.2f" stroke-linejoin="round"/>' % (
                self.pts(g.polygon(c, r, inset=0.12)), self.col(h.get("ring")), w))
        o.append("</g>")
        return "\n".join(o)

    def use(self, sym, x, y, rot, scale, color, cls=""):
        return '<use xlink:href="#sym-%s" class="%s" transform="translate(%.2f,%.2f) rotate(%.1f) scale(%.2f)" style="color:%s"/>' % (
            sym, cls, x, y, rot, scale, color)

    def layer_hexglyphs(self):
        o = ['<g class="layer hexglyphs">']
        for h in self.root.findall("hex"):
            f = self.find_or_warn(h.get("id"), "hex glyphs")
            if f is None:
                continue
            g, (c, r) = f
            slot_count = {}
            for gl in h.findall("glyph"):
                slot = gl.get("slot") or "c"
                n = slot_count.get(slot, 0)
                slot_count[slot] = n + 1
                x, y = g.slot_point(c, r, slot)
                # several glyphs in one slot are spread horizontally
                x += (n - 0) * g.size * 0.36
                sym = gl.get("symbol")
                scale = g.size * float(gl.get("scale") or 1)
                color = self.col(gl.get("color"), "#333")
                rot = 0.0
                if gl.get("dir"):
                    rot = DIR_ANGLE[gl.get("dir")] - 270
                if sym != "text":
                    o.append(self.use(sym, x, y, rot, scale, color, "glyph " + sym))
                if gl.get("text"):
                    fs = g.size * 0.30 * float(gl.get("scale") or 1)
                    fill = "#fff" if sym == "position-badge" else "#111"
                    o.append('<text x="%.2f" y="%.2f" font-size="%.2f" font-weight="bold" text-anchor="middle" dominant-baseline="central" fill="%s">%s</text>' % (
                        x, y, fs, fill, escape(gl.get("text"))))
        o.append("</g>")
        return "\n".join(o)

    def layer_sideglyphs(self):
        o = ['<g class="layer sideglyphs">']
        for h in self.root.findall("hex"):
            f = self.find(h.get("id"))
            if f is None:
                continue
            g, (c, r) = f
            for s in h.findall("side"):
                d = s.get("dir")
                if d not in g.geom["edges"]:
                    self.warn("side %s:%s not valid for grid" % (h.get("id"), d))
                    continue
                x, y = g.edge_mid(c, r, d)
                rot = g.edge_angle(d) - 90
                o.append(self.use(s.get("symbol"), x, y, rot, g.size, self.col(s.get("color"), "#333"), "side " + s.get("symbol")))
        for e in self.root.findall("edge"):
            if not e.get("symbol"):
                continue
            pe = self.parse_edge(e.get("at"), "edge glyph")
            if pe is None:
                continue
            g, c, r, d = pe
            x, y = g.edge_mid(c, r, d)
            o.append(self.use(e.get("symbol"), x, y, g.edge_angle(d), g.size, self.col(e.get("color"), "#fff"), "edgeglyph"))
            if e.get("label"):
                o.append('<text x="%.2f" y="%.2f" font-size="%.2f" font-style="italic" text-anchor="middle" fill="#fff" stroke="#000" stroke-width="0.3" paint-order="stroke">%s</text>' % (
                    x, y + g.size * 0.75, g.size * 0.28, escape(e.get("label"))))
        o.append("</g>")
        return "\n".join(o)

    def layer_labels(self):
        o = ['<g class="layer labels">']
        # hex identifiers
        for g in self.grids:
            if not g.show_ids:
                continue
            fs = g.size * 0.22
            for (c, r), pid in g.cells.items():
                cx, cy = g.centre(c, r)
                ang = g.edge_angle(g.id_side)
                k = g.size * 0.70
                x = cx + k * math.cos(math.radians(ang))
                y = cy + k * math.sin(math.radians(ang))
                o.append('<text transform="translate(%.2f,%.2f) rotate(%d)" font-size="%.2f" text-anchor="middle" dominant-baseline="central" fill="#444" fill-opacity="0.8">%s</text>' % (
                    x, y, (ang + 90) % 360, fs, escape(pid)))
        # region labels
        for reg in self.root.findall("region"):
            if reg.get("label") and reg.get("label-at"):
                f = self.find_or_warn(reg.get("label-at"), "region label")
                if f:
                    g, (c, r) = f
                    x, y = g.centre(c, r)
                    o.append('<text x="%.2f" y="%.2f" font-size="%.2f" font-style="italic" text-anchor="middle" fill="#333" fill-opacity="0.8">%s</text>' % (
                        x, y, g.size * 0.32, escape(reg.get("label"))))
        # authored labels
        for lb in self.root.findall("label"):
            attrs = 'font-size="%s" text-anchor="%s" fill="%s"' % (lb.get("size"), lb.get("anchor") or "middle", self.col(lb.get("color"), "#111"))
            if (lb.get("weight") or "normal") == "bold":
                attrs += ' font-weight="bold"'
            if (lb.get("italic") or "false") == "true":
                attrs += ' font-style="italic"'
            if lb.get("spacing") and float(lb.get("spacing")) != 0:
                attrs += ' letter-spacing="%s"' % lb.get("spacing")
            if (lb.get("halo") or "false") == "true":
                attrs += ' stroke="#fff" stroke-width="%.2f" stroke-opacity="0.85" paint-order="stroke"' % (float(lb.get("size")) * 0.18)
            text = escape(lb.get("text"))
            if lb.get("path"):
                o.append('<text %s dominant-baseline="middle"><textPath xlink:href="#%s" startOffset="50%%">%s</textPath></text>' % (attrs, lb.get("path"), text))
                continue
            if lb.get("at"):
                f = self.find_or_warn(lb.get("at"), "label %s" % lb.get("text"))
                if f is None:
                    continue
                g, (c, r) = f
                x, y = g.slot_point(c, r, lb.get("slot") or "c", 0.9)
            else:
                x, y = float(lb.get("x") or 0), float(lb.get("y") or 0)
            ang = float(lb.get("angle") or 0)
            o.append('<text transform="translate(%.2f,%.2f) rotate(%.1f)" dominant-baseline="central" %s>%s</text>' % (x, y, ang, attrs, text))
        o.append("</g>")
        return "\n".join(o)

    def layer_panels(self):
        o = ['<g class="layer panels">']
        for p in self.root.findall("panel"):
            x, y, w, h = (float(p.get(k)) for k in ("x", "y", "w", "h"))
            rot = float(p.get("rotate") or 0)
            o.append('<g class="panel" id="%s" transform="translate(%.2f,%.2f) rotate(%.1f)">' % (p.get("id"), x, y, rot))
            o.append('<rect width="%.2f" height="%.2f" fill="%s" stroke="%s" stroke-width="1.2" rx="2"/>' % (
                w, h, self.col(p.get("fill"), "#f4f1e6"), self.col(p.get("stroke"), "#333")))
            if p.get("title"):
                t = p.get("title")
                fs = min(h * 0.22, 11, 1.9 * w / max(1, len(t)))   # never wider than the panel
                o.append('<text x="%.2f" y="%.2f" font-size="%.2f" font-weight="bold" text-anchor="middle" fill="#111">%s</text>' % (
                    w / 2, min(h * 0.25, 14), fs, escape(t)))
            for ch in p:
                if ch.tag == "text":
                    fw = ' font-weight="%s"' % ch.get("weight") if ch.get("weight") else ""
                    o.append('<text x="%s" y="%s" font-size="%s"%s fill="%s">%s</text>' % (
                        ch.get("x"), ch.get("y"), ch.get("size"), fw, self.col(ch.get("color"), "#111"), escape(ch.text or "")))
                elif ch.tag == "box":
                    bx, by, bw, bh = (float(ch.get(k)) for k in ("x", "y", "w", "h"))
                    o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s" stroke="#333" stroke-width="0.8"/>' % (
                        bx, by, bw, bh, self.col(ch.get("fill"), "#fff")))
                    if ch.get("label"):
                        o.append('<text x="%.2f" y="%.2f" font-size="%.2f" text-anchor="middle" fill="#111">%s</text>' % (
                            bx + bw / 2, by + min(bh * 0.4, 12), min(bh * 0.3, 9), escape(ch.get("label"))))
                elif ch.tag == "track":
                    tx, ty, cw, chh = (float(ch.get(k)) for k in ("x", "y", "cell-w", "cell-h"))
                    cells = ch.get("cells").split()
                    wrap = int(ch.get("wrap") or 0)
                    for i, lab in enumerate(cells):
                        if ch.get("direction") == "v":
                            cx, cy = tx, ty + i * chh
                        elif wrap:
                            cx, cy = tx + (i % wrap) * cw, ty + (i // wrap) * chh
                        else:
                            cx, cy = tx + i * cw, ty
                        o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s" stroke="#333" stroke-width="0.8"/>' % (
                            cx, cy, cw, chh, self.col(ch.get("fill"), "#fff")))
                        lab = lab.replace("_", " ")          # underscores keep multi-word cells together in the list
                        fs = min(min(cw, chh) * 0.45, 1.8 * cw / max(1, len(lab)))
                        o.append('<text x="%.2f" y="%.2f" font-size="%.2f" text-anchor="middle" fill="#111">%s</text>' % (
                            cx + cw / 2, cy + chh * 0.62, fs, escape(lab)))
                elif ch.tag == "table":
                    tx, ty, cw, chh = (float(ch.get(k)) for k in ("x", "y", "cell-w", "cell-h"))
                    fs = float(ch.get("size") or chh * 0.5)
                    for i, row in enumerate(ch.findall("row")):
                        for j, cell in enumerate(row.findall("cell")):
                            cx, cy = tx + j * cw, ty + i * chh
                            o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="#fff" stroke="#666" stroke-width="0.5"/>' % (cx, cy, cw, chh))
                            o.append('<text x="%.2f" y="%.2f" font-size="%.2f" text-anchor="middle" fill="#111">%s</text>' % (
                                cx + cw / 2, cy + chh * 0.66, fs, escape(cell.text or "")))
            o.append("</g>")
        o.append("</g>")
        return "\n".join(o)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    src = argv[1]
    out = None
    png = "--png" in argv
    scale = 1.0
    for i, a in enumerate(argv[2:], 2):
        if a == "--scale":
            scale = float(argv[i + 1])
        elif not a.startswith("--") and (i == 2 or argv[i - 1] != "--scale"):
            out = a
    if out is None:
        out = os.path.splitext(src)[0] + ".svg"
    r = Renderer(src)
    svg = r.render()
    with open(out, "w", encoding="utf-8") as f:
        f.write(svg)
    for w in sorted(set(r.warnings)):
        print("warning:", w, file=sys.stderr)
    print("wrote", out, "(%d warnings)" % len(set(r.warnings)))
    if png:
        pngout = os.path.splitext(out)[0] + ".png"
        W = float(r.root.get("width")) * scale
        subprocess.run([INKSCAPE, out, "--export-type=png", "--export-filename=" + pngout,
                        "--export-width=%d" % int(W), "--export-background=#ffffff"], check=True)
        print("wrote", pngout)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
