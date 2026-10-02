# Copyright Ben Paul Wise. All Rights Reserved.
"""Labelled crops of the Olympic detail photos, addressed by printed hex id.

  python crop.py PHOTO COL0 COL1 ROW0 ROW1 OUT.jpg [--no-lines] [--width 1100]

PHOTO is left, middle or right. The crop covers columns COL0..COL1 and rows ROW0..ROW1 (inclusive,
printed numbers) plus half a hex of margin. The fitted lattice is drawn as thin magenta hexagons,
each hex's id in small magenta type near its top. Use --no-lines to see the print unobscured (ids only).
Also: python crop.py where HEXID  -> which photos show that hex, and its pixel position in each."""
import sys, json, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
from reg import G, ALL, apply

H = {k: np.array(v) for k, v in json.load(open("H.json")).items()}
IMGS = {}


def photo(name):
    if name not in IMGS:
        IMGS[name] = Image.open(f"{name}_d4.png").convert("RGB")
    return IMGS[name]


def to_photo(name, pts):
    return apply(H[name], pts)


def hexid(col, row):
    return "%02d%02d" % (col, row)


def poly(name, h):
    c, r = ALL[h]
    return [tuple(p) for p in to_photo(name, G.polygon(c, r))]


def crop(name, c0, c1, r0, r1, out, lines=True, width=1100, marks=()):
    ids = [hexid(c, r) for c in range(c0, c1 + 1) for r in range(r0, r1 + 1) if hexid(c, r) in ALL]
    pts = np.vstack([poly(name, h) for h in ids])
    im = photo(name)
    x0, y0 = np.maximum(pts.min(0) - 20, 0).astype(int)
    x1, y1 = np.minimum(pts.max(0) + 20, [im.width, im.height]).astype(int)
    cr = im.crop((x0, y0, x1, y1))
    sc = min(1.0, width / cr.width)
    cr = cr.resize((int(cr.width * sc), int(cr.height * sc)), Image.LANCZOS)
    dr = ImageDraw.Draw(cr, "RGBA")
    font = ImageFont.truetype("arialbd.ttf", 12)
    for h in ids:
        p = [((x - x0) * sc, (y - y0) * sc) for x, y in poly(name, h)]
        if lines:
            dr.line(p + [p[0]], fill=(255, 0, 255, 150), width=1)
        cx = sum(q[0] for q in p) / 6
        top = min(q[1] for q in p)
        dr.text((cx - 14, top + 3), h, fill=(230, 0, 230, 255), font=font)
    for m in marks:                         # hexsides HEX:DIR drawn as thick yellow segments
        h, d = m.split(":")
        a, b = to_photo(name, G.edge_ends(*ALL[h], d))
        dr.line([((a[0] - x0) * sc, (a[1] - y0) * sc), ((b[0] - x0) * sc, (b[1] - y0) * sc)], fill=(255, 230, 0, 255), width=4)
    cr.save(out, quality=88)
    print(out, cr.size)


def where(h):
    c, r = ALL[h]
    for name in H:
        x, y = to_photo(name, [G.centre(c, r)])[0]
        im = photo(name)
        if 0 <= x < im.width and 0 <= y < im.height:
            print(name, round(x), round(y))


if __name__ == "__main__":
    a = sys.argv[1:]
    if a[0] == "where":
        where(a[1])
    else:
        w = int(a[a.index("--width") + 1]) if "--width" in a else 1100
        mk = a[a.index("--mark") + 1].split(",") if "--mark" in a else ()
        crop(a[0], int(a[1]), int(a[2]), int(a[3]), int(a[4]), a[5], "--no-lines" not in a, w, mk)
# Copyright Ben Paul Wise. All Rights Reserved.
