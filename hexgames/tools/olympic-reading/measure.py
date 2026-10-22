# Copyright Ben Paul Wise. All Rights Reserved.
"""Per-hex terrain fractions and per-hexside fill runs on the Olympic photo."""
import json, math
import numpy as np
from cells import *

im = np.asarray(Image.open(IMG).convert("RGB")).astype(int)
R, G, B = im[..., 0], im[..., 1], im[..., 2]
br = (R + G + B) / 3
water = (B - R > 40) & (B > 150)
tan = (R - B >= 20) & ~water
mountain = tan & (R - B >= 45) & (br < 135)
rough = tan & ~mountain
clear = ~water & ~tan & (br > 150) & (abs(R - B) < 25)
H, W_ = R.shape


def pts_in_hex(x, y, inset=0.85, n=900):
    rng = np.random.default_rng(1)
    u = rng.uniform(-1, 1, (n * 2, 2)) * SIZE * inset
    ok = (np.abs(u[:, 1]) <= math.sqrt(3) / 2 * SIZE * inset) & (np.abs(u[:, 0]) + np.abs(u[:, 1]) / math.sqrt(3) <= SIZE * inset)
    u = u[ok][:n]
    return np.clip((u[:, 0] + x).astype(int), 0, W_ - 1), np.clip((u[:, 1] + y).astype(int), 0, H - 1)


frac = {}
for h in CELLS:
    xs, ys = pts_in_hex(*xy(h))
    frac[h] = {k: float(m[ys, xs].mean()) for k, m in
               (("water", water), ("clear", clear), ("rough", rough), ("mountain", mountain))}

side = {}
for h in CELLS:
    for d in ("n", "ne", "se"):
        o = nb(h, d)
        if not o:
            continue
        (ax, ay), (bx, by) = side_points(h, o)
        ts = np.linspace(0.08, 0.92, 24)
        xs = np.clip((ax + (bx - ax) * ts).astype(int), 0, W_ - 1)
        ys = np.clip((ay + (by - ay) * ts).astype(int), 0, H - 1)

        # a strip 3 px either side of the hexside line, so the printed grid line itself does not count
        def run(mask):
            hits = []
            for off in (-3, 3):
                nx, ny = -(by - ay), (bx - ax)
                l = math.hypot(nx, ny)
                px = np.clip((xs + nx / l * off).astype(int), 0, W_ - 1)
                py = np.clip((ys + ny / l * off).astype(int), 0, H - 1)
                hits.append(mask[py, px])
            return float(np.mean(hits[0] & hits[1]))   # the fill crosses the side: both sides, at every point
        side["%s|%s" % (h, o)] = {"tan": run(tan), "mountain": run(mountain), "water": run(water)}
json.dump({"frac": frac, "side": side}, open("measure.json", "w"))
for k in ("water", "clear", "rough", "mountain"):
    v = np.array([f[k] for f in frac.values()])
    print(k, np.histogram(v, bins=[0, .05, .1, .2, .3, .4, .5, .6, .7, .8, .9, 1.01])[0].tolist())
# Copyright Ben Paul Wise. All Rights Reserved.
