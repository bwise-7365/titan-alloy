# Copyright Ben Paul Wise. All Rights Reserved.
"""The Olympic photo addressed by the fitted lattice: every printed cell's position on the wrinkled photo."""
import json, math
from PIL import Image, ImageDraw, ImageFont

HEX = r"C:/repos/ghub-per/titan-alloy/hexgames"
IMG = HEX + "/example maps/Olympic map wrinkled.png"
MARKED = HEX + "/example maps/Olympic map wrinkled and marked.png"
L = json.load(open(HEX + "/map_graphics/xml/tools/reader/work/Olympic_map_wrinkled/lattice.json"))
U = L["refit"]["lattice"]["u"]            # column step (flat-top: +1 column)
W = L["refit"]["lattice"]["w"]            # row step
SIZE = math.hypot(*W) / math.sqrt(3)      # hex size (centre to corner)
GROWN = L["grown"]["cells"]
PRINTED = set(L["printed"]["cells"])


def cid(c, r):
    return "%02d%02d" % (c, r - 1)       # numbering: col = c, row = r - 1 (anchors 1106, 1613, 3211)


CELLS = {}                                # id -> (c, r, x, y) on the photo
for key in PRINTED:
    c, r = map(int, key[1:].split("r"))
    x, y = GROWN[key][:2]
    CELLS[cid(c, r)] = (c, r, x, y)
BYIDX = {(v[0], v[1]): k for k, v in CELLS.items()}

# holes (no-holes invariant): cells the lattice run did not register, placed at their neighbours' centroid
import os as _os
if _os.path.exists("holes.json"):
    for _h, _c, _r in json.load(open("holes.json")):
        _tab = {"n": (0, -1), "s": (0, 1), "ne": (1, 0), "se": (1, 1), "nw": (-1, 0), "sw": (-1, 1)} if _c % 2 == 0 else                {"n": (0, -1), "s": (0, 1), "ne": (1, -1), "se": (1, 0), "nw": (-1, -1), "sw": (-1, 0)}
        _ns = [CELLS[BYIDX[(_c + dc, _r + dr)]] for dc, dr in _tab.values()]
        CELLS[_h] = (_c, _r, sum(n[2] for n in _ns) / 6, sum(n[3] for n in _ns) / 6)
        BYIDX[(_c, _r)] = _h

# snapped to the printed grid lines (snapcells.py), when that has been run
if _os.path.exists("snapped.json"):
    for _h, (_x, _y, _s) in json.load(open("snapped.json")).items():
        if _h in CELLS:
            CELLS[_h] = (CELLS[_h][0], CELLS[_h][1], _x, _y)

# flat-top; the EVEN columns sit half a hex lower (measured from the grown positions)
NB_SHIFT = {"n": (0, -1), "s": (0, 1), "ne": (1, 0), "se": (1, 1), "nw": (-1, 0), "sw": (-1, 1)}
NB_PLAIN = {"n": (0, -1), "s": (0, 1), "ne": (1, -1), "se": (1, 0), "nw": (-1, -1), "sw": (-1, 0)}


def nb(h, d):
    c, r = CELLS[h][:2]
    dc, dr = (NB_SHIFT if c % 2 == 0 else NB_PLAIN)[d]   # measured: even columns sit half a hex lower
    return BYIDX.get((c + dc, r + dr))


def xy(h):
    return CELLS[h][2], CELLS[h][3]


def side_points(a, b):
    """The shared hexside of neighbours a, b on the photo: its two corners, from the two grown centres."""
    (x0, y0), (x1, y1) = xy(a), xy(b)
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    dx, dy = x1 - x0, y1 - y0
    n = math.hypot(dx, dy)
    px, py = -dy / n, dx / n                # along the hexside
    h = SIZE / 2
    return (mx - px * h, my - py * h), (mx + px * h, my + py * h)


def overlay(x0, y0, x1, y1, scale, out, img=IMG, ids=True, extra=None):
    im = Image.open(img).convert("RGB").crop((x0, y0, x1, y1))
    im = im.resize((int((x1 - x0) * scale), int((y1 - y0) * scale)), Image.LANCZOS)
    dr = ImageDraw.Draw(im)
    font = ImageFont.truetype("arialbd.ttf", 11)
    T = lambda x, y: ((x - x0) * scale, (y - y0) * scale)
    for h, (c, r, x, y) in CELLS.items():
        if x0 - 30 < x < x1 + 30 and y0 - 30 < y < y1 + 30:
            pts = [T(x + SIZE * math.cos(math.radians(a)), y + SIZE * math.sin(math.radians(a))) for a in range(0, 360, 60)]
            dr.line(pts + [pts[0]], fill=(255, 0, 255), width=1)
            if ids:
                q = T(x, y)
                dr.text((q[0] - 13, q[1] - 6), h, fill=(170, 0, 170), font=font)
    if extra:
        extra(dr, T)
    im.save(out, quality=85)
# Copyright Ben Paul Wise. All Rights Reserved.
