# Copyright Ben Paul Wise. All Rights Reserved.
"""legend.py -- find a scanned map's legend panel, cut its swatches, and turn one naming look into the
vocabulary (reader tool B2).

    python legend.py DIR [--panel X0 Y0 X1 Y1] [--sizes 0.5,0.7,1.0] [--min 3] [--coverage 0.9] [--margin 0.12] [--rotate DEG]
    python legend.py DIR --finish                                            naming.json -> vocabulary.json

DIR is a map's work folder holding lattice.json (from lattice.py). The first form writes
  legend/panel-N.jpg        each panel found, with its swatches numbered
  legend/swatch-NN.png      each swatch, cut square round its centre at 2.4 times its size
  legend/contact.jpg        every swatch with its label neighbourhood, numbered, for the one look
  legend/naming.json        one row per swatch: measured features and empty name/kind/sheet fields
and a report line. --panel X0 Y0 X1 Y1 (source pixels) is a person's rectangle round the legend
when the finder fails or finds the wrong thing; inside it the gate relaxes. A person or a model fills naming.json (name as printed, kind, sheet id; or
"skip" for a swatch that is not a legend entry) and --finish checks it and writes vocabulary.json in
the README's schema. Nothing here decides a meaning: the measurements propose a kind, the namer rules.

Method. The printed cells' hexes (lattice.json) are masked out; what remains is furniture. In the
furniture a legend swatch is a hex outline drawn at some fraction of the map's hex size, so the score
"ink on a hex outline minus ink just inside it" is evaluated on a stride over the furniture at several
sizes and both orientations (a chart printed sideways turns pointy swatches flat). Peaks above a
threshold set from the map's own printed hexes are swatch candidates; candidates within a few swatch
sizes of each other form a panel; panels of fewer than --min swatches are dropped. Per swatch the
interior colour, which edges carry ink (a hexside kind marks one edge), and whether a line crosses the
hex (a link kind) are measured and written as features. A map with no panel gets an empty naming.json
and the report says so: that is the style-library case (G2), handled at --finish by rows with "from".
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFile, ImageFont
from scipy import ndimage

ImageFile.LOAD_TRUNCATED_IMAGES = True

import lattice as L

KINDS = ("hex", "ring", "glyph", "side-along", "side-across", "side-mid", "vertex", "region")


def option(argv, key, default=None):
    return argv[argv.index(key) + 1] if key in argv else default


# ---------------------------------------------------------------- furniture mask


def furniture_mask(d, shape):
    """True outside the map body. The printed cells' polygons (grown positions, slightly enlarged) are
    painted, the body is closed by a hex and a half and its holes filled, so that sea and other cells
    the printed test missed inside the map do not count as furniture; what remains is furniture."""
    h, w = shape
    im = Image.new("L", (w, h), 0)
    dr = ImageDraw.Draw(im)
    size = d["size"]
    corners = L.hex_outline_points(size * 1.15, 0.0, d["orientation"], n=1)
    printed = set(d["printed"]["cells"])
    for key, (x, y, _) in d["grown"]["cells"].items():
        if key in printed:
            dr.polygon([(x + px, y + py) for px, py in corners], fill=255)
    k = 4
    small = np.asarray(im)[::k, ::k] > 0
    radius = max(1, int(1.5 * size / k))
    yy, xx = np.mgrid[-radius:radius + 1, -radius:radius + 1]
    disc = xx * xx + yy * yy <= radius * radius
    body = ndimage.binary_closing(small, structure=disc, border_value=0)
    body = ndimage.binary_fill_holes(body)
    body = ndimage.binary_dilation(body, structure=disc)
    full = np.repeat(np.repeat(body, k, axis=0), k, axis=1)[:h, :w]
    if full.shape != (h, w):
        pad = np.zeros((h, w), dtype=bool)
        pad[:full.shape[0], :full.shape[1]] = full
        full = pad
    return ~full


# ---------------------------------------------------------------- swatch candidates


def outline_score_map(e, mask, size, rot, orientation, stride, tolerance, gap, margin, ring_gate=0.3):
    """LINE COVERAGE per edge, by contrast: at each of ten points along an edge the energy ON the
    outline (the best of +-tolerance px along the normal) must exceed the energy OFF it (the mean at
    +-gap px along the normal) by margin, and exceed margin itself. The score is the weakest edge's
    coverage, or zero where a ring just OUTSIDE the hex is covered too, which is a block of text. A
    drawn swatch has a line on every edge with clean paper either side of
    it; paper texture and halftone have no contrast across a line; a table rule or a box crosses a
    slanted hex edge at one point. Evaluated at every stride-th pixel inside mask."""
    h, w = e.shape
    n = 10
    start = 30.0 if "pointy" == orientation else 0.0
    ys, xs = np.mgrid[0:h:stride, 0:w:stride]
    ok = mask[ys, xs]
    ys, xs = ys[ok], xs[ok]
    if 0 == len(xs):
        return xs, ys, np.zeros(0)

    def sample(px, py):
        ix = np.clip(np.rint(xs + px).astype(int), 0, w - 1)
        iy = np.clip(np.rint(ys + py).astype(int), 0, h - 1)
        return e[iy, ix]

    def coverage(scale, per_edge):
        pts = L.hex_outline_points(scale * size, rot, orientation, n=per_edge)
        out = []
        for k in range(6):
            ang = math.radians(start + 60 * k + 30 + rot)
            nx, ny = math.cos(ang), math.sin(ang)
            acc = np.zeros(len(xs), dtype=np.float32)
            for px, py in pts[per_edge * k:per_edge * (k + 1)]:
                on = np.zeros(len(xs), dtype=np.float32)
                for t in range(-tolerance, tolerance + 1):
                    on = np.maximum(on, sample(px + t * nx, py + t * ny))
                off = 0.5 * (sample(px + gap * nx, py + gap * ny) + sample(px - gap * nx, py - gap * ny))
                acc += (on > off + margin) & (on > margin)
            out.append(acc / per_edge)
        return out

    edges = coverage(1.0, n)
    weakest = np.minimum.reduce(edges)
    ring = np.mean(coverage(1.25, 4), axis=0)
    # measured on Tannenberg: its Terrain Key swatches have weakest-edge coverage 1.0 and ring 0.1;
    # text has 0.2-0.7 and 0.4-0.8. The gate is coverage >= 0.9 (caller) and ring <= 0.3 (here).
    return xs, ys, np.where(ring > ring_gate, 0.0, weakest)


def printed_reference(e, d):
    """The median edge energy on the map's own printed hex outlines: what a drawn line scores on this
    scan. Half of it is the level a point must exceed to count as ink."""
    size = d["size"]
    outer = np.array(L.hex_outline_points(size, 0.0, d["orientation"], n=6))
    h, w = e.shape
    scores = []
    for key in d["printed"]["cells"][::7]:
        x, y, _ = d["grown"]["cells"][key]
        ix = np.clip(np.rint(x + outer[:, 0]).astype(int), 0, w - 1)
        iy = np.clip(np.rint(y + outer[:, 1]).astype(int), 0, h - 1)
        scores.append(float(np.median(e[iy, ix])))
    return float(np.median(scores)) if scores else 0.0


def refine(e, x, y, size, rot, orientation, reach):
    """The best outline match within +-reach px of (x, y), returned with its score."""
    pts = np.array(L.hex_outline_points(size, rot, orientation, n=12))
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    return L.local_match(e, x, y, pts, offs, dx, dy)


def candidates(e, mask, d, fractions, threshold, margin, tolerance=2, ring_gate=0.3):
    """Swatch candidates (x, y, size, orientation, score) over the furniture, both orientations, each
    size, non-maximum suppressed within a swatch size. threshold is the least line coverage of the
    weakest edge; margin the contrast a line must show over the paper beside it."""
    found = []
    for frac in fractions:
        size = frac * d["size"]
        stride = max(2, int(0.12 * size))
        gap = max(4, int(0.1 * size))
        for orientation in ("pointy", "flat"):
            rot = 0.0
            xs, ys, sc = outline_score_map(e, mask, size, rot, orientation, stride, tolerance, gap, margin, ring_gate)
            keep = sc >= threshold
            for x, y, s in zip(xs[keep], ys[keep], sc[keep]):
                bx, by, bs = refine(e, float(x), float(y), size, rot, orientation, reach=stride)
                found.append((bx, by, size, orientation, float(bs)))
    # a larger hex suppresses any smaller candidate inside it (a symbol drawn in a swatch also scores as
    # a small hex); among equals the better score wins
    found.sort(key=lambda t: (-t[2], -t[4]))
    kept = []
    for c in found:
        if all(math.hypot(c[0] - k[0], c[1] - k[1]) > 0.9 * max(c[2], k[2]) for k in kept):
            kept.append(c)
    return kept


def edge_coverage_at(e, x, y, size, orientation, tolerance, margin):
    """The six edges' line coverage at one position (the per-position twin of outline_score_map)."""
    h, w = e.shape
    n = 10
    start = 30.0 if "pointy" == orientation else 0.0
    gap = max(4, int(0.1 * size))
    pts = L.hex_outline_points(size, 0.0, orientation, n=n)

    def sample(px, py):
        return e[min(h - 1, max(0, int(round(y + py)))), min(w - 1, max(0, int(round(x + px))))]

    out = []
    for k in range(6):
        ang = math.radians(start + 60 * k + 30)
        nx, ny = math.cos(ang), math.sin(ang)
        hits = 0
        for px, py in pts[n * k:n * (k + 1)]:
            on = max(sample(px + t * nx, py + t * ny) for t in range(-tolerance, tolerance + 1))
            off = 0.5 * (sample(px + gap * nx, py + gap * ny) + sample(px - gap * nx, py - gap * ny))
            hits += (on > off + margin) and (on > margin)
        out.append(hits / n)
    return out


# ---------------------------------------------------------------- panels


def panels_of(cands, min_count):
    """Clusters of candidates within three swatch sizes of a neighbour, each cut to its modal swatch size;
    only clusters of at least min_count swatches are legend panels. Each panel's swatches are ordered
    top-to-bottom, left-to-right."""
    n = len(cands)
    parent = list(range(n))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i in range(n):
        for j in range(i + 1, n):
            if math.hypot(cands[i][0] - cands[j][0], cands[i][1] - cands[j][1]) < 3.0 * max(cands[i][2], cands[j][2]):
                parent[find(i)] = find(j)
    groups = {}
    for i in range(n):
        groups.setdefault(find(i), []).append(cands[i])
    panels = []
    for g in groups.values():
        # a legend draws its swatches at one size: keep the modal size (within a fifth) and its members
        sizes = sorted(c[2] for c in g)
        best = max(sizes, key=lambda s0: sum(1 for t in sizes if abs(t - s0) <= 0.2 * s0))
        g = [c for c in g if abs(c[2] - best) <= 0.2 * best]
        if len(g) >= min_count:
            panels.append(g)
    for g in panels:
        row = np.median([c[2] for c in g]) * 1.2
        g.sort(key=lambda c: (round(c[1] / row), c[0]))
    panels.sort(key=lambda g: (-len(g), g[0][1], g[0][0]))
    return panels


def panel_steps(g):
    """The panel's row and column steps: the two most common pairwise displacements between its
    swatches (quantised to a tenth of the swatch size), at least 1.2 sizes long and not collinear."""
    size = float(np.median([c[2] for c in g]))
    q = max(2.0, 0.1 * size)
    counts = {}
    for i in range(len(g)):
        for j in range(len(g)):
            if i == j:
                continue
            dx, dy = g[j][0] - g[i][0], g[j][1] - g[i][1]
            if math.hypot(dx, dy) < 1.2 * size or math.hypot(dx, dy) > 8 * size:
                continue
            key = (int(round(dx / q)), int(round(dy / q)))
            counts[key] = counts.get(key, 0) + 1
    steps = []
    for key, cnt in sorted(counts.items(), key=lambda t: -t[1]):
        v = (key[0] * q, key[1] * q)
        if cnt < 2:
            break
        if any(abs(v[0] * u[1] - v[1] * u[0]) < 0.35 * math.hypot(*v) * math.hypot(*u) for u in steps):
            continue          # collinear with a step already taken (or its multiple)
        steps.append(v)
        if 2 == len(steps):
            break
    return steps


def grow_panel(e, g, limits, tolerance, margin, gate=0.35):
    """Ben's propagation applied to a legend: from the swatches found, predict the positions one step
    along the panel's row and column vectors, refine each on the ink and accept it when its weakest
    edge still shows a line (a relaxed gate, since the position was predicted, not searched). Bounded
    by limits (x0, y0, x1, y1), the panel's own box widened by a step. Returns the grown panel and the
    number added."""
    steps = panel_steps(g)
    if not steps:
        return g, 0
    size = float(np.median([c[2] for c in g]))
    orientation = max(set(c[3] for c in g), key=[c[3] for c in g].count)
    pts = np.array(L.hex_outline_points(size, 0.0, orientation, n=12))
    reach = max(3, int(0.15 * size))
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    members = list(g)
    added = 0
    frontier = list(members)
    while frontier:
        base = frontier.pop()
        for v in steps:
            for sign in (1, -1):
                px, py = base[0] + sign * v[0], base[1] + sign * v[1]
                if not (limits[0] <= px <= limits[2] and limits[1] <= py <= limits[3]):
                    continue
                if any(math.hypot(px - m[0], py - m[1]) < 0.9 * size for m in members):
                    continue
                bx, by, sc = L.local_match(e, px, py, pts, offs, dx, dy)
                cov = edge_coverage_at(e, bx, by, size, orientation, tolerance, margin)
                if min(cov) >= gate:
                    c = (bx, by, size, orientation, float(sc))
                    members.append(c)
                    frontier.append(c)
                    added += 1
    row = size * 1.2
    members.sort(key=lambda c: (round(c[1] / row), c[0]))
    return members, added


# ---------------------------------------------------------------- swatch features


def features(rgb, grey, e, c):
    """Interior colour, per-edge ink relative to the outline's mean, and the ink along the six
    half-diameters (a line through the hex)."""
    x, y, size, orientation, score = c
    rot = 0.0
    h, w = grey.shape
    r = 0.45 * size
    ys, xs = np.mgrid[int(y - r):int(y + r) + 1, int(x - r):int(x + r) + 1]
    inside = (xs - x) ** 2 + (ys - y) ** 2 <= r * r
    ys, xs = np.clip(ys[inside], 0, h - 1), np.clip(xs[inside], 0, w - 1)
    colour = np.median(rgb[ys, xs], axis=0)
    edges = []
    for k in range(6):
        pts = L.hex_outline_points(size, rot, orientation, n=8)
        seg = pts[k * 8:(k + 1) * 8]
        ix = np.clip(np.rint([x + px for px, _ in seg]).astype(int), 0, w - 1)
        iy = np.clip(np.rint([y + py for _, py in seg]).astype(int), 0, h - 1)
        edges.append(float(e[iy, ix].mean()))
    mean_edge = float(np.mean(edges)) or 1e-6
    spokes = []
    for k in range(6):
        ang = math.radians(rot + 60 * k + (30 if "pointy" == orientation else 0))
        ts = np.linspace(0.15, 0.85, 12) * size
        ix = np.clip(np.rint(x + ts * math.cos(ang)).astype(int), 0, w - 1)
        iy = np.clip(np.rint(y + ts * math.sin(ang)).astype(int), 0, h - 1)
        spokes.append(float(1.0 - grey[iy, ix].mean()))
    interior_ink = float(1.0 - grey[ys, xs].mean())
    marked = [i for i, v in enumerate(edges) if v > 1.8 * mean_edge]
    guess = "hex"
    if 1 == len(marked):
        guess = "side-along"
    elif max(spokes) > interior_ink + 0.25:
        guess = "side-across"
    return {
        "colour": "#%02x%02x%02x" % tuple(int(v) for v in colour),
        "edge_ink": [round(v / mean_edge, 2) for v in edges],
        "spoke_ink": [round(v, 2) for v in spokes],
        "interior_ink": round(interior_ink, 2),
        "kind_guess": guess,
    }


# ---------------------------------------------------------------- outputs


def font(size):
    try:
        return ImageFont.truetype("arial.ttf", size)
    except OSError:
        return ImageFont.load_default()


def write_outputs(rgb_im, d, panels, out, rotate):
    os.makedirs(out, exist_ok=True)
    rgb = np.asarray(rgb_im)
    grey = np.asarray(rgb_im.convert("L"), dtype=np.float32) / 255.0
    e = L.gradient(rgb_im)
    rows = []
    n = 0
    for pi, g in enumerate(panels, 1):
        xs = [c[0] for c in g]
        ys = [c[1] for c in g]
        m = 2.5 * max(c[2] for c in g)
        box = (int(max(0, min(xs) - m)), int(max(0, min(ys) - m)),
               int(min(rgb_im.width, max(xs) + m)), int(min(rgb_im.height, max(ys) + m)))
        panel = rgb_im.crop(box).copy()
        dr = ImageDraw.Draw(panel)
        f = font(max(12, int(0.5 * g[0][2])))
        for c in g:
            n += 1
            x, y, size, orientation, score = c
            px, py = x - box[0], y - box[1]
            dr.ellipse((px - size, py - size, px + size, py + size), outline=(235, 0, 170), width=2)
            dr.text((px - size, py - size - f.size), "%02d" % n, fill=(235, 0, 170), font=f)
            half = 1.2 * size
            sw = rgb_im.crop((int(x - half), int(y - half), int(x + half), int(y + half)))
            sw = sw.resize((128, 128), Image.LANCZOS).convert("RGB")
            sw.save(os.path.join(out, "swatch-%02d.png" % n))
            feats = features(rgb, grey, e, c)
            rows.append({"swatch": "swatch-%02d.png" % n, "panel": pi, "x": round(x, 1), "y": round(y, 1),
                         "size_px": round(size, 1), "orientation": orientation, "score": round(score, 3),
                         **feats, "name": None, "kind": None, "sheet": None})
        panel.save(os.path.join(out, "panel-%d.jpg" % pi), quality=88)
    contact(rgb_im, rows, out, rotate)
    with open(os.path.join(out, "naming.json"), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    return rows


def contact(rgb_im, rows, out, rotate):
    """Every swatch with its label neighbourhood (3.2 sizes wide, 2.6 tall below the centre), numbered,
    six to a row, for the one naming look. --rotate turns each crop for a chart printed sideways."""
    if not rows:
        return
    cell_w, cell_h = 300, 240
    per_row = 6
    n_rows = (len(rows) + per_row - 1) // per_row
    sheet = Image.new("RGB", (cell_w * per_row, cell_h * n_rows), "white")
    dr = ImageDraw.Draw(sheet)
    f = font(16)
    for i, r in enumerate(rows):
        s = r["size_px"]
        x, y = r["x"], r["y"]
        crop = rgb_im.crop((int(x - 1.6 * s), int(y - 1.3 * s), int(x + 1.6 * s), int(y + 2.2 * s)))
        if rotate:
            crop = crop.rotate(rotate, expand=True)
        scale = min((cell_w - 8) / crop.width, (cell_h - 28) / crop.height)
        crop = crop.resize((max(1, int(crop.width * scale)), max(1, int(crop.height * scale))), Image.LANCZOS)
        ox, oy = (i % per_row) * cell_w, (i // per_row) * cell_h
        sheet.paste(crop, (ox + 4, oy + 24))
        dr.text((ox + 6, oy + 4), "%02d  %s  %s" % (i + 1, r["kind_guess"], r["colour"]), fill=(200, 0, 0), font=f)
    sheet.save(os.path.join(out, "contact.jpg"), quality=88)


# ---------------------------------------------------------------- finish


def finish(out):
    """naming.json -> vocabulary.json: every row named or skipped, kinds from the closed set, sheet ids
    unique per kind. Rows with "from" are library borrowings and are kept as the README says."""
    with open(os.path.join(out, "naming.json"), encoding="utf-8") as fh:
        rows = json.load(fh)
    vocab = []
    seen = set()
    for r in rows:
        if "skip" == r.get("name"):
            continue
        if not r.get("name") or not r.get("kind") or not r.get("sheet"):
            raise ValueError("%s is not named: name, kind and sheet are all required (or name 'skip')" % r["swatch"])
        if r["kind"] not in KINDS:
            raise ValueError("%s: kind '%s' is not one of %s" % (r["swatch"], r["kind"], KINDS))
        key = (r["kind"], r["sheet"])
        if key in seen:
            raise ValueError("%s: sheet id '%s' already used for kind %s" % (r["swatch"], r["sheet"], r["kind"]))
        seen.add(key)
        entry = {"swatch": r["swatch"], "name": r["name"], "kind": r["kind"], "sheet": r["sheet"],
                 "shape": r.get("shape", "line" if r["kind"].startswith("side") else "fill"),
                 "colour": r["colour"]}
        if r.get("from"):
            entry["from"] = r["from"]
        vocab.append(entry)
    with open(os.path.join(out, "vocabulary.json"), "w", encoding="utf-8") as fh:
        json.dump(vocab, fh, indent=1)
    return vocab


# ---------------------------------------------------------------- main


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    folder = argv[1]
    out = os.path.join(folder, "legend")
    if "--finish" in argv:
        vocab = finish(out)
        print("%s: vocabulary %d entries: %s" % (os.path.basename(folder), len(vocab),
              ", ".join("%s/%s" % (v["kind"], v["sheet"]) for v in vocab)))
        return 0
    with open(os.path.join(folder, "lattice.json"), encoding="utf-8") as fh:
        d = json.load(fh)
    fractions = [float(t) for t in option(argv, "--sizes", ",".join("%.2f" % (0.35 + 0.05 * i) for i in range(15))).split(",")]
    min_count = int(option(argv, "--min", 3))
    rotate = float(option(argv, "--rotate", 0))
    rgb_im = Image.open(d["source"]).convert("RGB")
    e = L.gradient(rgb_im)
    mask = furniture_mask(d, e.shape)
    ref = printed_reference(e, d)
    given = "--panel" in argv
    if given:
        # a person's rectangle (source pixels) replaces the panel search; inside it the gate relaxes,
        # since the rectangle has already excluded text blocks and tables (Ben, 2026-09-19: what the eye
        # does in a second, the finder need not do at all)
        i = argv.index("--panel") + 1
        x0, y0, x1, y1 = (int(float(v)) for v in argv[i:i + 4])
        rect = np.zeros_like(mask)
        rect[max(0, y0):y1, max(0, x0):x1] = True
        mask = rect
    margin = float(option(argv, "--margin", 0.08 if given else 0.12))
    threshold = float(option(argv, "--coverage", 0.5 if given else 0.9))
    cands = candidates(e, mask, d, fractions, threshold, margin, tolerance=3 if given else 2, ring_gate=1.0 if given else 0.3)
    panels = panels_of(cands, min_count)
    grown = []
    added_total = 0
    for g in panels:
        size = float(np.median([c[2] for c in g]))
        pad = 8 * size
        limits = (min(c[0] for c in g) - pad, min(c[1] for c in g) - pad, max(c[0] for c in g) + pad, max(c[1] for c in g) + pad)
        if given:
            limits = (max(limits[0], x0), max(limits[1], y0), min(limits[2], x1), min(limits[3], y1))
        g2, added = grow_panel(e, g, limits, tolerance=3, margin=0.06)
        grown.append(g2)
        added_total += added
    panels = grown
    rows = write_outputs(rgb_im, d, panels, out, rotate)
    print("%s: %s %.0f%% of the scan, map outline ink %.3f, line margin %.2f, coverage >= %.2f, candidates %d, panels %s (%d grown), swatches %d%s" % (
        os.path.basename(folder), "given panel" if given else "furniture", 100.0 * mask.mean(), ref, margin, threshold, len(cands),
        [len(g) for g in panels], added_total, len(rows), "" if rows else "  NO LEGEND PANEL FOUND: style-library case"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
