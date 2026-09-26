# Copyright Ben Paul Wise. All Rights Reserved.
import json, math
import numpy as np
from scipy.interpolate import LinearNDInterpolator, NearestNDInterpolator
from cells import *
from tilt import apply, TILT
_T = json.load(open("tilt.json"))
_H = np.array(_T["H"])
_P = np.array(_T["pts"]); _R = np.array(_T["res"])
_lin = LinearNDInterpolator(_P[:, :2], _R); _near = NearestNDInterpolator(_P[:, :2], _R)


def to_tilt(x, y):
    u, v = apply(_H, x, y)
    d = _lin(x, y)
    if np.any(np.isnan(d)):
        d = _near(x, y)
    d = np.ravel(d)
    return u + d[0], v + d[1]


def tilt_overlay(x0, y0, x1, y1, out, width=1100, ids=True, only=None):
    """a crop of the tilted photo around a wrinkled-photo rectangle, cells outlined and labelled"""
    corners = [to_tilt(x, y) for x, y in ((x0, y0), (x1, y0), (x0, y1), (x1, y1))]
    bx0 = int(min(c[0] for c in corners)); by0 = int(min(c[1] for c in corners))
    bx1 = int(max(c[0] for c in corners)); by1 = int(max(c[1] for c in corners))
    im = Image.open(TILT).convert("RGB").crop((bx0, by0, bx1, by1))
    sc = width / im.width
    im = im.resize((width, int(im.height * sc)), Image.LANCZOS)
    dr = ImageDraw.Draw(im); font = ImageFont.truetype("arialbd.ttf", 13)
    for h in (only or CELLS):
        x, y = xy(h)
        if not (x0 - 20 < x < x1 + 20 and y0 - 20 < y < y1 + 20):
            continue
        poly = [to_tilt(x + SIZE * math.cos(math.radians(a)), y + SIZE * math.sin(math.radians(a))) for a in range(0, 360, 60)]
        pts = [((u - bx0) * sc, (v - by0) * sc) for u, v in poly]
        dr.line(pts + [pts[0]], fill=(255, 0, 255), width=1)
        if ids:
            u, v = to_tilt(x, y)
            dr.text(((u - bx0) * sc - 14, (v - by0) * sc - 7), h, fill=(200, 0, 200), font=font)
    im.save(out, quality=85)
# Copyright Ben Paul Wise. All Rights Reserved.
