# Copyright Ben Paul Wise. All Rights Reserved.
"""candidates.py -- stage 4: colour masks propose features; drafts and candidate tiles for the readers.

    python candidates.py MAP [NAME ...]

From the colour masks in maps/MAP.json "candidates" it measures, on the primary source:
  per hex      the fraction of the hex (inset from its outline) covered by each mask
  per hexside  for each hexside line (river ...): how much of the hexside's length has the line's mask
               within a band beside it ("along", 0-1); for each link kind (rail, road): the runs where
               the link's mask crosses the hexside segment, with their position t (0-1 from one corner)
and writes work/MAP/candidates.json, then for every tile (tile.py's plan) a draft catalogue record
work/MAP/catalogue/draft/TILE.json and a candidate image work/MAP/tiles/cand/TILE.jpg: the scan with
river candidates as magenta hexside strokes (dashed when only "maybe"), rail steps red and road steps
yellow centre to centre, crossings near a corner as a red or yellow ring, and the terrain the draft
proposes as a letter (W woods, S swamp, L lake; lower case when borderline).

Candidates are hints. A draft becomes a catalogue record only when a reader has checked it against
the scan tile (catalogue stage). NAME limits the drafts and images to those tiles.
"""
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import common as C
import overlay
import tile as T


def colour_mask(rgb, spec):
    """A 0/1 mask: pixels within tol (largest channel difference) of any listed rgb, or, with "green",
    pixels whose green exceeds red and blue by the given margins and whose red is at least min_r; then an
    optional grey closing ("close", fills texture speckles), opening ("open", removes thin strokes) and
    growth ("grow", a radius)."""
    near = np.zeros(rgb.shape[:2], dtype=bool)
    for c in spec.get("rgb", []):
        d = np.abs(rgb.astype(np.int16) - np.array(c, dtype=np.int16)).max(axis=2)
        near |= d <= spec["tol"]
    if "green" in spec:
        r, g, b = (rgb[..., i].astype(np.int16) for i in range(3))
        rule = spec["green"]
        near |= (g - r >= rule["over_r"]) & (g - b >= rule["over_b"]) & (r >= rule["min_r"])
    out = near.astype(np.uint8)
    if spec.get("close"):
        out = ndimage.minimum_filter(ndimage.maximum_filter(out, spec["close"]), spec["close"])
    if spec.get("open"):
        out = ndimage.maximum_filter(ndimage.minimum_filter(out, spec["open"]), spec["open"])
    if spec.get("grow"):
        out = ndimage.maximum_filter(out, 2 * spec["grow"] + 1)
    return out


def sample(mask, xs, ys):
    xi = np.clip(np.rint(xs).astype(int), 0, mask.shape[1] - 1)
    yi = np.clip(np.rint(ys).astype(int), 0, mask.shape[0] - 1)
    return mask[yi, xi]


def hex_fractions(grid, masks):
    out = {}
    for pid, (c, r) in grid.ids.items():
        poly = grid.polygon(c, r, inset=0.08)
        xs, ys = [p[0] for p in poly], [p[1] for p in poly]
        x0, y0, x1, y1 = int(min(xs)), int(min(ys)), int(max(xs)) + 1, int(max(ys)) + 1
        stencil = Image.new("L", (x1 - x0, y1 - y0), 0)
        ImageDraw.Draw(stencil).polygon([(x - x0, y - y0) for x, y in poly], fill=1)
        inside = np.asarray(stencil, dtype=bool)
        out[pid] = {k: round(float(m[y0:y1, x0:x1][inside].mean()), 3) for k, m in masks.items()}
    return out


def along(grid, mask, a, b, band):
    """Fraction of 20 bins along segment a-b holding mask pixels within band px of the segment."""
    t = np.linspace(0.0, 1.0, 20)[:, None]
    s = np.linspace(-band, band, 2 * int(band) // 2 + 1)[None, :]
    ux, uy = b[0] - a[0], b[1] - a[1]
    n = np.hypot(ux, uy)
    nx, ny = -uy / n, ux / n
    xs = a[0] + t * ux + s * nx
    ys = a[1] + t * uy + s * ny
    return round(float(sample(mask, xs, ys).max(axis=1).mean()), 2)


def crossings(mask, a, b, half):
    """Runs where mask crosses segment a-b: [t_start, t_end] pairs."""
    n = int(np.hypot(b[0] - a[0], b[1] - a[1]))
    t = np.linspace(0.0, 1.0, n + 1)[:, None]
    ux, uy = b[0] - a[0], b[1] - a[1]
    nx, ny = -uy / n, ux / n
    s = np.arange(-half, half + 1)[None, :]
    hit = sample(mask, a[0] + t * ux + s * nx, a[1] + t * uy + s * ny).max(axis=1) > 0
    runs, start = [], None
    for i, h in enumerate(hit):
        if h and start is None:
            start = i
        if (not h or i == n) and start is not None:
            runs.append([round(start / n, 2), round((i if not h else i) / n, 2)])
            start = None
    return runs


def measure(cfg):
    spec = cfg["candidates"]
    grid = C.make_grid(cfg, C.load_fit(cfg))
    rgb = np.asarray(C.open_image(C.source(cfg, "primary")["path"]))
    masks = {k: colour_mask(rgb, v) for k, v in spec["colour_masks"].items()}
    hexes = hex_fractions(grid, {k: masks[k] for k in spec["hex_masks"]})
    sides = {}
    band = spec["band"] * grid.size
    for name in C.all_sides(grid):
        a, b = C.side_segment(grid, name)
        rec = {line: along(grid, masks[line], a, b, band) for line in spec["hexside_lines"]}
        for kind in spec["links"]:
            runs = crossings(masks[kind], a, b, spec["half_width"])
            if runs:
                rec[kind] = runs
        sides[name] = rec
    data = dict(hexes=hexes, sides=sides)
    C.write_json(C.work(cfg, "candidates.json"), data)
    print("measured %d hexes and %d hexsides" % (len(hexes), len(sides)))
    return grid, data


def terrain_of(spec, frac):
    """The draft terrain and a borderline flag from the configured coverage rules."""
    for terrain, rule in spec["terrain"].items():
        f = frac[rule["mask"]]
        if f >= rule["at_least"]:
            return terrain, f < rule["sure"]
    borderline = any(frac[r["mask"]] >= r["maybe"] for r in spec["terrain"].values())
    return cfg_default(spec), borderline


def cfg_default(spec):
    return spec["default_terrain"]


def link_step(runs, length, spec):
    """'step', 'corner' (a crossing near a hexside end), 'along' or None for one hexside's runs."""
    kind = None
    for t0, t1 in runs:
        if (t1 - t0) >= spec["along_share"]:
            return "along"
        if (t1 - t0) < spec["min_run"] and (t1 <= spec["end"] or t0 >= 1 - spec["end"]):
            continue  # a line passing just beside the hexside's end corner, not crossing it
        mid = (t0 + t1) / 2
        if spec["corner"] <= mid <= 1 - spec["corner"]:
            kind = "step"
        elif kind is None:
            kind = "corner"
    return kind


def draft(cfg, grid, data, t):
    spec = cfg["candidates"]
    area = set(t["core"]) | set(t["margin"])
    rec = dict(tile=t["name"], reader="draft", hexes={}, rivers=[], rail=[], road=[], markers=[], unclear=[], notes=[])
    for line in spec["hexside_lines"]:
        rec.setdefault(line + "s" if line != "river" else "rivers", [])
    for pid in sorted(area, key=lambda p: grid.ids[p]):
        terrain, borderline = terrain_of(spec, data["hexes"][pid])
        rec["hexes"][pid] = dict(terrain=terrain)
        if borderline:
            rec["unclear"].append(dict(at=pid, feature="terrain", note="coverage " + " ".join(
                "%s %.2f" % (k, v) for k, v in data["hexes"][pid].items() if v >= 0.02)))
        city = data["hexes"][pid].get("city", 0)
        if city >= spec["city_at_least"]:
            rec["unclear"].append(dict(at=pid, feature="place", note="city-block colour %.2f: a city or its name?" % city))
    for name, m in data["sides"].items():
        hexes = name.split(":")[0].split("-")
        if not all(h in area for h in hexes):
            continue
        for line in spec["hexside_lines"]:
            v = m[line]
            if v >= spec["along_sure"]:
                rec["rivers" if "river" == line else line + "s"].append(name)
            elif v >= spec["along_maybe"]:
                rec["unclear"].append(dict(at=name, feature=line, note="along %.2f" % v))
        for kind in spec["links"]:
            if kind not in m:
                continue
            what = link_step(m[kind], 1.0, spec)
            thin = [h for h in hexes if data["hexes"][h][kind] < spec["presence"][kind]]
            if "step" == what and not thin:
                rec[kind].append(name)
            elif "step" == what:
                rec["unclear"].append(dict(at=name, feature=kind, note="crosses the hexside, but only a corner of %s holds the line (%s)" % (
                    " ".join(thin), " ".join("%.3f" % data["hexes"][h][kind] for h in thin))))
            elif what is not None and not thin:
                rec["unclear"].append(dict(at=name, feature=kind, note="%s crossing %s" % (what, m[kind])))
    return rec


def draw_candidates(rec, grid, spec):
    def draw(img, t, g):
        x0, y0 = t["box"][0], t["box"][1]
        s = T_scale[0]
        d = ImageDraw.Draw(img, "RGBA")

        def at(p):
            return (p[0] - x0) * s, (p[1] - y0) * s

        maybe = {u["at"]: u for u in rec["unclear"]}
        for name in rec["rivers"]:
            a, b = C.side_segment(grid, name)
            d.line((at(a), at(b)), fill=(255, 0, 200, 150), width=7)
        for name, u in maybe.items():
            if u["feature"] in spec["hexside_lines"]:
                a, b = C.side_segment(grid, name)
                for k in range(0, 10, 2):
                    p = (a[0] + (b[0] - a[0]) * k / 10, a[1] + (b[1] - a[1]) * k / 10)
                    q = (a[0] + (b[0] - a[0]) * (k + 1) / 10, a[1] + (b[1] - a[1]) * (k + 1) / 10)
                    d.line((at(p), at(q)), fill=(255, 0, 200, 150), width=5)
        for kind, colour in (("rail", (230, 0, 0, 200)), ("road", (255, 200, 0, 220))):
            for name in rec[kind]:
                if ":" in name:
                    continue
                h1, h2 = name.split("-")
                d.line((at(grid.centre(*grid.ids[h1])), at(grid.centre(*grid.ids[h2]))), fill=colour, width=3)
            for name, u in maybe.items():
                if u["feature"] == kind and ":" not in name:
                    a, b = C.side_segment(grid, name)
                    mx, my = at(((a[0] + b[0]) / 2, (a[1] + b[1]) / 2))
                    d.ellipse((mx - 9, my - 9, mx + 9, my + 9), outline=colour, width=3)
        f = overlay.font(int(grid.size * 0.3 * s))
        for pid, h in rec["hexes"].items():
            letter = {"woods": "W", "swamp": "S", "lake": "L"}.get(h["terrain"])
            flagged = any(u["at"] == pid and u["feature"] == "terrain" for u in rec["unclear"])
            if letter or flagged:
                cx, cy = at(grid.centre(*grid.ids[pid]))
                d.text((cx, cy), (letter or "?").lower() if flagged else letter, font=f, fill=(0, 60, 255, 255),
                       anchor="mm", stroke_width=2, stroke_fill=(255, 255, 255, 230))
        return img
    return draw


T_scale = [1.0]


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    only = set(argv[2:])
    grid, data = measure(cfg)
    _, tiles = T.manifest(cfg)
    T_scale[0] = cfg["tiles"]["scale"]
    full = C.open_image(C.source(cfg, "primary")["path"])
    n = 0
    for t in tiles:
        if only and t["name"] not in only:
            continue
        rec = draft(cfg, grid, data, t)
        C.write_json(C.work(cfg, "catalogue", "draft", t["name"] + ".json"), rec)
        x0, y0, x1, y1 = t["box"]
        s = T_scale[0]
        img = full.crop((x0, y0, x1, y1)).resize((int((x1 - x0) * s), int((y1 - y0) * s)), Image.LANCZOS)
        draw_candidates(rec, grid, cfg["candidates"])(img, t, grid)
        overlay.draw_grid(img, grid, t["box"], s, core=set(t["core"]), dots=False)
        C.save_jpeg(img, C.work(cfg, "tiles", "cand", t["name"] + ".jpg"))
        n += 1
    print("wrote %d drafts and candidate tiles" % n)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
