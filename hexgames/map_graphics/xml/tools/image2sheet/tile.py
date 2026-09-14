# Copyright Ben Paul Wise. All Rights Reserved.
"""tile.py -- stage 3, and the render half of stage 8: cut an image into overlapping, annotated tiles.

    python tile.py MAP scan|render [NAME ...]

Tiles hold core x core hexes (maps/MAP.json "tiles": core, margin, scale) with margin hexes on every
side, cut at the tile scale, each a JPEG under 300 KB carrying the overlay of overlay.py (core ids in
magenta, margin ids in orange). A tile is named FIRST_LAST after the first and last hex of its core.
work/MAP/tiles/manifest.json lists every tile: name, core hexes, margin hexes, pixel box.
"scan" cuts the primary source into work/MAP/tiles/scan/, "render" cuts the rendered sheet PNG (same
pixel frame) into work/MAP/tiles/render/, in exactly the same boxes. NAME limits the run to those tiles.
"""
import math
import sys

from PIL import Image

import common as C
import overlay


def plan(cfg, grid, W, H):
    core_n, margin_n = cfg["tiles"]["core"], cfg["tiles"]["margin"]
    tiles = []
    for j in range(0, grid.rows, core_n):
        for i in range(0, grid.cols, core_n):
            core = [grid.cells[(c, r)] for c in range(i, i + core_n) for r in range(j, j + core_n)
                    if (c, r) in grid.cells]
            if not core:
                continue
            area = [grid.cells[(c, r)] for c in range(i - margin_n, i + core_n + margin_n)
                    for r in range(j - margin_n, j + core_n + margin_n) if (c, r) in grid.cells]
            pts = [pt for pid in area for pt in grid.polygon(*grid.ids[pid])]
            xs, ys = [p[0] for p in pts], [p[1] for p in pts]
            box = [max(0, math.floor(min(xs)) - 6), max(0, math.floor(min(ys)) - 6),
                   min(W, math.ceil(max(xs)) + 6), min(H, math.ceil(max(ys)) + 6)]
            tiles.append(dict(name="%s_%s" % (core[0], core[-1]), core=core,
                              margin=[p for p in area if p not in core], box=box))
    return tiles


def manifest(cfg):
    """The tile plan, written once from the primary calibration and reused by every later stage."""
    grid = C.make_grid(cfg, C.load_fit(cfg))
    with Image.open(C.source(cfg, "primary")["path"]) as im:
        W, H = im.size
    tiles = plan(cfg, grid, W, H)
    C.write_json(C.work(cfg, "tiles", "manifest.json"), dict(scale=cfg["tiles"]["scale"], tiles=tiles))
    return grid, tiles


def cut_all(cfg, image_path, kind, only, draw=None):
    """Cut every planned tile from image_path into work/MAP/tiles/KIND/; draw(img, tile, grid) adds a layer."""
    grid, tiles = manifest(cfg)
    scale = cfg["tiles"]["scale"]
    full = C.open_image(image_path)
    written = 0
    for t in tiles:
        if only and t["name"] not in only:
            continue
        x0, y0, x1, y1 = t["box"]
        img = full.crop((x0, y0, x1, y1)).resize((int((x1 - x0) * scale), int((y1 - y0) * scale)), Image.LANCZOS)
        if draw is not None:
            draw(img, t, grid)
        overlay.draw_grid(img, grid, t["box"], scale, core=set(t["core"]))
        C.save_jpeg(img, C.work(cfg, "tiles", kind, t["name"] + ".jpg"))
        written += 1
    print("wrote %d %s tiles to %s" % (written, kind, C.work(cfg, "tiles", kind)))
    return tiles


def main(argv):
    if len(argv) < 3 or argv[2] not in ("scan", "render"):
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    if "scan" == argv[2]:
        path = C.source(cfg, "primary")["path"]
    else:
        path = C.os.path.join(C.XML_DIR, cfg["sheet"]["png"])
    cut_all(cfg, path, argv[2], set(argv[3:]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
