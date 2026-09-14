#!/usr/bin/env python
# Copyright Ben Paul Wise. All Rights Reserved.
"""counters2svg.py -- reference renderer for hexcounters XML.

    python counters2svg.py set.xml [--png] [--dpi N]

Validates against hexcounters.xsd (beside this script) and writes, per sheet,
<set>-<sheet>-front.svg, -back.svg and -both.svg (front and back side by side,
as on a proof sheet).  Front and back come from one grid function: the back is
the front grid mirrored, and crop marks are drawn at identical positions on both,
so a duplex print cuts true.  Physical units are millimetres.
"""
import math
import os
import subprocess
import sys
from xml.sax.saxutils import escape

from lxml import etree

HERE = os.path.dirname(os.path.abspath(__file__))
INKSCAPE = r"C:\Program Files\Inkscape\bin\inkscape.exe"

# slot anchor points on the unit square, and default text size (small, medium, large) in face units
SLOTS = {
    "UL": (8, 12, "start"), "TC": (50, 13, "middle"), "UR": (92, 12, "end"),
    "ML": (12, 44, "middle"), "MR": (88, 44, "middle"),
    "LL": (7, 91, "start"), "BOTTOM": (50, 92, "middle"), "LR": (93, 91, "end"),
    "BAND": (50, 92, "middle"), "CENTRE": (50, 44, "middle"), "LCOL": (8, 34, "middle"),
}
SIZES = {"small": 8, "medium": 10, "large": 19}
ECHELON = {"company": "I", "battalion": "II", "regiment": "III", "brigade": "X", "division": "XX",
           "corps": "XXX", "army": "XXXX", "army-group": "XXXXX", "high-command": "XXXXXX"}
BOX_W, BOX_H, BOX_CX, BOX_CY = 40.0, 26.0, 50.0, 44.0


class Renderer:
    def __init__(self, path):
        self.doc = etree.parse(path)
        schema = etree.XMLSchema(etree.parse(os.path.join(HERE, "hexcounters.xsd")))
        if not schema.validate(self.doc):
            for e in schema.error_log:
                print("XSD: line %d: %s" % (e.line, e.message), file=sys.stderr)
            raise SystemExit("invalid: " + path)
        self.root = self.doc.getroot()
        self.path = path
        self.colors = {c.get("id"): c.get("value") for c in self.root.iter("color")}
        self.styles = {s.get("id"): s for s in self.root.iter("style")}
        self.counters = {c.get("id"): c for c in self.root.findall("counter")}
        self.size = float(self.root.get("size") or 12.7)
        self.corner = float(self.root.get("corner") or 0)
        self.font = self.root.get("font") or "Arial Narrow, Arial, sans-serif"
        self.warnings = []

    def col(self, cid, default="#000000"):
        if cid is None:
            return default
        v = self.colors.get(cid)
        if v is None:
            self.warnings.append("unknown colour " + cid)
            return default
        return v

    # ------------------------------------------------------------ faces
    def face_svg(self, face, counter):
        """Return SVG for one face in a 100x100 frame (no outer <svg>)."""
        sid = face.get("style") or counter.find("front").get("style")
        if sid not in self.styles:
            self.warnings.append("counter %s: no style" % counter.get("id"))
            sid = next(iter(self.styles))
        st = self.styles[sid]
        ground = self.col(face.get("ground") or st.get("ground"))
        textc = self.col(face.get("text") or st.get("text"))
        o = []
        r = self.corner * 100
        # a hairline cut line on every tile, so white grounds stay visible on a white sheet
        o.append('<rect width="100" height="100" rx="%.1f" fill="%s" stroke="#8a8a8a" stroke-width="0.6"/>' % (r, ground))
        if face.get("split"):
            o.append('<polygon points="100,0 100,100 0,0" fill="%s"/>' % self.col(face.get("split")))
        band = face.find("band")
        band_top = None
        if band is not None:
            h = float(band.get("height") or 16)
            band_top = 100 - h - 4
            o.append('<rect x="0" y="%.1f" width="100" height="%.1f" fill="%s"/>' % (band_top, h, self.col(band.get("color"))))
            if band.text and band.text.strip():
                o.append(self.text(50, band_top + h * 0.7, band.text.strip(), 9, self.col(band.get("text-color"), "#ffffff"), "middle", bold=True))
        for el in face:
            tag = el.tag
            if tag == "symbol":
                o.append(self.symbol(el, st, textc))
            elif tag == "silhouette":
                o.append(self.silhouette(el, textc))
            elif tag == "emblem":
                o.append(self.emblem(el, textc))
            elif tag == "echelon":
                o.append(self.text(BOX_CX, BOX_CY - BOX_H / 2 - 4, ECHELON[el.get("level")], 9,
                                   self.col(el.get("color"), textc), "middle", bold=True, spacing=-0.5))
            elif tag == "value":
                fs = SIZES[el.get("size") or "large"]
                x = {"start": 8, "middle": 50, "end": 92}[el.get("anchor") or "middle"]
                o.append(self.text(x, 93, (el.text or "").strip(), fs, self.col(el.get("color"), textc), el.get("anchor") or "middle", bold=True))
            elif tag == "text":
                o.append(self.slot_text(el, textc))
            elif tag == "steps":
                o.append(self.steps(el, textc))
            elif tag == "glyph":
                o.append(self.glyph(el, textc))
            elif tag == "tile":
                o.append(self.tile(el, textc))
        return "\n".join(o)

    def text(self, x, y, s, size, color, anchor="middle", bold=False, italic=False, rotate=0, spacing=0):
        attrs = 'x="0" y="0" font-size="%.1f" text-anchor="%s" fill="%s" dominant-baseline="central"' % (size, anchor, color)
        if bold:
            attrs += ' font-weight="bold"'
        if italic:
            attrs += ' font-style="italic"'
        if spacing:
            attrs += ' letter-spacing="%.1f"' % spacing
        lines = s.split("|")
        out = ['<g transform="translate(%.1f,%.1f) rotate(%.1f)">' % (x, y, rotate)]
        n = len(lines)
        for i, ln in enumerate(lines):
            dy = (i - (n - 1) / 2) * size * 1.05
            out.append('<text %s transform="translate(0,%.1f)">%s</text>' % (attrs, dy, escape(ln)))
        out.append("</g>")
        return "".join(out)

    def slot_text(self, el, textc):
        slot = el.get("slot")
        if slot == "free":
            x, y, anchor = float(el.get("x") or 50), float(el.get("y") or 50), "middle"
        else:
            x, y, anchor = SLOTS[slot]
        size = SIZES[el.get("size") or "small"]
        return self.text(x, y, (el.text or "").strip(), size, self.col(el.get("color"), textc), anchor,
                         bold=(el.get("weight") == "bold"), italic=(el.get("italic") == "true"),
                         rotate=float(el.get("rotate") or 0))

    # ------------------------------------------------------------ symbol box
    def symbol(self, el, st, textc):
        icon = el.get("icon")
        if icon == "none":
            return ""
        sc = float(el.get("scale") or 1)
        w, h = BOX_W * sc, BOX_H * sc
        x0, y0 = BOX_CX - w / 2, BOX_CY - h / 2
        stroke = self.col(el.get("stroke") or st.get("box-stroke"))
        fill = self.col(el.get("fill") or st.get("box-fill"), "none") if (el.get("fill") or st.get("box-fill")) else "none"
        sw = 2.2
        o = ['<g class="symbol %s">' % icon]
        # square corners on the box, always
        o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (x0, y0, w, h, fill, stroke, sw))
        if el.get("partial") in ("left", "right"):
            px = x0 if el.get("partial") == "left" else x0 + w * 0.62
            o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"/>' % (px, y0, w * 0.38, h, stroke))
        ln = 'fill="none" stroke="%s" stroke-width="%.1f" stroke-linecap="round" stroke-linejoin="round"' % (stroke, sw)
        x1, y1 = x0 + w, y0 + h
        cx, cy = BOX_CX, BOX_CY
        if icon in ("infantry", "heavy-infantry", "machine-gun", "garrison", "mechanized", "airborne"):
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f" %s/>' % (x0, y0, x1, y1, x0, y1, x1, y0, ln))
        if icon == "machine-gun":
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f" %s/>' % (cx, y0, cx, y1, ln))
        if icon == "garrison":
            o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s" stroke="%s" stroke-width="%.1f"/>' % (cx, cy, h * 0.16, fill if fill != "none" else "#ffffff", stroke, sw * 0.7))
        if icon in ("armour", "mechanized", "motorized", "cav-mech"):
            # a stadium: fully rounded ends, never a rectangle
            ow, oh = w * 0.62, h * 0.55
            o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f" ry="%.1f" %s/>' % (cx - ow / 2, cy - oh / 2, ow, oh, oh / 2, oh / 2, ln))
        if icon == "motorized":
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f" %s/>' % (x0, y0, x1, y1, x0, y1, x1, y0, ln))
        if icon in ("cavalry", "cav-mech"):
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f" %s/>' % (x0, y1, x1, y0, ln))
        if icon == "engineer":
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f" %s/>' % (
                cx - w * 0.28, cy + h * 0.22, cx + w * 0.28, cy + h * 0.22,
                cx - w * 0.28, cy - h * 0.22, cx - w * 0.28, cy + h * 0.22,
                cx, cy - h * 0.22, cx, cy + h * 0.22,
                cx + w * 0.28, cy - h * 0.22, cx + w * 0.28, cy + h * 0.22, ln))
        if icon == "artillery":
            o.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="%s"/>' % (cx, cy, h * 0.2, stroke))
        if icon == "anti-tank":
            o.append('<path d="M%.1f,%.1f L%.1f,%.1f L%.1f,%.1f" %s/>' % (x0, y1, cx, y0, x1, y1, ln))
        if icon == "hq":
            o.append(self.text(cx, cy, el.get("text") or "HQ", h * 0.62, stroke, "middle", bold=True))
        if icon == "hq-checker":
            n, m = 4, 3
            for i in range(n):
                for j in range(m):
                    if (i + j) % 2 == 0:
                        o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s"/>' % (x0 + i * w / n, y0 + j * h / m, w / n, h / m, stroke))
        if icon == "fortress":
            pass
        if icon == "port-a-fort":
            for k in (-0.25, 0, 0.25):
                o.append('<path d="M%.1f,%.1f L%.1f,%.1f" %s/>' % (x0 + w * 0.15, cy + k * h, x1 - w * 0.15, cy + k * h, ln))
        if icon in ("worker", "partisan", "text"):
            t = {"worker": "W", "partisan": "P"}.get(icon, el.get("text") or "")
            o.append(self.text(cx, cy, t, h * 0.62, stroke, "middle", bold=True))
        for mod in (el.get("modifiers") or "").split():
            if mod == "mountain":
                o.append('<polygon points="%.1f,%.1f %.1f,%.1f %.1f,%.1f" fill="%s"/>' % (cx - w * 0.12, y1, cx + w * 0.12, y1, cx, y1 - h * 0.42, stroke))
            elif mod == "airborne":
                o.append('<path d="M%.1f,%.1f A%.1f,%.1f 0 0 1 %.1f,%.1f" %s/>' % (cx - w * 0.2, y0 + h * 0.34, w * 0.2, h * 0.3, cx + w * 0.2, y0 + h * 0.34, ln))
            elif mod == "marine":
                o.append('<path d="M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f A%.1f,%.1f 0 0 0 %.1f,%.1f" %s/>' % (
                    cx, y1 - h * 0.46, cx, y1 - h * 0.08, cx - w * 0.1, y1 - h * 0.36, cx + w * 0.1, y1 - h * 0.36,
                    cx - w * 0.14, y1 - h * 0.2, w * 0.14, h * 0.15, cx + w * 0.14, y1 - h * 0.2, ln))
            elif mod == "reserve":
                o.append(self.text(cx, y0 - 3, "Res", 6, stroke, "middle"))
        if el.get("text") and icon not in ("hq", "text"):
            o.append(self.text(cx, y1 + 5, el.get("text"), 6, textc, "middle"))
        o.append("</g>")
        return "\n".join(o)

    # ------------------------------------------------------------ silhouettes and emblems
    def silhouette(self, el, textc):
        kind = el.get("kind")
        c = self.col(el.get("color"), textc)
        sc = float(el.get("scale") or 1)
        g = '<g class="silhouette %s" transform="translate(%.1f,%.1f) scale(%.2f)" fill="%s">' % (kind, BOX_CX, BOX_CY, sc, c)
        if el.get("href"):
            return g + '<image href="%s" x="-22" y="-14" width="44" height="28"/></g>' % escape(el.get("href"))
    # shapes in a 44 x 28 frame centred on the origin
        S = {
            "tank": '<rect x="-20" y="2" width="40" height="10" rx="5"/><rect x="-9" y="-6" width="18" height="9" rx="2"/><rect x="6" y="-4" width="17" height="2.4"/>',
            "lvt": '<path d="M-21,2 L21,2 L18,12 L-18,12 Z"/><rect x="-8" y="-6" width="14" height="9" rx="1.5"/><rect x="-21" y="4" width="42" height="3" fill="#ffffff" fill-opacity="0.5"/>',
            "truck": '<rect x="-20" y="-2" width="26" height="12" rx="1"/><rect x="6" y="2" width="13" height="8" rx="2"/><circle cx="-12" cy="11" r="3"/><circle cx="10" cy="11" r="3"/>',
            "aircraft": '<path d="M0,-13 L3,-4 L21,2 L21,5 L3,3 L2,10 L7,13 L7,15 L0,13 L-7,15 L-7,13 L-2,10 L-3,3 L-21,5 L-21,2 L-3,-4 Z"/>',
            "bomber": '<path d="M0,-13 L2.5,-6 L22,-1 L22,3 L11,3 L11,6 L7,6 L7,3 L2.5,4 L2,10 L9,13 L9,15 L0,13 L-9,15 L-9,13 L-2,10 L-2.5,4 L-7,3 L-7,6 L-11,6 L-11,3 L-22,3 L-22,-1 L-2.5,-6 Z"/>',
            "interceptor": '<path d="M0,-14 L2,-2 L14,6 L14,8 L2,5 L1,10 L5,13 L5,14 L0,12 L-5,14 L-5,13 L-1,10 L-2,5 L-14,8 L-14,6 L-2,-2 Z"/>',
            "ship": '<path d="M-22,4 L22,4 L17,12 L-18,12 Z"/><rect x="-8" y="-4" width="16" height="8"/><rect x="-3" y="-11" width="4" height="7"/><rect x="-16" y="0" width="5" height="4"/><rect x="10" y="0" width="5" height="4"/>',
            "carrier": '<path d="M-22,4 L22,4 L17,12 L-18,12 Z"/><rect x="-23" y="-1" width="46" height="5"/><rect x="8" y="-9" width="6" height="8"/>',
            "submarine": '<rect x="-22" y="0" width="44" height="9" rx="4.5"/><rect x="-4" y="-7" width="8" height="8" rx="1"/><rect x="-1" y="-11" width="2" height="4"/>',
            "locomotive": '<rect x="-20" y="-2" width="16" height="12"/><rect x="-4" y="2" width="24" height="8" rx="4"/><rect x="13" y="-5" width="4" height="7"/><circle cx="-13" cy="12" r="3"/><circle cx="-4" cy="12" r="3"/><circle cx="8" cy="12" r="3"/><circle cx="16" cy="12" r="3"/>',
            "rifle": '<path d="M-20,8 L-8,4 L-8,2 L18,-6 L20,-3 L-6,6 L-6,7 L-14,10 Z"/><rect x="0" y="-2" width="6" height="3" transform="rotate(-18)"/>',
        }
        return g + S.get(kind, "") + "</g>"

    def emblem(self, el, textc):
        kind = el.get("kind")
        c = self.col(el.get("color"), textc)
        c2 = self.col(el.get("color2"), "#ffffff")
        sc = float(el.get("scale") or 1)
        x, y, _ = SLOTS[el.get("slot") or "CENTRE"]
        g = '<g class="emblem %s" transform="translate(%.1f,%.1f) scale(%.2f)">' % (kind, x, y, sc)
        star = " ".join("%.1f,%.1f" % (14 * math.cos(math.radians(-90 + i * 36)) * (1 if i % 2 == 0 else 0.4),
                                       14 * math.sin(math.radians(-90 + i * 36)) * (1 if i % 2 == 0 else 0.4)) for i in range(10))
        S = {
            "star": '<polygon points="%s" fill="%s"/>' % (star, c),
            "cross": '<path d="M-4,-14 H4 V-4 H14 V4 H4 V14 H-4 V4 H-14 V-4 H-4 Z" fill="%s"/>' % c,
            "balkenkreuz": '<path d="M-5,-15 H5 V-5 H15 V5 H5 V15 H-5 V5 H-15 V-5 H-5 Z" fill="%s"/><path d="M-3.5,-13 H3.5 V-3.5 H13 V3.5 H3.5 V13 H-3.5 V3.5 H-13 V-3.5 H-3.5 Z" fill="none" stroke="%s" stroke-width="2"/>' % (c2, c),
            "roundel": '<circle r="14" fill="%s"/><circle r="9" fill="%s"/><circle r="4" fill="%s"/>' % (c, c2, c),
            "flag": '<rect x="-14" y="-12" width="2" height="26" fill="%s"/><rect x="-12" y="-12" width="22" height="14" fill="%s"/>' % (c, c2),
            "hexagon": '<polygon points="16,0 8,13.9 -8,13.9 -16,0 -8,-13.9 8,-13.9" fill="none" stroke="%s" stroke-width="3"/>' % c,
            "parachute": '<path d="M-14,0 A14,14 0 0 1 14,0 Z" fill="%s"/><path d="M-14,0 L-2,12 M14,0 L2,12 M0,0 L0,12" stroke="%s" stroke-width="1.2" fill="none"/><rect x="-3" y="12" width="6" height="4" fill="%s"/>' % (c, c, c),
            "anchor": '<path d="M0,-12 L0,10 M-9,-2 L9,-2 M-12,4 A12,10 0 0 0 12,4" fill="none" stroke="%s" stroke-width="3" stroke-linecap="round"/><circle cy="-13" r="3" fill="none" stroke="%s" stroke-width="2"/>' % (c, c),
            "pennant": '<rect x="-13" y="-12" width="2" height="26" fill="%s"/><polygon points="-11,-12 12,-6 -11,0" fill="%s"/>' % (c, c2),
            "cloud": '<ellipse cx="0" cy="-6" rx="13" ry="7" fill="%s"/><rect x="-3" y="-2" width="6" height="12" fill="%s"/><ellipse cx="0" cy="11" rx="8" ry="2.5" fill="%s"/>' % (c, c, c),
            "skull": '<circle cy="-3" r="9" fill="%s"/><rect x="-5" y="4" width="10" height="5" fill="%s"/><circle cx="-3.5" cy="-4" r="2.2" fill="%s"/><circle cx="3.5" cy="-4" r="2.2" fill="%s"/>' % (c, c, c2, c2),
        }
        return g + S.get(kind, "") + "</g>"

    # ------------------------------------------------------------ small marks
    def steps(self, el, textc):
        n = int(el.get("count") or 0)
        c = self.col(el.get("color"), textc)
        mark = el.get("mark") or "dot"
        slot = el.get("slot") or "LCOL"
        o = ['<g class="steps">']
        for i in range(n):
            if slot == "LCOL":
                x, y = 7, 30 + i * 8
            else:
                x, y = 93 - i * 7, 10
            if mark == "dot":
                o.append('<circle cx="%.1f" cy="%.1f" r="2.6" fill="%s"/>' % (x, y, c))
            else:
                o.append('<rect x="%.1f" y="%.1f" width="5" height="5" fill="%s"/>' % (x - 2.5, y - 2.5, c))
        o.append("</g>")
        return "".join(o)

    def glyph(self, el, textc):
        kind = el.get("kind")
        c = self.col(el.get("color"), textc)
        if el.get("x") and el.get("y"):
            x, y = float(el.get("x")), float(el.get("y"))
        else:
            x, y, _ = SLOTS[el.get("slot") or "LL"]
            if (el.get("slot") or "LL") in ("LL", "LR"):
                x = x + (4 if el.get("slot", "LL") == "LL" else -4)
        t = el.get("text") or ""
        if kind == "circle":
            return '<circle cx="%.1f" cy="%.1f" r="5" fill="%s"/>' % (x, y, c)
        if kind == "diamond":
            return '<polygon points="%.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f" fill="%s"/>' % (x, y - 6, x + 6, y, x, y + 6, x - 6, y, c)
        if kind == "triangle":
            return '<polygon points="%.1f,%.1f %.1f,%.1f %.1f,%.1f" fill="%s"/>' % (x, y - 6, x + 6, y + 5, x - 6, y + 5, c)
        if kind == "disc":
            return '<circle cx="%.1f" cy="%.1f" r="5" fill="%s" stroke="#ffffff" stroke-width="1"/>' % (x, y, c)
        if kind == "range":
            return '<circle cx="%.1f" cy="%.1f" r="6.5" fill="none" stroke="%s" stroke-width="1.5"/>%s' % (x - 3, y + 1, c, self.text(x - 3, y + 1, t, 8, c, "middle", bold=True))
        if kind == "drm":
            return '<rect x="%.1f" y="%.1f" width="13" height="11" fill="%s" stroke="#000000" stroke-width="0.6"/>%s' % (
                x - 9, y - 6, c, self.text(x - 2.5, y - 0.5, t, 8, "#ffffff" if c.lower() not in ("#ffffff",) else "#000000", "middle", bold=True))
        if kind in ("arrow-up", "arrow-down"):
            d = -1 if kind == "arrow-up" else 1
            return '<path d="M%.1f,%.1f L%.1f,%.1f M%.1f,%.1f L%.1f,%.1f L%.1f,%.1f" fill="none" stroke="%s" stroke-width="2"/>' % (
                x, y - 6 * d, x, y + 6 * d, x - 4, y + 2 * d, x, y + 6 * d, x + 4, y + 2 * d, c)
        if kind == "earmark":
            return '<rect x="%.1f" y="%.1f" width="8" height="2.5" fill="%s"/>' % (x - 4, y + 6, c)
        if kind == "hexagon":
            return '<polygon points="%s" fill="none" stroke="%s" stroke-width="3"/>' % (
                " ".join("%.1f,%.1f" % (50 + 34 * math.cos(math.radians(60 * i)), 50 + 34 * math.sin(math.radians(60 * i))) for i in range(6)), c)
        return ""

    def tile(self, el, textc):
        return '<rect x="20" y="24" width="60" height="44" rx="6" fill="%s" stroke="#333333" stroke-width="0.8"/>%s' % (
            self.col(el.get("fill")), self.text(50, 46, (el.text or "").strip(), 8, self.col(el.get("color"), "#111111"), "middle", bold=True))

    # ------------------------------------------------------------ backs
    def back_face(self, counter):
        back = counter.find("back")
        front = counter.find("front")
        if back is None:
            return None
        if back.get("ref"):
            return self.counters[back.get("ref")].find("front")
        if back.get("derived"):
            d = back.get("derived")
            new = etree.Element("front", attrib=dict(front.attrib))
            for k, v in back.attrib.items():
                if k not in ("derived", "ref"):
                    new.set(k, v)
            for el in front:
                if d == "concealed" and el.tag not in ("symbol", "silhouette"):
                    continue
                if d == "reduced" and el.tag in ("value", "steps") and back.find(el.tag) is not None:
                    continue
                if d == "concealed" and el.tag == "symbol":
                    e = etree.SubElement(new, "symbol", attrib=dict(el.attrib))
                    if "text" in e.attrib:
                        del e.attrib["text"]
                    continue
                new.append(etree.fromstring(etree.tostring(el)))
            for el in back:      # overrides and additions
                new.append(etree.fromstring(etree.tostring(el)))
            return new
        return back

    # ------------------------------------------------------------ sheets
    def render_sheet(self, sheet):
        cols, rows = int(sheet.get("cols")), int(sheet.get("rows"))
        gut = float(sheet.get("gutter") or 0)
        mar = float(sheet.get("margin") or 10)
        S = self.size
        W = mar * 2 + cols * S + (cols - 1) * gut
        H = mar * 2 + rows * S + (rows - 1) * gut
        cells = []
        for p in sheet.findall("place"):
            for _ in range(int(p.get("repeat") or 1)):
                cells.append(None if p.get("blank") == "true" else p.get("counter"))
        if len(cells) > cols * rows:
            self.warnings.append("sheet %s: %d places for %d cells" % (sheet.get("id"), len(cells), cols * rows))
        mirror = sheet.get("mirror") or "horizontal"

        def origin(i, side):
            c, r = i % cols, i // cols
            if side == "back":
                if mirror == "horizontal":
                    c = cols - 1 - c
                elif mirror == "vertical":
                    r = rows - 1 - r
            return mar + c * (S + gut), mar + r * (S + gut)

        def crop_marks():
            o = ['<g class="crop" stroke="#000000" stroke-width="0.15">']
            xs = sorted({mar + c * (S + gut) for c in range(cols)} | {mar + c * (S + gut) + S for c in range(cols)})
            ys = sorted({mar + r * (S + gut) for r in range(rows)} | {mar + r * (S + gut) + S for r in range(rows)})
            for x in xs:
                o.append('<path d="M%.3f,0 V%.3f M%.3f,%.3f V%.3f"/>' % (x, mar * 0.7, x, H - mar * 0.7, H))
            for y in ys:
                o.append('<path d="M0,%.3f H%.3f M%.3f,%.3f H%.3f"/>' % (y, mar * 0.7, W - mar * 0.7, y, W))
            # registration crosses at the four corners, identical on both faces
            for (x, y) in ((mar / 2, mar / 2), (W - mar / 2, mar / 2), (mar / 2, H - mar / 2), (W - mar / 2, H - mar / 2)):
                o.append('<path d="M%.3f,%.3f H%.3f M%.3f,%.3f V%.3f" stroke-width="0.25"/><circle cx="%.3f" cy="%.3f" r="%.3f" fill="none" stroke-width="0.25"/>' % (
                    x - mar / 3, y, x + mar / 3, x, y - mar / 3, y + mar / 3, x, y, mar / 4))
            o.append("</g>")
            return "\n".join(o)

        def face_group(face, cid, x, y):
            k = S / 100.0
            return '<g class="counter" data-id="%s" transform="translate(%.3f,%.3f) scale(%.5f)">%s</g>' % (
                escape(cid), x, y, k, self.face_svg(face, self.counters[cid]))

        out = {}
        for side in ("front", "back"):
            o = ['<svg xmlns="http://www.w3.org/2000/svg" width="%.2fmm" height="%.2fmm" viewBox="0 0 %.3f %.3f" font-family="%s">' % (W, H, W, H, escape(self.font))]
            o.append('<title>%s -- %s</title>' % (escape(sheet.get("title") or sheet.get("id")), side))
            o.append('<rect width="%.3f" height="%.3f" fill="%s"/>' % (W, H, self.col(sheet.get("background"), "#ffffff")))
            for i, cid in enumerate(cells):
                if cid is None or i >= cols * rows:
                    continue
                counter = self.counters.get(cid)
                if counter is None:
                    self.warnings.append("unknown counter " + cid)
                    continue
                face = counter.find("front") if side == "front" else self.back_face(counter)
                if face is None:
                    continue
                x, y = origin(i, side)
                o.append(face_group(face, cid, x, y))
            if (sheet.get("crop-marks") or "true") == "true":
                o.append(crop_marks())
            o.append('<text x="%.2f" y="%.2f" font-size="%.2f" fill="#333333">%s -- %s</text>' % (
                mar, H - mar * 0.15, mar * 0.35, escape(sheet.get("title") or sheet.get("id")), side))
            o.append("</svg>")
            out[side] = ("\n".join(o), W, H)
        return out

    def run(self, png=False, dpi=300):
        base = os.path.splitext(self.path)[0]
        written = []
        for sheet in self.root.findall("sheet"):
            res = self.render_sheet(sheet)
            for side, (svg, W, H) in res.items():
                p = "%s-%s-%s.svg" % (base, sheet.get("id"), side)
                open(p, "w", encoding="utf-8").write(svg)
                written.append((p, W, H))
            # proof: front and back side by side, as printed sheets often show them
            fsvg, W, H = res["front"]
            bsvg = res["back"][0]
            inner = lambda s: s[s.index(">", s.index("<svg")) + 1:s.rindex("</svg>")]
            both = '<svg xmlns="http://www.w3.org/2000/svg" width="%.2fmm" height="%.2fmm" viewBox="0 0 %.3f %.3f" font-family="%s">' % (
                2 * W + 5, H, 2 * W + 5, H, escape(self.font))
            both += '<g>%s</g><g transform="translate(%.3f,0)">%s</g></svg>' % (inner(fsvg), W + 5, inner(bsvg))
            p = "%s-%s-both.svg" % (base, sheet.get("id"))
            open(p, "w", encoding="utf-8").write(both)
            written.append((p, 2 * W + 5, H))
        for w in sorted(set(self.warnings)):
            print("warning:", w, file=sys.stderr)
        for p, W, H in written:
            print("wrote", os.path.basename(p))
            if png:
                pngp = os.path.splitext(p)[0] + ".png"
                subprocess.run([INKSCAPE, p, "--export-type=png", "--export-filename=" + pngp,
                                "--export-width=%d" % int(W / 25.4 * dpi), "--export-background=#ffffff"], check=True)
                print("wrote", os.path.basename(pngp))


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    dpi = 300
    if "--dpi" in argv:
        dpi = int(argv[argv.index("--dpi") + 1])
    Renderer(argv[1]).run(png="--png" in argv, dpi=dpi)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
