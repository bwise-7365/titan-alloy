# Copyright Ben Paul Wise. All Rights Reserved.
"""cells.py -- score every lattice cell of a scan against the named legend swatches (B3, pure CPU).

    python cells.py DIR [--vocabulary OTHER/legend] [--overlay]

DIR is the map's work folder: lattice.json (the grown lattice and its numbering) and legend/ (the
vocabulary and naming.json written by legend.py, or borrowed with --vocabulary from another map's
legend folder when this map has none). Writes DIR/cells.json in the README's shape: one row per printed hex (the grown lattice's other cells are margin and furniture)
("hex", "glyph", "ring", "vertex") and one per hexside ("along", "across", "mid"), scores in [0, 1] per
sheet id, only kinds the vocabulary declares. Nothing here is a decision; it is measurement, and
structure.py ranks it. --overlay paints DIR/cells-overlay.jpg for one look: each cell in its best fill,
hexsides and spokes drawn where a line kind scores above one half.

Every reference is measured on the legend's own swatch with the same probes that measure the cells, so
a kind is "what this print does" and not a stored constant: a hex fill is the colour of an annulus
inside the outline and outside any pictogram; a ring
is the mean colour of the six side bands; a glyph is the colour of what is drawn over the swatch's fill;
a side-along, side-across or side-mid line is the colour drawn over the fill in the swatch's one side
band, spoke or side midpoint that carries drawing. A fill scores by colour similarity; a line by the
fraction of the band's length at which some pixel across it has the line colour (so a river that
wanders off the printed hexside still scores by the length it covers); a glyph by how much is drawn in
the interior times the similarity of its colour. Whatever the swatch does not show scores 0. The printed
hex id is not masked yet: the median makes small ids harmless, and no map read so far prints ids in a
colour that survives it (TODO before Target Leningrad).
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lattice as L
from legend import option

DIRS = {"flat": ["se", "s", "sw", "nw", "n", "ne"], "pointy": ["se", "sw", "w", "nw", "ne", "e"]}
COLOUR_TOLERANCE = 80.0  # RGB distance at which a colour similarity reaches zero


# ---------------------------------------------------------------- probes

HIT = 70.0  # RGB distance within which a pixel counts as the reference colour (the legend renders a line smaller and darker than the map does: BFM river 60 apart)
ALONG_HALF = 0.30  # half-width of the hexside band, in hex sizes (rivers wander off the printed side)
ACROSS_HALF = 0.15  # half-width of the spoke band
OUTLIER = 40.0  # RGB distance from a region's own median beyond which a pixel is "drawn on", not fill
GLYPH_FULL = 0.04  # outlier fraction at which a glyph score is 1 (a small pictogram covers about 4%)
MID_FULL = 0.30  # hit fraction at which a hexside-midpoint mark scores 1
TEXTURED = 0.05  # a fill swatch with more than this share drawn over its fill is a textured kind
CORRIDOR = 0.30  # a spoke scoring above this is masked out of the glyph probe


class Probe:
    """Pixel samples at a hex placed on an image: an interior disc, a band along each side, a band
    along each spoke (centre to side midpoint), a disc at each side midpoint."""

    def __init__(self, rgb_im):
        self.rgb = np.asarray(rgb_im).astype(np.float32)
        self.h, self.w = self.rgb.shape[:2]

    def _sample(self, xs, ys):
        ix = np.clip(np.rint(xs).astype(int), 0, self.w - 1)
        iy = np.clip(np.rint(ys).astype(int), 0, self.h - 1)
        return self.rgb[iy, ix]

    def interior(self, x, y, size, radius=0.45):
        r = radius * size
        ys, xs = np.mgrid[int(y - r):int(y + r) + 1, int(x - r):int(x + r) + 1]
        inside = (xs - x) ** 2 + (ys - y) ** 2 <= r * r
        return self._sample(xs[inside], ys[inside])

    def annulus(self, x, y, size, inner=0.55, outer=0.78):
        ys, xs = np.mgrid[int(y - outer * size):int(y + outer * size) + 1, int(x - outer * size):int(x + outer * size) + 1]
        rr = (xs - x) ** 2 + (ys - y) ** 2
        ring = (rr >= (inner * size) ** 2) & (rr <= (outer * size) ** 2)
        return self._sample(xs[ring], ys[ring])

    def fill(self, x, y, size):
        return median_colour(self.annulus(x, y, size))

    def side_band(self, x, y, size, rot, orientation, k, half=ALONG_HALF):
        (x0, y0), (x1, y1) = _side_ends(size, rot, orientation, k)
        nx, ny = _normal(rot, orientation, k)
        w = max(2.0, half * size)
        ts = np.linspace(0.15, 0.85, 20)
        offs = np.linspace(-w, w, max(5, int(w)))
        xs = x + (x0 + (x1 - x0) * ts)[:, None] + nx * offs[None, :]
        ys = y + (y0 + (y1 - y0) * ts)[:, None] + ny * offs[None, :]
        return self._sample(xs, ys)

    def spoke(self, x, y, size, rot, orientation, k, half=ACROSS_HALF):
        nx, ny = _normal(rot, orientation, k)
        apothem = size * math.sqrt(3) / 2
        w = max(2.0, half * size)
        ts = np.linspace(0.25, 0.85, 16) * apothem
        offs = np.linspace(-w, w, max(5, int(w)))
        xs = x + nx * ts[:, None] - ny * offs[None, :]
        ys = y + ny * ts[:, None] + nx * offs[None, :]
        return self._sample(xs, ys)

    def midpoint(self, x, y, size, rot, orientation, k):
        nx, ny = _normal(rot, orientation, k)
        apothem = size * math.sqrt(3) / 2
        r = 0.2 * size
        mx, my = x + nx * apothem, y + ny * apothem
        ys, xs = np.mgrid[int(my - r):int(my + r) + 1, int(mx - r):int(mx + r) + 1]
        inside = (xs - mx) ** 2 + (ys - my) ** 2 <= r * r
        return self._sample(xs[inside], ys[inside])


def _side_ends(size, rot, orientation, k):
    start = 30.0 if "pointy" == orientation else 0.0
    a0 = math.radians(start + 60 * k + rot)
    a1 = math.radians(start + 60 * (k + 1) + rot)
    return (size * math.cos(a0), size * math.sin(a0)), (size * math.cos(a1), size * math.sin(a1))


def _normal(rot, orientation, k):
    start = 30.0 if "pointy" == orientation else 0.0
    a = math.radians(start + 60 * k + 30 + rot)
    return math.cos(a), math.sin(a)


def median_colour(pix):
    return np.median(pix.reshape(-1, 3), axis=0)


def similarity(colour, reference):
    return max(0.0, 1.0 - float(np.linalg.norm(colour - reference)) / COLOUR_TOLERANCE)


def drawn_on(pix, fill, outline=None):
    """The pixels of a sample that are neither its fill nor (when given) the printed hex outline:
    farther than OUTLIER from each."""
    flat = pix.reshape(-1, 3)
    keep = np.linalg.norm(flat - fill, axis=1) > OUTLIER
    if outline is not None:
        keep &= np.linalg.norm(flat - outline, axis=1) > OUTLIER
    return flat[keep]


def line_colour(pix, fill, outline=None):
    """The colour of what is drawn over a fill: the median of the outlier pixels (the thin tan rail in a
    pale swatch, not the band's pale median; SMW's red border dashes, not the grey outline they cross)."""
    out = drawn_on(pix, fill, outline)
    return median_colour(out) if len(out) else None


def swatch_outline(probe, x, y, size, orientation, fill):
    """The swatch's own printed outline colour: median over its six sides of what a 6%-wide band on the
    side shows over the fill (the line the swatch demonstrates is on one or two sides; the outline is on
    all six)."""
    per_side = []
    for k in range(6):
        drawn = drawn_on(probe.side_band(x, y, size, 0.0, orientation, k, 0.06), fill)
        if len(drawn):
            per_side.append(median_colour(drawn))
    return np.median(np.array(per_side), axis=0) if per_side else fill


def is_line(pix, reference, others):
    """Pixels that are the reference colour: within HIT of it and nearer to it than to any of the
    others (the two fills with their printed textures, the hex outline, the other line kinds). SMW's
    sea fill is 65 units from its river blue, and rough's dark speckles are 55 from its red front line;
    both are nearer to their own fill's palette than to the line, so they are not the line."""
    d = np.linalg.norm(pix - reference, axis=-1)
    hit = d < HIT
    if len(others):
        nearest = np.min(np.linalg.norm(pix[..., None, :] - others, axis=-1), axis=-1)
        hit &= d < nearest
    return hit


def run_fraction(pix, reference, others):
    """For a band sampled (along, across): the fraction of positions along it at which some pixel across
    it is the line. A wandering river or a straight rail both score by length covered."""
    return float(is_line(pix, reference, others).any(axis=1).mean())


def hit_fraction(pix, reference, others):
    return float(is_line(pix.reshape(-1, 3), reference, others).mean())


def palette(pix, share=0.03, step=16):
    """The colours a fill kind prints, texture included: every quantised colour that covers at least
    `share` of the swatch interior (rough's speckles, forest's mottle, the fill itself)."""
    q = np.floor(pix.reshape(-1, 3) / step) * step + step / 2
    keys, counts = np.unique(q, axis=0, return_counts=True)
    keep = counts >= share * len(q)
    return keys[keep] if keep.any() else np.array([median_colour(pix)])


def not_in(pix, others, tolerance=OUTLIER):
    """Pixels farther than the tolerance from every colour in others."""
    if 0 == len(others):
        return pix.reshape(-1, 3)
    flat = pix.reshape(-1, 3)
    return flat[np.min(np.linalg.norm(flat[:, None, :] - others, axis=-1), axis=-1) > tolerance]


def grid_colour(probe, d, cells, fill_of):
    """The colour of the printed hex outline: on a sample of hexsides, the median of what is drawn over
    the fill in a band 8% of a hex wide, then the median of those across the sides (rivers and borders
    are on a minority of sides, so the outline wins). It is drawn over every fill and is not a line
    kind, so the line probes exclude it."""
    size, ori, rot = d["size"], d["orientation"], float(d.get("rotation_deg", 0.0))
    keys = list(cells)[::max(1, len(cells) // 40)]
    per_side = []
    for k in keys:
        for s in range(6):
            drawn = drawn_on(probe.side_band(cells[k][0], cells[k][1], size, rot, ori, s, 0.08), fill_of[k])
            if len(drawn):
                per_side.append(median_colour(drawn))
    return np.median(np.array(per_side), axis=0)


def glyph_pixels(probe, x, y, size, own_palette, grid, strong_spokes, radius=0.45):
    """Interior pixels that are drawn on: not the cell's fill palette, not the outline colour, and not
    inside the corridor of a spoke that scored as a line (a rail through a hex is not a city glyph;
    the star of a mobilization hex, red like a border, is a glyph because it is in the middle)."""
    r = radius * size
    ys, xs = np.mgrid[int(y - r):int(y + r) + 1, int(x - r):int(x + r) + 1]
    dx, dy = xs - x, ys - y
    keep = dx * dx + dy * dy <= r * r
    w = max(2.0, ACROSS_HALF * size)
    for nx, ny in strong_spokes:
        # the corridor starts a quarter hex out: a star or a city sits on the centre with the rails
        along = dx * nx + dy * ny
        perp = np.abs(-dx * ny + dy * nx)
        keep &= ~((along > 0.25 * size) & (perp < w))
    pix = probe._sample(xs[keep], ys[keep])
    return not_in(pix, np.vstack([own_palette, grid[None, :]]))


def odd_one_out(samples, fill, outline):
    """Of six probes, the one with most pixels that are neither fill nor outline: the swatch's line side
    or spoke, not one of its plain sides."""
    return max(samples, key=lambda pix: len(drawn_on(pix, fill, outline)))


# ---------------------------------------------------------------- references from the legend


def glyph_slot(legend_dir):
    """[dx, dy] in hex sizes from legends.json for this map, or the centre."""
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "legends.json")
    name = os.path.basename(os.path.dirname(os.path.abspath(legend_dir)))
    if os.path.exists(path):
        with open(path, encoding="utf-8") as fh:
            entry = json.load(fh).get(name, {})
        return tuple(entry.get("glyph-slot", (0.0, 0.0)))
    return (0.0, 0.0)


def references(legend_dir):
    """One reference per vocabulary entry, measured on the legend's own swatch."""
    with open(os.path.join(legend_dir, "vocabulary.json"), encoding="utf-8") as fh:
        vocab = json.load(fh)
    with open(os.path.join(legend_dir, "naming.json"), encoding="utf-8") as fh:
        rows = {r["swatch"]: r for r in json.load(fh)}
    with open(os.path.join(os.path.dirname(legend_dir), "lattice.json"), encoding="utf-8") as fh:
        source = json.load(fh)["source"]
    probe = Probe(Image.open(source).convert("RGB"))
    slot = glyph_slot(legend_dir)
    refs = []
    for v in vocab:
        if v.get("from") or v["swatch"] not in rows:
            raise ValueError("%s: borrowed swatches without a print to measure are not supported yet" % v["name"])
        r = rows[v["swatch"]]
        x, y, size, ori = r["x"], r["y"], r["size_px"], r["orientation"]
        ref = {"name": v["name"], "kind": v["kind"], "sheet": v["sheet"]}
        interior = probe.interior(x, y, size)
        fill = probe.fill(x, y, size)
        outline = swatch_outline(probe, x, y, size, ori, fill)
        if "hex" == v["kind"]:
            ref["colour"] = fill
            ref["palette"] = palette(interior)
            drawn = drawn_on(interior, fill, outline)
            ref["texture_share"] = len(drawn) / len(interior)
            ref["texture"] = median_colour(drawn) if ref["texture_share"] > TEXTURED else None
        elif "ring" == v["kind"]:
            ref["colour"] = np.mean([median_colour(probe.side_band(x, y, size, 0.0, ori, k, 0.08)) for k in range(6)], axis=0)
        elif "glyph" == v["kind"]:
            ref["colour"] = line_colour(probe.interior(x + slot[0] * size, y + slot[1] * size, size, 0.35 if any(slot) else 0.45), fill, outline)
        elif "side-along" == v["kind"]:
            ref["colour"] = line_colour(odd_one_out([probe.side_band(x, y, size, 0.0, ori, k) for k in range(6)], fill, outline), fill, outline)
        elif "side-across" == v["kind"]:
            ref["colour"] = line_colour(odd_one_out([probe.spoke(x, y, size, 0.0, ori, k) for k in range(6)], fill, outline), fill, outline)
        elif "side-mid" == v["kind"]:
            ref["colour"] = line_colour(odd_one_out([probe.midpoint(x, y, size, 0.0, ori, k) for k in range(6)], fill, outline), fill, outline)
        else:
            raise ValueError("%s: kind %s is not scored yet (vertex, region come with their maps)" % (v["name"], v["kind"]))
        if ref["colour"] is None:
            raise ValueError("%s: the swatch shows nothing drawn over its fill; check legends.json" % v["name"])
        refs.append(ref)
    return refs


# ---------------------------------------------------------------- cells


def score_map(d, refs, probe, slot=(0.0, 0.0)):
    size, ori, rot, spacing = d["size"], d["orientation"], float(d.get("rotation_deg", 0.0)), d["spacing"]
    numbering = d.get("numbering")
    printed = set(d["printed"]["cells"])
    cells = {tuple(int(t) for t in k[1:].split("r")): (v[0], v[1]) for k, v in d["grown"]["cells"].items() if k in printed}
    cols = max(c for c, _ in cells) + 1
    rows = max(r for _, r in cells) + 1
    label = {k: L.cell_label(k, numbering, cols, rows) for k in cells}
    keys = list(cells)
    tree = cKDTree([cells[k] for k in keys])
    by_kind = {}
    for ref in refs:
        by_kind.setdefault(ref["kind"], []).append(ref)
    hexes = {}
    sides = {}
    spoke_scores = {}
    fill_of = {k: probe.fill(cells[k][0], cells[k][1], size) for k in keys}
    grid = grid_colour(probe, d, cells, fill_of)
    line_refs = {ref["sheet"]: ref["colour"] for kind in ("side-along", "side-across", "side-mid") for ref in by_kind.get(kind, [])}
    palettes = {}
    for k in keys:
        colour = fill_of[k]
        scores = {ref["sheet"]: similarity(colour, ref["colour"]) for ref in by_kind.get("hex", [])}
        best = max(scores, key=scores.get) if scores else None
        own = [colour[None, :]]
        if best is not None and scores[best] > 0.5:
            own.append(next(r for r in by_kind["hex"] if r["sheet"] == best)["palette"])
        palettes[k] = np.vstack(own)

    def others_for(sheet, ks):
        return np.vstack([palettes[q] for q in ks] + [grid[None, :]] + [c[None, :] for sh, c in line_refs.items() if sh != sheet])

    for k in keys:
        x, y = cells[k]
        interior = probe.interior(x, y, size)
        colour = fill_of[k]
        row = {"hex": {}, "glyph": {}, "ring": {}, "vertex": {}}
        textured = [ref for ref in by_kind.get("hex", []) if ref["texture"] is not None]
        texture_score = {}
        if textured:
            own = not_in(interior, np.vstack([colour[None, :], grid[None, :]]))
            for ref in textured:
                share = float((np.linalg.norm(own - ref["texture"], axis=1) < HIT).sum()) / len(interior)
                texture_score[ref["sheet"]] = min(1.0, share / ref["texture_share"])
        most_texture = max(texture_score.values()) if texture_score else 0.0
        for ref in by_kind.get("hex", []):
            sim = similarity(colour, ref["colour"])
            if ref["texture"] is not None:
                row["hex"][ref["sheet"]] = round(0.5 * sim + 0.5 * texture_score[ref["sheet"]], 2)
            else:
                row["hex"][ref["sheet"]] = round(sim * (1.0 - 0.5 * most_texture), 2)
        bands = [probe.side_band(x, y, size, rot, ori, s) for s in range(6)] if by_kind.get("ring") or by_kind.get("side-along") else None
        for ref in by_kind.get("ring", []):
            mid = bands[0].shape[1] // 2
            row["ring"][ref["sheet"]] = round(float(np.mean([similarity(median_colour(b[:, mid - 1:mid + 2]), ref["colour"]) for b in bands])), 2)
        strong_spokes = []
        for s in range(6):
            nx, ny = _normal(rot, ori, s)
            dist, j = tree.query((x + nx * spacing, y + ny * spacing))
            other = keys[j] if dist < 0.3 * spacing else None
            name = "%s:%s" % (label[k], DIRS[ori][s]) if other is None else "-".join(sorted((label[k], label[other])))
            pair = [k] if other is None else [k, other]
            if by_kind.get("side-across"):
                spoke = probe.spoke(x, y, size, rot, ori, s)
                sc = {ref["sheet"]: run_fraction(spoke, ref["colour"], others_for(ref["sheet"], [k])) for ref in by_kind["side-across"]}
                spoke_scores.setdefault(name, []).append(sc)
                if max(sc.values()) > CORRIDOR:
                    strong_spokes.append((nx, ny))
            if name in sides:
                continue
            side = {"along": {}, "across": {}, "mid": {}}
            for ref in by_kind.get("side-along", []):
                side["along"][ref["sheet"]] = round(run_fraction(bands[s], ref["colour"], others_for(ref["sheet"], pair)), 2)
            for ref in by_kind.get("side-mid", []):
                side["mid"][ref["sheet"]] = round(min(1.0, hit_fraction(probe.midpoint(x, y, size, rot, ori, s), ref["colour"], others_for(ref["sheet"], pair)) / MID_FULL), 2)
            sides[name] = side
        if by_kind.get("glyph"):
            drawn = glyph_pixels(probe, x + slot[0] * size, y + slot[1] * size, size, palettes[k], grid, strong_spokes, 0.35 if any(slot) else 0.45)
            glyph_refs = np.array([ref["colour"] for ref in by_kind["glyph"]])
            if len(drawn):
                dist = np.linalg.norm(drawn[:, None, :] - glyph_refs, axis=-1)
                nearest = np.argmin(dist, axis=1)
                counted = dist[np.arange(len(drawn)), nearest] < HIT
            for i, ref in enumerate(by_kind["glyph"]):
                n = int(((nearest == i) & counted).sum()) if len(drawn) else 0
                row["glyph"][ref["sheet"]] = round(min(1.0, n / (GLYPH_FULL * len(interior))), 2)
        hexes[label[k]] = row
    for name, per_hex in spoke_scores.items():
        for sheet in per_hex[0]:
            sides[name]["across"][sheet] = round(float(np.mean([p[sheet] for p in per_hex])), 2)
    print("  grid outline colour #%02x%02x%02x" % tuple(int(v) for v in grid))
    return hexes, sides, cells, label


# ---------------------------------------------------------------- overlay


def overlay(rgb_im, d, refs, hexes, sides, cells, label, path):
    size, ori, rot = d["size"], d["orientation"], float(d.get("rotation_deg", 0.0))
    im = rgb_im.copy()
    paint = Image.new("RGBA", im.size, (0, 0, 0, 0))
    dr = ImageDraw.Draw(paint)
    colour = {(r["kind"], r["sheet"]): tuple(int(v) for v in r["colour"]) for r in refs}
    pts = L.hex_outline_points(size * 0.92, rot, ori, n=1)
    for k, (x, y) in cells.items():
        row = hexes[label[k]]
        if row["hex"]:
            best = max(row["hex"], key=row["hex"].get)
            if row["hex"][best] > 0.5:
                dr.polygon([(x + px, y + py) for px, py in pts], fill=colour[("hex", best)] + (150,))
        for sheet, sc in row["glyph"].items():
            if sc > 0.5:
                dr.ellipse((x - 6, y - 6, x + 6, y + 6), fill=(255, 0, 255, 220))
        for s in range(6):
            nx, ny = _normal(rot, ori, s)
            (x0, y0), (x1, y1) = _side_ends(size, rot, ori, s)
            for sheet, sc in _side_of(sides, label, k, s, cells, d).get("along", {}).items():
                if sc > 0.5:
                    dr.line((x + x0, y + y0, x + x1, y + y1), fill=colour[("side-along", sheet)] + (255,), width=5)
            for sheet, sc in _side_of(sides, label, k, s, cells, d).get("across", {}).items():
                if sc > 0.5:
                    a = size * math.sqrt(3) / 2
                    dr.line((x, y, x + nx * a, y + ny * a), fill=(0, 0, 0, 255), width=4)
    im.paste(paint, (0, 0), paint)
    im.save(path, quality=85)


def _side_of(sides, label, k, s, cells, d):
    ori = d["orientation"]
    x, y = cells[k]
    nx, ny = _normal(float(d.get("rotation_deg", 0.0)), ori, s)
    px, py = x + nx * d["spacing"], y + ny * d["spacing"]
    other = min(cells, key=lambda q: (cells[q][0] - px) ** 2 + (cells[q][1] - py) ** 2)
    if math.hypot(cells[other][0] - px, cells[other][1] - py) < 0.3 * d["spacing"]:
        return sides.get("-".join(sorted((label[k], label[other]))), {})
    return sides.get("%s:%s" % (label[k], DIRS[ori][s]), {})


# ---------------------------------------------------------------- main


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    folder = argv[1]
    legend_dir = option(argv, "--vocabulary", os.path.join(folder, "legend"))
    with open(os.path.join(folder, "lattice.json"), encoding="utf-8") as fh:
        d = json.load(fh)
    refs = references(legend_dir)
    rgb_im = Image.open(d["source"]).convert("RGB")
    hexes, sides, cells, label = score_map(d, refs, Probe(rgb_im), glyph_slot(legend_dir))
    out = {"source": d["source"], "vocabulary": os.path.abspath(legend_dir),
           "kinds": sorted({r["kind"] for r in refs}), "hex": hexes, "side": sides}
    with open(os.path.join(folder, "cells.json"), "w", encoding="utf-8") as fh:
        json.dump(out, fh, indent=0)
    if "--overlay" in argv:
        overlay(rgb_im, d, refs, hexes, sides, cells, label, os.path.join(folder, "cells-overlay.jpg"))
    best = {}
    for row in hexes.values():
        if row["hex"] and max(row["hex"].values()) > 0.5:
            b = max(row["hex"], key=row["hex"].get)
            best[b] = best.get(b, 0) + 1
    print("%s: %d printed hexes, %d hexsides, %d references from %s; best fill counts %s" % (
        os.path.basename(os.path.normpath(folder)), len(hexes), len(sides), len(refs), legend_dir, best))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
