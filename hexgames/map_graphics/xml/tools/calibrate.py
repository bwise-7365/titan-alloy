"""calibrate.py SRC x0 y0 x1 y1 smin smax OUT.png -- find a flat-topped hex grid in a scan crop.

Scores every (size, ox, oy) by cross-correlating an edge map with one hex outline
and folding by the lattice period.  Prints size (circumradius), ox, oy in SCAN
pixel coordinates for the hex with column/row index (0,0), offset="odd" (odd
columns shifted down half a hex), chosen so the grid covers the whole scan.
"""
import math
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

Image.MAX_IMAGE_PIXELS = None
SQ3 = math.sqrt(3)

src = sys.argv[1]
x0, y0, x1, y1 = map(int, sys.argv[2:6])
smin, smax = float(sys.argv[6]), float(sys.argv[7])
out = sys.argv[8]
ORIENT = sys.argv[9] if len(sys.argv) > 9 else "pointy"

im = Image.open(src).convert("L")
crop = im.crop((x0, y0, x1, y1))
g = np.asarray(crop, dtype=np.float32)
blur = np.asarray(crop.filter(ImageFilter.GaussianBlur(4)), dtype=np.float32)
E = np.clip(np.abs(g - blur) - 4, 0, 40)
H, W = E.shape
ANG = (0, 60, 120, 180, 240, 300) if ORIENT == "flat" else (30, 90, 150, 210, 270, 330)
corners = [(math.cos(math.radians(a)), math.sin(math.radians(a))) for a in ANG]
unit = []
for i in range(6):
    ax, ay = corners[i]
    bx, by = corners[(i + 1) % 6]
    for t in np.linspace(0.06, 0.94, 12):
        unit.append((ax + (bx - ax) * t, ay + (by - ay) * t))
unit = np.array(unit)
PAD = int(smax) + 2
Ep = np.pad(E, PAD)


def score_map(s):
    """corr[y,x] = mean of E along a hex outline centred at (x,y); then fold by the lattice."""
    corr = np.zeros_like(E)
    for dx, dy in unit * s:
        ix, iy = int(round(dx)), int(round(dy))
        corr += Ep[PAD + iy:PAD + iy + H, PAD + ix:PAD + ix + W]
    corr /= len(unit)
    if ORIENT == "flat":
        px, py, kx, ky = 3 * s, SQ3 * s, 1.5 * s, SQ3 / 2 * s
    else:
        px, py, kx, ky = SQ3 * s, 3 * s, SQ3 / 2 * s, 1.5 * s
    nx, ny = int(px), int(py)
    fold = np.zeros((ny, nx))
    cnt = np.zeros((ny, nx))
    for i in range(int(W / px) + 1):
        for j in range(int(H / py) + 1):
            for k, dy in ((0, 0.0), (kx, ky)):
                xs = int(round(i * px + k))
                ys = int(round(j * py + dy))
                sub = corr[ys:ys + ny, xs:xs + nx]
                fold[:sub.shape[0], :sub.shape[1]] += sub
                cnt[:sub.shape[0], :sub.shape[1]] += 1
    fold /= np.maximum(cnt, 1)
    return fold


results = []
for s in np.arange(smin, smax, 0.2):
    f = score_map(s)
    j, i = np.unravel_index(np.argmax(f), f.shape)
    results.append((f[j, i], s, i, j))
results.sort(reverse=True)
print("top candidates (score, size, ox, oy):")
for r in results[:6]:
    print("  %.3f  s=%.1f  ox=%d oy=%d" % r)
_, s0, ox0, oy0 = results[0]
best = results[0]
for s in np.arange(s0 - 0.3, s0 + 0.31, 0.05):
    f = score_map(s)
    j, i = np.unravel_index(np.argmax(f), f.shape)
    if f[j, i] > best[0]:
        best = (f[j, i], s, i, j)
v, s, ox, oy = best
print("fine: score %.3f size %.2f ox %d oy %d (crop coords)" % best)

gx, gy = ox + x0, oy + y0
if ORIENT == "flat":
    c_shift = int(math.ceil(gx / (1.5 * s))); gx -= c_shift * 1.5 * s
    if c_shift % 2 == 1: gy -= SQ3 / 2 * s
    r_shift = int(math.ceil(gy / (SQ3 * s))); gy -= r_shift * SQ3 * s
    ncols, nrows = int((im.width - gx) / (1.5 * s)) + 1, int((im.height - gy) / (SQ3 * s)) + 1
else:
    r_shift = int(math.ceil(gy / (1.5 * s))); gy -= r_shift * 1.5 * s
    if r_shift % 2 == 1: gx -= SQ3 / 2 * s
    c_shift = int(math.ceil(gx / (SQ3 * s))); gx -= c_shift * SQ3 * s
    ncols, nrows = int((im.width - gx) / (SQ3 * s)) + 1, int((im.height - gy) / (1.5 * s)) + 1
print("SCAN grid: orientation=%s offset=odd size=%.2f ox=%.2f oy=%.2f cols=%d rows=%d (scan %dx%d)" % (ORIENT, s, gx, gy, ncols, nrows, im.width, im.height))

ov = Image.open(src).convert("RGB").crop((x0, y0, x1, y1))
d = ImageDraw.Draw(ov)
if ORIENT == "flat":
    cells = [(ox + c * 1.5 * s, oy + r * SQ3 * s + (SQ3 / 2 * s if c % 2 else 0)) for c in range(-1, int(W / (1.5 * s)) + 2) for r in range(-1, int(H / (SQ3 * s)) + 2)]
else:
    cells = [(ox + c * SQ3 * s + (SQ3 / 2 * s if r % 2 else 0), oy + r * 1.5 * s) for c in range(-1, int(W / (SQ3 * s)) + 2) for r in range(-1, int(H / (1.5 * s)) + 2)]
for cx, cy in cells:
    d.polygon([(cx + s * px_, cy + s * py_) for px_, py_ in corners], outline=(255, 0, 255))
ov.save(out)
print("overlay", out)
