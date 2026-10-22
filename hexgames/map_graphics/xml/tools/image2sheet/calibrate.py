# Copyright Ben Paul Wise. All Rights Reserved.
"""calibrate.py -- stage 1: fit the hex grid to printed hex numbers; the geometry gate.

    python calibrate.py MAP locate SOURCE ID X Y [ID X Y ...]
    python calibrate.py MAP fit [SOURCE ...]

locate  refines each approximate hex centre (X, Y in source pixels, read off a crop.py --ruler crop)
        against the printed grid lines, prints a control-point line to paste into the source's
        "control_points" in maps/MAP.json, and writes work/MAP/calibrate/SOURCE-ID.jpg: the hex with a
        red cross at the refined centre and the expected id written above the crop. Read that crop:
        the printed number in the crossed hex must be ID, and the magenta outline must lie on the
        printed hex outline. Only then paste the line.
fit     least-squares fit of size, ox and oy to the source's control points (default source: all
        sources with control points). Reports every control point's residual in hexes (1 hex = the
        centre-to-centre spacing), the worst per region of the image, an affine fit as a diagnostic
        (scale x and y, rotation), the printed extent, every grid hex whose outline is not printed, any
        printed hex outline just outside the grid, and a dense check that refines every hex centre.
        Writes work/MAP/calibration.json.
        GATE: every control-point residual under 0.1 hex, at least one control point in each of the
        nine regions, every grid hex inside the image, and no printed hex outside the grid. Exit 1 on
        failure. The dense check reports; its worst hexes are for a look, not a gate.
"""
import math
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import common as C
import overlay

GATE_HEX = 0.10
LINE_DARK = 60   # grey level under which a pixel is grid-line ink (per source: "line_dark")
LINED = 0.18     # mean line strength along an edge from which the edge counts as printed


def line_map(src):
    grey = np.asarray(C.open_image(src["path"]).convert("L"))
    dark = (grey < src.get("line_dark", LINE_DARK)).astype(np.float32)
    return ndimage.gaussian_filter(dark, 1.2)


def outline(geom, size, n=15):
    """Sample points along the six edges of a hex of circumradius size (corners left out), with edge index."""
    angs = [math.radians(a) for a in geom["corners"].values()]
    pts, edge = [], []
    for i in range(6):
        a, b = angs[i], angs[(i + 1) % 6]
        for t in np.linspace(0.15, 0.85, n):
            pts.append((((1 - t) * math.cos(a) + t * math.cos(b)) * size,
                        ((1 - t) * math.sin(a) + t * math.sin(b)) * size))
            edge.append(i)
    return np.array(pts), np.array(edge)


def sample(L, xs, ys):
    xi = np.clip(np.rint(xs).astype(int), 0, L.shape[1] - 1)
    yi = np.clip(np.rint(ys).astype(int), 0, L.shape[0] - 1)
    return L[yi, xi]


def vertex(a, b, c):
    den = a - 2 * b + c
    return 0.0 if 0 == den else 0.5 * (a - c) / den


def lined_edges(L, geom, size, x, y):
    pts, edge = outline(geom, size)
    v = sample(L, x + pts[:, 0], y + pts[:, 1])
    return int(sum(v[edge == i].mean() >= LINED for i in range(6)))


def refine(L, geom, size, x, y, reach):
    """The centre near (x, y) where a hex outline of this size best matches the printed lines."""
    pts, _ = outline(geom, size)
    offs = np.arange(-reach, reach + 1)
    dx, dy = np.meshgrid(offs, offs)
    score = sample(L, x + dx[..., None] + pts[:, 0], y + dy[..., None] + pts[:, 1]).mean(axis=-1)
    j, i = np.unravel_index(np.argmax(score), score.shape)
    k = len(offs) - 1
    fx = vertex(score[j, i - 1], score[j, i], score[j, i + 1]) if 0 < i < k else 0.0
    fy = vertex(score[j - 1, i], score[j, i], score[j + 1, i]) if 0 < j < k else 0.0
    bx, by = x + offs[i] + fx, y + offs[j] + fy
    return bx, by, float(score[j, i]), lined_edges(L, geom, size, bx, by)


def locate(cfg, name, args):
    src = C.source(cfg, name)
    geom = C.H.FLAT if "flat" == cfg["grid"]["orientation"] else C.H.POINTY
    size = float(src["size_hint"])
    L = line_map(src)
    full = C.open_image(src["path"])
    for i in range(0, len(args), 3):
        pid, x, y = args[i], float(args[i + 1]), float(args[i + 2])
        bx, by, score, lined = refine(L, geom, size, x, y, int(0.35 * size))
        print('{"id": "%s", "x": %.1f, "y": %.1f},   score %.2f, printed edges %d of 6' % (pid, bx, by, score, lined))
        half, scale = 1.6 * size, 2.0
        box = (int(bx - half), int(by - half), int(bx + half), int(by + half))
        img = full.crop(box).resize((int(2 * half * scale), int(2 * half * scale)), Image.LANCZOS)
        d = ImageDraw.Draw(img, "RGBA")
        cx, cy = (bx - box[0]) * scale, (by - box[1]) * scale
        d.line((cx - 12, cy, cx + 12, cy), fill=(255, 0, 0, 255), width=2)
        d.line((cx, cy - 12, cx, cy + 12), fill=(255, 0, 0, 255), width=2)
        pts, _ = outline(geom, size, 2)
        angs = [math.radians(a) for a in geom["corners"].values()]
        poly = [(cx + size * scale * math.cos(a), cy + size * scale * math.sin(a)) for a in angs]
        d.line(poly + [poly[0]], fill=overlay.OUTLINE, width=2)
        d.text((6, 4), "expect %s" % pid, font=overlay.font(18), fill=(255, 0, 0, 255), stroke_width=2,
               stroke_fill=(255, 255, 255))
        print("   crop", C.save_jpeg(img, C.work(cfg, "calibrate", "%s-%s.jpg" % (name, pid))))
    return 0


def region(x, y, W, H):
    v = "N" if y < H / 3 else ("S" if y > 2 * H / 3 else "")
    h = "W" if x < W / 3 else ("E" if x > 2 * W / 3 else "")
    return (v + h) or "C"


def lstsq(A, b):
    return np.linalg.lstsq(np.array(A, dtype=float), np.array(b, dtype=float), rcond=None)[0]


def fit(cfg, name, fits):
    src = C.source(cfg, name)
    points = src.get("control_points", [])
    if len(points) < 3:
        raise ValueError("source %s: %d control points, need at least 3 (12 or more for the gate)" % (name, len(points)))
    with Image.open(src["path"]) as im:
        W, H = im.size
    unit = C.make_grid(cfg, dict(size=1.0, ox=0.0, oy=0.0))
    A, b, rows = [], [], []
    for p in points:
        ux, uy = unit.centre(*unit.ids[C.known(unit, p["id"], "%s control point" % name)])
        A += [[1, 0, ux], [0, 1, uy]]
        b += [p["x"], p["y"]]
        rows.append((p, ux, uy))
    ox, oy, size = lstsq(A, b)
    result = dict(size=float(size), ox=float(ox), oy=float(oy))
    grid = C.make_grid(cfg, result)
    hexu = C.spacing(grid)
    failures = []
    print("== %s: %s (%d x %d)" % (name, src["path"], W, H))
    print("fit: size %.3f  ox %.2f  oy %.2f  (1 hex = %.2f px)" % (size, ox, oy, hexu))
    worst = {}
    print("control points (residual in hexes):")
    for p, ux, uy in rows:
        cx, cy = grid.centre(*grid.ids[p["id"]])
        res = math.hypot(p["x"] - cx, p["y"] - cy) / hexu
        reg = region(p["x"], p["y"], W, H)
        worst[reg] = max(worst.get(reg, (0, ""))[0], res), p["id"] if res >= worst.get(reg, (0, ""))[0] else worst[reg][1]
        flag = "  <-- over gate" if res >= GATE_HEX else ""
        print("  %-6s %-3s dx %+6.2f dy %+6.2f  %.3f%s" % (p["id"], reg, p["x"] - cx, p["y"] - cy, res, flag))
        if res >= GATE_HEX:
            failures.append("control point %s residual %.3f hex" % (p["id"], res))
    print("worst per region:", "  ".join("%s %.3f (%s)" % (k, v[0], v[1]) for k, v in sorted(worst.items())))
    for reg in ("NW", "N", "NE", "W", "C", "E", "SW", "S", "SE"):
        if reg not in worst:
            failures.append("no control point in region %s" % reg)
    ax = lstsq([[1, ux, uy] for _, ux, uy in rows], [p["x"] for p, _, _ in rows])
    ay = lstsq([[1, ux, uy] for _, ux, uy in rows], [p["y"] for p, _, _ in rows])
    result["affine"] = dict(scale_x=float(math.hypot(ax[1], ay[1])), scale_y=float(math.hypot(ax[2], ay[2])),
                            rotation_deg=float(math.degrees(math.atan2(ay[1], ax[1]))))
    print("affine diagnostic: scale x %.3f  scale y %.3f  rotation %.4f deg" % tuple(result["affine"].values()))
    outside = [pid for pid, (c, r) in grid.ids.items()
               if any(not (0 <= x <= W and 0 <= y <= H) for x, y in grid.polygon(c, r))]
    if outside:
        failures.append("%d grid hexes extend outside the image: %s" % (len(outside), " ".join(sorted(outside)[:12])))
    for key in ("first", "last"):
        C.known(grid, cfg["grid"]["extent"][key], "grid extent")
    geom = grid.geom
    L = line_map(src)
    unprinted = [pid for pid, (c, r) in sorted(grid.ids.items(), key=lambda kv: kv[1])
                 if lined_edges(L, geom, size, *grid.centre(c, r)) <= 2]
    print("grid hexes with 2 or fewer printed edges: %d %s" % (len(unprinted), " ".join(unprinted[:30])))
    beyond = []
    for c in range(-1, grid.cols + 1):
        for r in range(-1, grid.rows + 1):
            if (c, r) in grid.cells:
                continue
            x, y = grid.centre(c, r)
            if size < x < W - size and size < y < H - size and lined_edges(L, geom, size, x, y) >= 5:
                beyond.append("index (%d,%d) at %.0f,%.0f" % (c, r, x, y))
    if beyond:
        failures.append("printed hex outlines outside the grid: %s" % "; ".join(beyond[:10]))
    offsets = []
    for pid, (c, r) in grid.ids.items():
        x, y = grid.centre(c, r)
        bx, by, score, lined = refine(L, geom, size, x, y, max(3, int(0.15 * hexu)))
        if lined >= 5:
            offsets.append((math.hypot(bx - x, by - y) / hexu, pid, bx - x, by - y))
    offsets.sort(reverse=True)
    vals = np.array([o[0] for o in offsets])
    result["dense"] = dict(checked=len(offsets), median=float(np.median(vals)), p95=float(np.percentile(vals, 95)),
                           max=float(vals.max()), worst=[o[1] for o in offsets[:10]])
    print("dense check: %d hexes with a printed outline; offset median %.3f  p95 %.3f  max %.3f hex" % (
        len(offsets), result["dense"]["median"], result["dense"]["p95"], result["dense"]["max"]))
    print("  worst:", "  ".join("%s %.3f (%+.1f,%+.1f)" % (o[1], o[0], o[2], o[3]) for o in offsets[:10]))
    result["passed"] = not failures
    result["failures"] = failures
    fits[name] = result
    print("GATE PASSED" if not failures else "GATE FAILED:\n  " + "\n  ".join(failures))
    return not failures


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    if "locate" == argv[2]:
        return locate(cfg, argv[3], argv[4:])
    if "fit" != argv[2]:
        print(__doc__)
        return 2
    path = C.work(cfg, "calibration.json")
    try:
        fits = C.read_json(path)
    except FileNotFoundError:
        fits = {}
    names = argv[3:] or [n for n, s in cfg["sources"].items() if s.get("control_points")]
    ok = all([fit(cfg, n, fits) for n in names])
    C.write_json(path, fits)
    print("wrote", path)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
