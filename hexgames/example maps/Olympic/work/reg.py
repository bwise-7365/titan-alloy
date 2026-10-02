# Copyright Ben Paul Wise. All Rights Reserved.
"""Register a detail photo (quarter scale, rotated upright) to the Olympic sheet's ideal grid.
Model: homography sheet px -> photo px, fitted to landmarks, then refined by maximising the
printed grid-line strength sampled along every hexside that falls inside the photo."""
import sys, json, math
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter, map_coordinates
from scipy.optimize import minimize
import xml.etree.ElementTree as ET

HEX = "C:/repos/ghub-per/titan-alloy/hexgames"
sys.path.insert(0, HEX + "/map_graphics/xml")
import hexsheet2svg as hs

Image.MAX_IMAGE_PIXELS = None
G = hs.Grid(ET.parse(HEX + "/map_graphics/xml/operation-olympic.xml").getroot().find("grid"))
ALL = {}                                   # every lattice cell, clipped or not: id -> (c, r)
for c in range(G.cols):
    for r in range(G.rows):
        ALL[G.make_id(c, r)] = (c, r)


def centre(h):
    return G.centre(*ALL[h])


def homography(src, dst):
    A = []
    for (x, y), (u, v) in zip(src, dst):
        A.append([-x, -y, -1, 0, 0, 0, u * x, u * y, u])
        A.append([0, 0, 0, -x, -y, -1, v * x, v * y, v])
    _, _, vt = np.linalg.svd(np.array(A, float))
    return vt[-1].reshape(3, 3) / vt[-1][-1]


def apply(H, pts):
    pts = np.asarray(pts, float)
    p = np.c_[pts, np.ones(len(pts))] @ H.T
    return p[:, :2] / p[:, 2:]


def side_samples():
    """points along the middle 70% of every hexside of the lattice, in sheet px"""
    out = []
    for h, (c, r) in ALL.items():
        for d in ("n", "ne", "se"):
            a, b = G.edge_ends(c, r, d)
            for t in np.linspace(0.15, 0.85, 8):
                out.append((a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])))
    return np.array(out)


def lineness(gray, sigma):
    g = gaussian_filter(gray, 1.0)
    return gaussian_filter(np.maximum(gaussian_filter(g, 6) - g, 0), sigma)


def fit(name, land):
    im = np.asarray(Image.open(f"{name}_d4.png").convert("L"), float)
    src = [centre(h) for h, _ in land]
    dst = [(x * 4 / 1 if False else x, y) for _, (x, y) in land]
    H = homography(src, dst)
    res = apply(H, src) - np.array(dst)
    print(name, "landmark residuals px(/4):", np.round(np.hypot(*res.T)).astype(int).tolist())
    S = side_samples()
    for sigma, scale in ((4, 4), (2, 2), (1, 1)):
        small = lineness(im[::scale, ::scale], sigma)
        def score(p):
            Hp = (H.ravel()[:8] + p * np.array([1e-3, 1e-3, 1, 1e-3, 1e-3, 1, 1e-7, 1e-7])).tolist() + [1]
            q = apply(np.array(Hp).reshape(3, 3), S) / scale
            ok = (q[:, 0] > 5) & (q[:, 1] > 5) & (q[:, 0] < small.shape[1] - 5) & (q[:, 1] < small.shape[0] - 5)
            return -map_coordinates(small, [q[ok, 1], q[ok, 0]], order=1).mean()
        r = minimize(score, np.zeros(8), method="Powell", options={"xtol": 1e-3, "maxfev": 4000})
        H = np.array((H.ravel()[:8] + r.x * np.array([1e-3, 1e-3, 1, 1e-3, 1e-3, 1, 1e-7, 1e-7])).tolist() + [1]).reshape(3, 3)
        print(" scale", scale, "score", round(-r.fun, 3))
    return H


if __name__ == "__main__":
    LAND = json.load(open("landmarks.json"))
    out = {}
    for name, land in LAND.items():
        out[name] = fit(name, land).tolist()
    json.dump(out, open("H.json", "w"), indent=1)
# Copyright Ben Paul Wise. All Rights Reserved.
