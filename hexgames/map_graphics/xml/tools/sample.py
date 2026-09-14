# Copyright Ben Paul Wise. All Rights Reserved.
"""sample.py -- sample the scan colour at every hex centre of a calibrated grid.

python sample.py SRC GRIDSPEC clusters K                -> print K colour clusters with counts and an example hex
python sample.py SRC GRIDSPEC classify name=#rrggbb ... -> print <hexes terrain=name ids=.../> per class
                                                           (add x0,y0,x1,y1 as bbox=... to restrict to the hex field)

GRIDSPEC is a quoted string of grid attributes as in hexsheet XML, e.g.
  "orientation=pointy offset=odd size=20.65 ox=-13.5 oy=-30.45 cols=35 rows=49 id-format={rowletter}{col} col-start=35 col-step=-1 row-start=-5"
"""
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
rest = sys.argv[4:]
for a in list(rest):
    if a.startswith("bbox="):
        bbox = [float(v) for v in a[5:].split(",")]
        rest.remove(a)

rad = max(2, int(g.size * (0.30 if mode == "clusters" else 0.62)))
yy, xx = np.mgrid[-rad:rad + 1, -rad:rad + 1]
disc = (xx ** 2 + yy ** 2) <= rad ** 2

samples = {}
for (c, r), pid in g.cells.items():
    cx, cy = g.centre(c, r)
    if bbox and not (bbox[0] <= cx <= bbox[2] and bbox[1] <= cy <= bbox[3]):
        continue
    x, y = int(round(cx)), int(round(cy))
    if x - rad < 0 or y - rad < 0 or x + rad >= Ww or y + rad >= Hh:
        continue
    patch = im[y - rad:y + rad + 1, x - rad:x + rad + 1][disc]
    samples[pid] = np.median(patch, axis=0)

ids = list(samples)
X = np.array([samples[i] for i in ids])

if mode == "clusters":
    K = int(rest[0])
    rng = np.random.default_rng(1)
    cent = X[rng.choice(len(X), K, replace=False)]
    for _ in range(40):
        d = ((X[:, None, :] - cent[None, :, :]) ** 2).sum(-1)
        lab = d.argmin(1)
        for k in range(K):
            if (lab == k).any():
                cent[k] = X[lab == k].mean(0)
    order = np.argsort(-np.bincount(lab, minlength=K))
    for k in order:
        n = int((lab == k).sum())
        if n == 0:
            continue
        ex = [ids[i] for i in np.where(lab == k)[0][:6]]
        print("#%02x%02x%02x  n=%4d  e.g. %s" % (tuple(int(v) for v in cent[k]) + (n, " ".join(ex))))
else:
    names, refs = [], []
    for a in rest:
        n, col = a.split("=")
        names.append(n)
        refs.append([int(col[i:i + 2], 16) for i in (1, 3, 5)])
    refs = np.array(refs, dtype=np.float32)
    assigned = {}
    for (c, r), pid in g.cells.items():
        cx, cy = g.centre(c, r)
        if bbox and not (bbox[0] <= cx <= bbox[2] and bbox[1] <= cy <= bbox[3]):
            continue
        x, y = int(round(cx)), int(round(cy))
        if x - rad < 0 or y - rad < 0 or x + rad >= Ww or y + rad >= Hh:
            continue
        patch = im[y - rad:y + rad + 1, x - rad:x + rad + 1][disc]
        d = ((patch[:, None, :] - refs[None, :, :]) ** 2).sum(-1)
        votes = np.bincount(d.argmin(1), minlength=len(names))
        assigned[pid] = int(votes.argmax())
    ids = list(assigned)
    for k, n in enumerate(names):
        members = [i for i in ids if assigned[i] == k]
        if members:
            print('<hexes terrain="%s" ids="%s"/>' % (n, " ".join(members)))
    print("<!-- %d hexes classified -->" % len(ids))
# Copyright Ben Paul Wise. All Rights Reserved.
