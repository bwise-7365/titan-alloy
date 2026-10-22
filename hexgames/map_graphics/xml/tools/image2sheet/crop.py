# Copyright Ben Paul Wise. All Rights Reserved.
"""crop.py -- a focused look: a small JPEG (under 300 KB) of one place in a source image or the render.

    python crop.py MAP SOURCE OUT.jpg --hex ID [--radius R] [--scale S] [--grid]
    python crop.py MAP SOURCE OUT.jpg --side A-B [--radius R] [--scale S] [--grid]
    python crop.py MAP SOURCE OUT.jpg --box X0 Y0 X1 Y1 [--scale S] [--grid] [--ruler STEP]

SOURCE is a source name from maps/MAP.json, or "render" for the rendered sheet PNG (which shares the
primary source's pixel frame). --hex and --side centre the crop on a hex or a hexside midpoint, R hexes
each way (default 1.6); they and --grid need the source's calibration. --ruler draws pixel ticks every
STEP source pixels along the top and left edges, labelled in source pixels, so a person or model can
read positions off the crop (use it to find approximate control points before calibration).
Default scale 2.
"""
import os
import sys

from PIL import Image, ImageDraw

import common as C
import overlay


def image_path(cfg, name):
    if "render" == name:
        return os.path.join(C.XML_DIR, cfg["sheet"]["png"]), "primary"
    return C.source(cfg, name)["path"], name


def option(argv, key, count=1, default=None):
    if key not in argv:
        return default
    i = argv.index(key)
    vals = argv[i + 1:i + 1 + count]
    return vals if count > 1 else vals[0]


def ruler(img, box, scale, step):
    d = ImageDraw.Draw(img, "RGBA")
    f = overlay.font(12)
    x0, y0, x1, y1 = box
    for x in range((int(x0) // step + 1) * step, int(x1), step):
        px = (x - x0) * scale
        d.line((px, 0, px, 14), fill=(255, 0, 0, 255), width=2)
        d.text((px + 2, 14), str(x), font=f, fill=(255, 0, 0, 255), stroke_width=2, stroke_fill=(255, 255, 255))
    for y in range((int(y0) // step + 1) * step, int(y1), step):
        py = (y - y0) * scale
        d.line((0, py, 14, py), fill=(255, 0, 0, 255), width=2)
        d.text((16, py - 7), str(y), font=f, fill=(255, 0, 0, 255), stroke_width=2, stroke_fill=(255, 255, 255))
    return img


def cut(cfg, name, box, scale, grid=None, core=None, step=None):
    path, _ = image_path(cfg, name)
    with Image.open(path) as full:
        W, H = full.size
        box = (max(0, box[0]), max(0, box[1]), min(W, box[2]), min(H, box[3]))
        img = full.convert("RGB").crop(tuple(int(round(v)) for v in box))
    box = tuple(int(round(v)) for v in box)
    img = img.resize((int(img.width * scale), int(img.height * scale)), Image.LANCZOS)
    if grid is not None:
        overlay.draw_grid(img, grid, box, scale, core=core)
    if step:
        ruler(img, box, scale, step)
    return img


def main(argv):
    if len(argv) < 5:
        print(__doc__)
        return 2
    cfg = C.load_config(argv[1])
    name, out = argv[2], argv[3]
    scale = float(option(argv, "--scale", default="2"))
    radius = float(option(argv, "--radius", default="1.6"))
    fit_name = image_path(cfg, name)[1]
    grid = C.make_grid(cfg, C.load_fit(cfg, fit_name)) if ("--hex" in argv or "--side" in argv or "--grid" in argv) else None
    if "--hex" in argv:
        pid = C.known(grid, option(argv, "--hex"), "crop")
        cx, cy = grid.centre(*grid.ids[pid])
        core = [pid]
    elif "--side" in argv:
        side = C.canonical_side(grid, option(argv, "--side"), "crop")
        (ax, ay), (bx, by) = C.side_segment(grid, side)
        cx, cy = (ax + bx) / 2, (ay + by) / 2
        core = side.split(":")[0].split("-")
    elif "--box" in argv:
        x0, y0, x1, y1 = (float(v) for v in option(argv, "--box", 4))
        img = cut(cfg, name, (x0, y0, x1, y1), scale, grid if "--grid" in argv else None,
                  step=int(option(argv, "--ruler")) if "--ruler" in argv else None)
        print("wrote", C.save_jpeg(img, out))
        return 0
    else:
        print(__doc__)
        return 2
    half = radius * C.spacing(grid)
    img = cut(cfg, name, (cx - half, cy - half, cx + half, cy + half), scale,
              grid if "--grid" in argv else None, core, int(option(argv, "--ruler")) if "--ruler" in argv else None)
    print("wrote", C.save_jpeg(img, out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
# Copyright Ben Paul Wise. All Rights Reserved.
