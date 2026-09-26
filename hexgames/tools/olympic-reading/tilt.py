# Copyright Ben Paul Wise. All Rights Reserved.
"""Register the tilted photo to the wrinkled one: homography from landmarks, then patch NCC refinement."""
import json
import numpy as np
from scipy.signal import fftconvolve
from scipy.interpolate import LinearNDInterpolator, NearestNDInterpolator
from cells import *

TILT = HEX + "/example maps/olympic full map, tilted.jpg"
# landmark: (wrinkled photo px, tilted photo px), read off the two overviews
LAND = [((588, 414), (1147, 1164)), ((1494, 198), (2331, 897)), ((2178, 924), (3503, 1739)),
        ((1362, 1464), (2213, 2603)), ((1428, 756), (2331, 1512)), ((690, 1006), (1197, 1890))]


def homography(src, dst):
    A = []
    for (x, y), (u, v) in zip(src, dst):
        A.append([-x, -y, -1, 0, 0, 0, u * x, u * y, u])
        A.append([0, 0, 0, -x, -y, -1, v * x, v * y, v])
    _, _, vt = np.linalg.svd(np.array(A, float))
    return vt[-1].reshape(3, 3) / vt[-1][-1]


def apply(Hm, x, y):
    p = Hm @ np.array([x, y, 1.0])
    return p[0] / p[2], p[1] / p[2]


def norm(a):
    a = a - a.mean(); s = a.std()
    return a / s if s > 1e-6 else a


def ncc(t, s):
    t = norm(t)
    num = fftconvolve(s - s.mean(), t[::-1, ::-1], mode="valid")
    ones = np.ones_like(t)
    s2 = fftconvolve(s ** 2, ones, mode="valid"); s1 = fftconvolve(s, ones, mode="valid")
    sc = num / np.sqrt(np.maximum(s2 - s1 ** 2 / t.size, 1e-6) * t.size)
    iy, ix = np.unravel_index(np.argmax(sc), sc.shape)
    return ix, iy, sc[iy, ix]


if __name__ == "__main__":
    Hm = homography([a for a, _ in LAND], [b for _, b in LAND])
    for (a, b) in LAND:
        u, v = apply(Hm, *a)
        print("landmark residual", round(u - b[0]), round(v - b[1]))
    src = np.asarray(Image.open(IMG).convert("L")).astype(float)
    tilt = Image.open(TILT).convert("L")
    K = 1.6                                        # tilted px per wrinkled px, roughly
    sm = np.asarray(tilt.resize((int(tilt.width / K), int(tilt.height / K)), Image.LANCZOS)).astype(float)
    pts = []
    P, SR = 30, 30
    for y in range(60, src.shape[0] - 60, 40):
        for x in range(60, src.shape[1] - 60, 40):
            tm = src[y - P:y + P, x - P:x + P]
            if tm.std() < 10:
                continue
            u, v = apply(Hm, x, y)
            u, v = u / K, v / K
            x0, y0 = int(u) - P - SR, int(v) - P - SR
            if x0 < 0 or y0 < 0 or x0 + 2 * (P + SR) > sm.shape[1] or y0 + 2 * (P + SR) > sm.shape[0]:
                continue
            ix, iy, s = ncc(tm, sm[y0:y0 + 2 * (P + SR), x0:x0 + 2 * (P + SR)])
            if s > 0.6:
                pts.append([x, y, (x0 + ix + P) * K, (y0 + iy + P) * K])
    pts = np.array(pts)
    pred = np.array([apply(Hm, x, y) for x, y in pts[:, :2]])
    res = pts[:, 2:] - pred
    keep = np.hypot(*res.T) < 60
    print("control points", len(pts), "kept", keep.sum(), "median residual", np.median(np.hypot(*res[keep].T)))
    json.dump({"H": Hm.tolist(), "pts": pts[keep].tolist(), "res": res[keep].tolist()}, open("tilt.json", "w"))
# Copyright Ben Paul Wise. All Rights Reserved.
