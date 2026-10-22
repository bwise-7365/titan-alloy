# Copyright Ben Paul Wise. All Rights Reserved.
"""trace.py -- detect coloured features on a calibrated hex grid in a scan.

python trace.py SRC GRIDSPEC MODE name=#rrggbb:minfrac ... [bbox=x0,y0,x1,y1] [band=W]

MODE
  edges    a line along a hexside            -> <edge at="HEX:DIR" line="name"/>
  links    a line between two hex centres    -> <link kind="name" line="name" hexes="A B"/>
  sides    a blob just inside a hexside      -> <hex id><side dir symbol="fire-intense" color="name"/></hex>
  rings    a coloured inset outline           -> <hex id ring="name"/>
  centres  a coloured blob at the centre      -> <hex id><glyph symbol="name" color="name"/></hex>
  slots    a coloured blob at a compass slot  -> <hex id><glyph symbol="name" slot="s" color="name"/></hex>

Pixels in the sampled area are classified to the nearest of the named reference
colours or the background colours of the hexes concerned; a feature is reported
when a reference's fraction reaches its minfrac.
"""
import math
import sys

import numpy as np
from PIL import Image
from lxml import etree

sys.path.insert(0, r"C:\repos\ghub-per\titan-alloy\hexgames\map_graphics\xml")
import hexsheet2svg as H

Image.MAX_IMAGE_PIXELS = None
src, spec, mode = sys.argv[1], sys.argv[2], sys.argv[3]
attrs = dict(kv.split("=", 1) for kv in spec.split())
el = etree.Element("grid", terrain="t")
for k, v in attrs.items():
    el.set(k, v)
g = H.Grid(el)
im = np.asarray(Image.open(src).convert("RGB"), dtype=np.float32)
Hh, Ww = im.shape[:2]
bbox = None
band = max(2.0, g.size * 0.12)
names, refs, mins = [], [], []
for a in sys.argv[4:]:
    if a.startswith("bbox="):
        bbox = [float(v) for v in a[5:].split(",")]
    elif a.startswith("band="):
        band = float(a[5:])
    else:
        n, rest = a.split("=")
        col, mn = rest.split(":")
        names.append(n)
        refs.append([int(col[i:i + 2], 16) for i in (1, 3, 5)])
        mins.append(float(mn))
refs = np.array(refs, dtype=np.float32)
mins = np.array(mins)


def disc_pixels(x, y, rad):
    x, y = int(round(x)), int(round(y))
    if x - rad < 0 or y - rad < 0 or x + rad >= Ww or y + rad >= Hh:
        return None
    yy, xx = np.mgrid[-rad:rad + 1, -rad:rad + 1]
    return im[y - rad:y + rad + 1, x - rad:x + rad + 1][(xx ** 2 + yy ** 2) <= rad ** 2]


def sample_segment(a, b):
    """pixels in a band of half-width `band` along the middle 70% of segment a-b"""
    ax, ay = a
    bx, by = b
    L = math.hypot(bx - ax, by - ay)
    if L < 1:
        return None
    ux, uy = (bx - ax) / L, (by - ay) / L
    nx, ny = -uy, ux
    ts = np.linspace(0.15 * L, 0.85 * L, max(6, int(0.7 * L)))
    ws = np.linspace(-band, band, max(3, int(2 * band)))
    X = (ax + ts[:, None] * ux + ws[None, :] * nx).ravel()
    Y = (ay + ts[:, None] * uy + ws[None, :] * ny).ravel()
    ok = (X >= 0) & (X < Ww - 1) & (Y >= 0) & (Y < Hh - 1)
    if ok.sum() < 6:
        return None
    return im[Y[ok].astype(int), X[ok].astype(int)]


RAD = max(2, int(g.size * 0.25))
_centre_cache = {}


def centre_colour(c, r):
    if (c, r) not in _centre_cache:
        cx, cy = g.centre(c, r)
        pix = disc_pixels(cx, cy, RAD)
        _centre_cache[(c, r)] = None if pix is None else np.median(pix, axis=0)
    return _centre_cache[(c, r)]


def classify(pix, bgs):
    """nearest-colour vote against the references plus the given background colours"""
    bgs = [b for b in bgs if b is not None] or [np.median(pix, axis=0)]
    R = np.vstack([refs, np.array(bgs)])
    d = ((pix[:, None, :] - R[None, :, :]) ** 2).sum(-1)
    lab = d.argmin(1)
    frac = np.bincount(lab, minlength=len(R))[:len(refs)] / len(pix)
    k = int(frac.argmax())
    return (names[k] if frac[k] >= mins[k] else None), frac


def inside(p):
    return (not bbox) or (bbox[0] <= p[0] <= bbox[2] and bbox[1] <= p[1] <= bbox[3])


SLOTS = ("n", "ne", "e", "se", "s", "sw", "w", "nw")
out = []
for (c, r), pid in g.cells.items():
    cx, cy = g.centre(c, r)
    if not inside((cx, cy)):
        continue
    if mode in ("edges", "links"):
        for d in g.geom["edges"]:
            n = g.neighbour(c, r, d)
            if n is None or n not in g.cells or n < (c, r):   # each edge / link once
                continue
            if mode == "edges":
                a, b = g.edge_ends(c, r, d)
                pix = sample_segment(a, b)
            else:
                pix = sample_segment((cx, cy), g.centre(*n))
            if pix is None:
                continue
            name, frac = classify(pix, [centre_colour(c, r), centre_colour(*n)])
            if name and mode == "edges":
                out.append('<edge at="%s:%s" line="%s"/>' % (pid, d, name))
            elif name:
                out.append('<link kind="%s" line="%s" hexes="%s %s"/>' % (name, name, pid, g.cells[n]))
        continue
    bgc = centre_colour(c, r)
    if mode == "sides":
        found = []
        for d in g.geom["edges"]:
            mx, my = g.edge_mid(c, r, d)
            k = 0.84
            pix = disc_pixels(cx + (mx - cx) * k, cy + (my - cy) * k, max(2, int(g.size * 0.11)))
            if pix is None:
                continue
            name, frac = classify(pix, [bgc])
            if name:
                found.append('<side dir="%s" symbol="fire-intense" color="%s"/>' % (d, name))
        if found:
            out.append('<hex id="%s">%s</hex>' % (pid, "".join(found)))
    elif mode == "rings":
        pts = g.polygon(c, r, inset=0.10)
        pix = [s for s in (sample_segment(pts[i], pts[(i + 1) % 6]) for i in range(6)) if s is not None]
        if pix:
            name, frac = classify(np.vstack(pix), [bgc])
            if name:
                out.append('<hex id="%s" ring="%s"/>' % (pid, name))
    elif mode == "centres":
        pix = disc_pixels(cx, cy, max(2, int(g.size * 0.20)))
        if pix is not None:
            name, frac = classify(pix, [])
            if name:
                out.append('<hex id="%s"><glyph symbol="%s" color="%s"/></hex>' % (pid, name, name))
    elif mode == "slots":
        found = []
        for slot in SLOTS:
            sx, sy = g.slot_point(c, r, slot, 0.5)
            pix = disc_pixels(sx, sy, max(2, int(g.size * 0.12)))
            if pix is None:
                continue
            name, frac = classify(pix, [bgc])
            if name:
                found.append('<glyph symbol="%s" slot="%s" color="%s"/>' % (name, slot, name))
        if found:
            out.append('<hex id="%s">%s</hex>' % (pid, found[0]))   # one port per hex is enough
print("\n".join(out))
print("<!-- %d %s -->" % (len(out), mode))
# Copyright Ben Paul Wise. All Rights Reserved.
