# Copyright Ben Paul Wise. All Rights Reserved.
"""vectorize_logo.py -- the simplified Circling Dragons logo as SVG.

Source: the right-hand (simplified) panel of Circling Dragons/file_0000000093a481f58355b3278556e667.png,
cropped to Circling Dragons/circling-dragons-logo.png (930 x 796, the caption below the panel removed).

Stage A (quantize): classify every pixel into a colour family by hue (red dragon and its text, blue dragon
and its text, neutrals: background, white, greys, black), then k-means each family's lightness into a few
shades: 3 for each dragon plus the white highlights; six neutrals for the background, the hills, buildings,
bridge, China and the text.  Writes the palette and a posterized preview into Circling Dragons/work/.
Stage B (trace): one potrace layer per palette colour.  Stacked (the default): each layer's mask includes
every layer painted after it, so the traced boundaries never leave hairline gaps; dark on top.  Flat
(--flat): each colour traced once from its own mask, a thin same-colour stroke closing the seams, about
half the size.  Writes Circling Dragons/circling-dragons-logo.svg (or -flat.svg).

    python vectorize_logo.py quantize [red_shades blue_shades grey_levels]      default 3 3 6
    python vectorize_logo.py trace [--flat]

Needs potracer (pip install potracer; imported as potrace), numpy, scipy, Pillow.  potracer takes False as
the foreground, so the masks are inverted before tracing (a 5 x 5 True square traced as its complement).
"""
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy.cluster.vq import kmeans2

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
LOGO_DIR = os.path.join(REPO, "Circling Dragons")
WORK = os.path.join(LOGO_DIR, "work")
SRC = os.path.join(LOGO_DIR, "circling-dragons-logo.png")
LABELS = os.path.join(WORK, "logo_labels.npy")
PALETTE = os.path.join(WORK, "logo_palette.json")
PREVIEW = os.path.join(WORK, "logo_posterized.png")
SVG = os.path.join(LOGO_DIR, "circling-dragons-logo.svg")


def rgb_to_hsv(a):
    """a: float array (..., 3) in 0..1 -> hue degrees, saturation, value."""
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    mx = a.max(axis=-1)
    mn = a.min(axis=-1)
    d = mx - mn
    h = np.zeros_like(mx)
    nz = d > 1e-6
    rmax = nz & (mx == r)
    gmax = nz & (mx == g) & ~rmax
    bmax = nz & ~rmax & ~gmax
    safe = np.where(d == 0, 1, d)
    h[rmax] = (60 * ((g - b) / safe))[rmax] % 360
    h[gmax] = (60 * ((b - r) / safe) + 120)[gmax]
    h[bmax] = (60 * ((r - g) / safe) + 240)[bmax]
    s = np.where(mx > 1e-6, d / np.where(mx == 0, 1, mx), 0)
    return h, s, mx


def quantize(red_k=3, blue_k=3, grey_k=6):
    os.makedirs(WORK, exist_ok=True)
    im = np.asarray(Image.open(SRC).convert("RGB")).astype(np.float64) / 255.0
    h, s, v = rgb_to_hsv(im)
    red = (s > 0.18) & ((h >= 335) | (h <= 25))
    blue = (s > 0.14) & (h >= 185) & (h <= 265)
    neutral = ~(red | blue)
    labels = np.full(im.shape[:2], -1, dtype=np.int32)
    palette = []

    def cluster(mask, k, family):
        pts = im[mask]
        lum = (0.299 * pts[:, 0] + 0.587 * pts[:, 1] + 0.114 * pts[:, 2]).reshape(-1, 1)
        # seeds spread over the family's lightness range, so every run gives the same clusters
        seeds = np.linspace(lum.min(), lum.max(), k).reshape(-1, 1)
        centroids, lab = kmeans2(lum, seeds, minit="matrix")
        order = np.argsort(centroids[:, 0])        # light to dark within the family
        remap = np.empty(k, dtype=int)
        for rank, ci in enumerate(order):
            remap[ci] = rank
        lab = remap[lab]
        base = len(palette)
        for rank in range(k):
            sel = pts[lab == rank]
            mean = sel.mean(axis=0) if len(sel) else np.array([0.5, 0.5, 0.5])
            palette.append(dict(family=family, rgb=[int(round(c * 255)) for c in mean], count=int(len(sel))))
        labels[mask] = lab + base
        return

    cluster(neutral, grey_k, "neutral")
    cluster(red, red_k, "red")
    cluster(blue, blue_k, "blue")
    # the background is the most common neutral colour
    counts = [p["count"] if p["family"] == "neutral" else 0 for p in palette]
    bg = int(np.argmax(counts))
    for i, p in enumerate(palette):
        p["id"] = "c%d" % i
        p["hex"] = "#%02x%02x%02x" % tuple(p["rgb"])
        p["background"] = (i == bg)
    np.save(LABELS, labels)
    json.dump(palette, open(PALETTE, "w"), indent=1)
    out = np.zeros_like(im)
    for i, p in enumerate(palette):
        out[labels == i] = np.array(p["rgb"]) / 255.0
    Image.fromarray((out * 255).round().astype(np.uint8)).save(PREVIEW)
    for p in palette:
        print("%-3s %-8s %s  %7d px%s" % (p["id"], p["family"], p["hex"], p["count"], "  (background)" if p["background"] else ""))
    print("wrote", PREVIEW)
    return


def path_data(path):
    """potrace path -> SVG d string, every curve closed."""
    parts = []
    for curve in path:
        sp = curve.start_point
        d = ["M%.2f %.2f" % (sp.x, sp.y)]
        for seg in curve.segments:
            if seg.is_corner:
                d.append("L%.2f %.2f L%.2f %.2f" % (seg.c.x, seg.c.y, seg.end_point.x, seg.end_point.y))
            else:
                d.append("C%.2f %.2f %.2f %.2f %.2f %.2f" % (seg.c1.x, seg.c1.y, seg.c2.x, seg.c2.y,
                                                             seg.end_point.x, seg.end_point.y))
        d.append("Z")
        parts.append(" ".join(d))
    return " ".join(parts)


def trace(flat=False):
    import potrace
    labels = np.load(LABELS)
    palette = json.load(open(PALETTE))
    H, W = labels.shape
    bg = next(p for p in palette if p["background"])
    # draw order: the background rect, then colours light to dark (dark on top)
    lum = {p["id"]: 0.299 * p["rgb"][0] + 0.587 * p["rgb"][1] + 0.114 * p["rgb"][2] for p in palette}
    order = sorted((p for p in palette if not p["background"]), key=lambda p: -lum[p["id"]])
    out = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">' % (W, H, W, H),
           '  <title>Circling Dragons</title>',
           '  <desc>China 1944-1945. Defeat Japan. Control China. Traced from the simplified logo; %d colours.</desc>' % len(palette),
           '  <rect width="%d" height="%d" fill="%s"/>' % (W, H, bg["hex"])]
    ids = [int(p["id"][1:]) for p in order]
    for n, p in enumerate(order):
        mask = (labels == ids[n]) if flat else np.isin(labels, ids[n:])   # stacked: this colour and all above it
        bm = potrace.Bitmap(~mask)                                        # potracer: False is the foreground
        path = bm.trace(turdsize=3, turnpolicy=potrace.POTRACE_TURNPOLICY_MINORITY, alphamax=1.0,
                        opticurve=True, opttolerance=0.2)
        d = path_data(path)
        if d:
            stroke = ' stroke="%s" stroke-width="0.5" stroke-linejoin="round"' % p["hex"] if flat else ""
            out.append('  <path id="%s" fill="%s" fill-rule="evenodd"%s d="%s"/>' % (p["family"] + "-" + p["id"], p["hex"], stroke, d))
        print("traced", p["id"], p["family"], p["hex"], "curves", len(list(path)))
    out.append('</svg>')
    target = SVG.replace(".svg", "-flat.svg") if flat else SVG
    open(target, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print("wrote", target, os.path.getsize(target), "bytes")
    return


if __name__ == "__main__":
    if sys.argv[1] == "quantize":
        ks = [int(x) for x in sys.argv[2:5]] or [3, 3, 6]
        quantize(*ks)
    else:
        trace(flat="--flat" in sys.argv)
# Copyright Ben Paul Wise. All Rights Reserved.
